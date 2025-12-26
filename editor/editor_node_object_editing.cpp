/**************************************************************************/
/*  editor_node_object_editing.cpp                                      */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             OCULUS ENGINE                             */
/*                        https://oculusengine.org                        */
/**************************************************************************/
/* Copyright (c) 2025-present Oculus Engine contributors (see AUTHORS.md). */
/*                                                                        */
/* This file was originally derived from Godot Engine:                   */
/* https://github.com/godotengine/godot/tree/63227bbc8ae5300319f14f8253c8158b846f355b */
/*                                                                        */
/* Original Godot Engine copyright:                                      */
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "editor_node.h"

#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/object/script_language.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/docks/groups_dock.h"
#include "editor/docks/import_dock.h"
#include "editor/docks/inspector_dock.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/docks/signals_dock.h"
#include "editor/editor_main_screen.h"
#include "editor/inspector/editor_properties.h"
#include "editor/inspector/multi_node_edit.h"
#include "editor/plugins/editor_plugin.h"
#include "editor/scene/editor_scene_tabs.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_feature_profile.h"
#include "editor/settings/editor_settings.h"

void EditorNode::edit_node(Node *p_node) {
	push_item(p_node);
}

bool EditorNode::_is_class_editor_disabled_by_feature_profile(const StringName &p_class) {
	Ref<EditorFeatureProfile> profile = EditorFeatureProfileManager::get_singleton()->get_current_profile();
	if (profile.is_null()) {
		return false;
	}

	StringName class_name = p_class;

	while (class_name != StringName()) {
		if (profile->is_class_disabled(class_name)) {
			return true;
		}
		if (profile->is_class_editor_disabled(class_name)) {
			return true;
		}
		class_name = ClassDB::get_parent_class(class_name);
	}

	return false;
}

void EditorNode::edit_item(Object *p_object, Object *p_editing_owner) {
	ERR_FAIL_NULL(p_editing_owner);

	// Editing for this type of object may be disabled by user's feature profile.
	if (!p_object || _is_class_editor_disabled_by_feature_profile(p_object->get_class())) {
		// Nothing to edit, clean up the owner context and return.
		hide_unused_editors(p_editing_owner);
		return;
	}

	// Get a list of editor plugins that can handle this type of object.
	Vector<EditorPlugin *> available_plugins = editor_data.get_handling_sub_editors(p_object);
	if (available_plugins.is_empty()) {
		// None, clean up the owner context and return.
		hide_unused_editors(p_editing_owner);
		return;
	}

	ObjectID owner_id = p_editing_owner->get_instance_id();

	// Remove editor plugins no longer used by this editing owner. Keep the ones that can
	// still be reused by the new edited object.

	List<EditorPlugin *> to_remove;
	for (EditorPlugin *plugin : active_plugins[owner_id]) {
		if (!available_plugins.has(plugin)) {
			to_remove.push_back(plugin);
			if (plugin->can_auto_hide()) {
				_plugin_over_edit(plugin, nullptr);
			} else {
				// If plugin can't be hidden, make it own itself and become responsible for closing.
				_plugin_over_self_own(plugin);
			}
		}
	}

	for (EditorPlugin *plugin : to_remove) {
		active_plugins[owner_id].erase(plugin);
	}

	LocalVector<EditorPlugin *> to_over_edit;

	// Send the edited object to the plugins.
	for (EditorPlugin *plugin : available_plugins) {
		if (active_plugins[owner_id].has(plugin)) {
			// Plugin was already active, just change the object and ensure it's visible.
			plugin->make_visible(true);
			plugin->edit(p_object);
			continue;
		}

		if (active_plugins.has(plugin->get_instance_id())) {
			// Plugin is already active, but as self-owning, so it needs a separate check.
			plugin->make_visible(true);
			plugin->edit(p_object);
			continue;
		}

		bool need_to_add = true;
		List<EditorPropertyResource *> to_fold;

		// If plugin is already associated with another owner, remove it from there first.
		for (KeyValue<ObjectID, HashSet<EditorPlugin *>> &kv : active_plugins) {
			if (kv.key == owner_id || !kv.value.has(plugin)) {
				continue;
			}
			EditorPropertyResource *epres = ObjectDB::get_instance<EditorPropertyResource>(kv.key);
			if (epres) {
				// If it's resource property editing the same resource type, fold it later to avoid premature modifications
				// that may result in unsafe iteration of active_plugins.
				to_fold.push_back(epres);
			} else {
				kv.value.erase(plugin);
				need_to_add = false;
			}
		}

		if (!need_to_add && to_fold.is_empty()) {
			plugin->make_visible(true);
			plugin->edit(p_object);
		} else {
			for (EditorPropertyResource *epres : to_fold) {
				epres->fold_resource();
			}

			// TODO: Call the function directly once a proper priority system is implemented.
			to_over_edit.push_back(plugin);
		}

		// Activate previously inactive plugin and edit the object.
		active_plugins[owner_id].insert(plugin);
	}

	for (EditorPlugin *plugin : to_over_edit) {
		_plugin_over_edit(plugin, p_object);
	}
}

void EditorNode::push_node_item(Node *p_node) {
	if (p_node || !InspectorDock::get_inspector_singleton()->get_edited_object() || Object::cast_to<Node>(InspectorDock::get_inspector_singleton()->get_edited_object()) || Object::cast_to<MultiNodeEdit>(InspectorDock::get_inspector_singleton()->get_edited_object())) {
		// Don't push null if the currently edited object is not a Node.
		push_item(p_node);
	}
}

void EditorNode::push_item(Object *p_object, const String &p_property, bool p_inspector_only) {
	if (!p_object) {
		InspectorDock::get_inspector_singleton()->edit(nullptr);
		SignalsDock::get_singleton()->set_object(nullptr);
		GroupsDock::get_singleton()->set_selection(Vector<Node *>());
		SceneTreeDock::get_singleton()->set_selected(nullptr);
		InspectorDock::get_singleton()->update(nullptr);
		hide_unused_editors();
		return;
	}
	_add_to_history(p_object, p_property, p_inspector_only);
	_edit_current();
}

void EditorNode::edit_previous_item() {
	if (editor_history.previous()) {
		_edit_current();
	}
}

void EditorNode::push_item_no_inspector(Object *p_object) {
	_add_to_history(p_object, "", false);
	_edit_current(false, true);
}

void EditorNode::hide_unused_editors(const Object *p_editing_owner) {
	if (p_editing_owner) {
		const ObjectID id = p_editing_owner->get_instance_id();
		for (EditorPlugin *plugin : active_plugins[id]) {
			if (plugin->can_auto_hide()) {
				_plugin_over_edit(plugin, nullptr);
			} else {
				_plugin_over_self_own(plugin);
			}
		}
		active_plugins.erase(id);
	} else {
		// If no editing owner is provided, this method will go over all owners and check if they are valid.
		// This is to sweep properties that were removed from the inspector.
		List<ObjectID> to_remove;
		for (KeyValue<ObjectID, HashSet<EditorPlugin *>> &kv : active_plugins) {
			Object *context = ObjectDB::get_instance(kv.key);
			if (context) {
				// In case of self-owning plugins, they are disabled here if they can auto hide.
				const EditorPlugin *self_owning = Object::cast_to<EditorPlugin>(context);
				if (self_owning && self_owning->can_auto_hide()) {
					context = nullptr;
				}
			}

			if (!context || context->call(SNAME("_should_stop_editing"))) {
				to_remove.push_back(kv.key);
				for (EditorPlugin *plugin : kv.value) {
					if (plugin->can_auto_hide()) {
						_plugin_over_edit(plugin, nullptr);
					} else {
						_plugin_over_self_own(plugin);
					}
				}
			}
		}

		for (const ObjectID &id : to_remove) {
			active_plugins.erase(id);
		}
	}
}

void EditorNode::_add_to_history(const Object *p_object, const String &p_property, bool p_inspector_only) {
	ObjectID id = p_object->get_instance_id();
	ObjectID history_id = editor_history.get_current();
	if (id != history_id) {
		const MultiNodeEdit *multi_node_edit = Object::cast_to<const MultiNodeEdit>(p_object);
		const MultiNodeEdit *history_multi_node_edit = ObjectDB::get_instance<MultiNodeEdit>(history_id);
		if (multi_node_edit && history_multi_node_edit && multi_node_edit->is_same_selection(history_multi_node_edit)) {
			return;
		}
		if (p_inspector_only) {
			editor_history.add_object(id, String(), true);
		} else if (p_property.is_empty()) {
			editor_history.add_object(id);
		} else {
			editor_history.add_object(id, p_property);
		}
	}
}

void EditorNode::_edit_current(bool p_skip_foreign, bool p_skip_inspector_update) {
	ObjectID current_id = editor_history.get_current();
	Object *current_obj = current_id.is_valid() ? ObjectDB::get_instance(current_id) : nullptr;

	Ref<Resource> res = Object::cast_to<Resource>(current_obj);
	if (p_skip_foreign && res.is_valid()) {
		const int current_tab = scene_tabs->get_current_tab();
		if (res->get_path().contains("::") && res->get_path().get_slice("::", 0) != editor_data.get_scene_path(current_tab)) {
			// Trying to edit resource that belongs to another scene; abort.
			current_obj = nullptr;
		}
	}

	bool inspector_only = editor_history.is_current_inspector_only();

	if (!current_obj) {
		SceneTreeDock::get_singleton()->set_selected(nullptr);
		InspectorDock::get_inspector_singleton()->edit(nullptr);
		SignalsDock::get_singleton()->set_object(nullptr);
		GroupsDock::get_singleton()->set_selection(Vector<Node *>());
		InspectorDock::get_singleton()->update(nullptr);
		EditorDebuggerNode::get_singleton()->clear_remote_tree_selection();
		hide_unused_editors();
		return;
	}

	// Update the use folding setting and state.
	bool disable_folding = bool(EDITOR_GET("interface/inspector/disable_folding")) || current_obj->is_class("EditorDebuggerRemoteObjects");
	if (InspectorDock::get_inspector_singleton()->is_using_folding() == disable_folding) {
		InspectorDock::get_inspector_singleton()->set_use_folding(!disable_folding, false);
	}

	bool is_resource = current_obj->is_class("Resource");
	bool is_node = current_obj->is_class("Node");
	bool stay_in_script_editor_on_node_selected = bool(EDITOR_GET("text_editor/behavior/navigation/stay_in_script_editor_on_node_selected"));
	bool skip_main_plugin = false;

	String editable_info; // None by default.
	bool info_is_warning = false;

	if (current_obj->has_method("_is_read_only")) {
		if (current_obj->call("_is_read_only")) {
			editable_info = TTR("This object is marked as read-only, so it's not editable.");
		}
	}

	if (is_resource) {
		Resource *current_res = Object::cast_to<Resource>(current_obj);
		ERR_FAIL_NULL(current_res);

		if (!p_skip_inspector_update) {
			InspectorDock::get_inspector_singleton()->edit(current_res);
			SceneTreeDock::get_singleton()->set_selected(nullptr);
			SignalsDock::get_singleton()->set_object(current_res);
			GroupsDock::get_singleton()->set_selection(Vector<Node *>());
			InspectorDock::get_singleton()->update(nullptr);
			EditorDebuggerNode::get_singleton()->clear_remote_tree_selection();
			ImportDock::get_singleton()->set_edit_path(current_res->get_path());
		}

		int subr_idx = current_res->get_path().find("::");
		if (subr_idx != -1) {
			String base_path = current_res->get_path().substr(0, subr_idx);
			if (FileAccess::exists(base_path + ".import")) {
				if (!base_path.is_resource_file()) {
					if (get_edited_scene() && get_edited_scene()->get_scene_file_path() == base_path) {
						info_is_warning = true;
					}
				}
				editable_info = TTR("This resource belongs to a scene that was imported, so it's not editable.\nPlease read the documentation relevant to importing scenes to better understand this workflow.");
			} else if ((!get_edited_scene() || get_edited_scene()->get_scene_file_path() != base_path) && ResourceLoader::get_resource_type(base_path) == "PackedScene") {
				editable_info = TTR("This resource belongs to a scene that was instantiated or inherited.\nChanges to it must be made inside the original scene.");
			}
		} else if (current_res->get_path().is_resource_file()) {
			if (FileAccess::exists(current_res->get_path() + ".import")) {
				editable_info = TTR("This resource was imported, so it's not editable. Change its settings in the import panel and then re-import.");
			}
		}
	} else if (is_node) {
		Node *current_node = Object::cast_to<Node>(current_obj);
		ERR_FAIL_NULL(current_node);

		InspectorDock::get_inspector_singleton()->edit(current_node);
		if (current_node->is_inside_tree()) {
			SignalsDock::get_singleton()->set_object(current_node);
			GroupsDock::get_singleton()->set_selection(Vector<Node *>{ current_node });
			SceneTreeDock::get_singleton()->set_selected(current_node);
			SceneTreeDock::get_singleton()->set_selection({ current_node });
			InspectorDock::get_singleton()->update(current_node);
			if (!inspector_only && !skip_main_plugin) {
				if (!ScriptEditor::get_singleton()->is_editor_floating() && ScriptEditor::get_singleton()->is_visible_in_tree()) {
					skip_main_plugin = stay_in_script_editor_on_node_selected;
				} else {
					skip_main_plugin = !editor_main_screen->can_auto_switch_screens();
				}
			}
		} else {
			SignalsDock::get_singleton()->set_object(nullptr);
			GroupsDock::get_singleton()->set_selection(Vector<Node *>());
			SceneTreeDock::get_singleton()->set_selected(nullptr);
			InspectorDock::get_singleton()->update(nullptr);
		}
		EditorDebuggerNode::get_singleton()->clear_remote_tree_selection();

		if (get_edited_scene() && !get_edited_scene()->get_scene_file_path().is_empty()) {
			String source_scene = get_edited_scene()->get_scene_file_path();
			if (FileAccess::exists(source_scene + ".import")) {
				editable_info = TTR("This scene was imported, so changes to it won't be kept.\nInstantiating or inheriting it will allow you to make changes to it.\nPlease read the documentation relevant to importing scenes to better understand this workflow.");
				info_is_warning = true;
			}
		}
	} else {
		Node *selected_node = nullptr;

		Vector<Node *> multi_nodes;
		if (current_obj->is_class("MultiNodeEdit")) {
			Node *scene = get_edited_scene();
			if (scene) {
				MultiNodeEdit *multi_node_edit = Object::cast_to<MultiNodeEdit>(current_obj);
				int node_count = multi_node_edit->get_node_count();
				if (node_count > 0) {
					for (int node_index = 0; node_index < node_count; ++node_index) {
						Node *node = scene->get_node(multi_node_edit->get_node(node_index));
						if (node) {
							multi_nodes.push_back(node);
						}
					}
					if (!multi_nodes.is_empty()) {
						// Pick the top-most node.
						selected_node = multi_nodes[0];
						Node::Comparator comparator;
						for (Node *node : multi_nodes) {
							if (comparator(node, selected_node)) {
								selected_node = node;
							}
						}
					}
				}
			}
		}

		if (!current_obj->is_class("EditorDebuggerRemoteObjects")) {
			EditorDebuggerNode::get_singleton()->clear_remote_tree_selection();
		}

		InspectorDock::get_inspector_singleton()->edit(current_obj);
		SignalsDock::get_singleton()->set_object(nullptr);
		GroupsDock::get_singleton()->set_selection(multi_nodes);
		SceneTreeDock::get_singleton()->set_selected(selected_node);
		SceneTreeDock::get_singleton()->set_selection(multi_nodes);
		InspectorDock::get_singleton()->update(nullptr);
	}

	InspectorDock::get_singleton()->set_info(
			info_is_warning ? TTR("Changes may be lost!") : TTR("This object is read-only."),
			editable_info,
			info_is_warning);

	Object *editor_owner = (is_node || current_obj->is_class("MultiNodeEdit")) ? (Object *)SceneTreeDock::get_singleton() : is_resource ? (Object *)InspectorDock::get_inspector_singleton()
																																		: (Object *)this;

	// Take care of the main editor plugin.

	if (!inspector_only) {
		EditorPlugin *main_plugin = editor_data.get_handling_main_editor(current_obj);

		int plugin_index = editor_main_screen->get_plugin_index(main_plugin);
		if (main_plugin && plugin_index >= 0 && !editor_main_screen->is_button_enabled(plugin_index)) {
			main_plugin = nullptr;
		}
		EditorPlugin *editor_plugin_screen = editor_main_screen->get_selected_plugin();

		ObjectID editor_owner_id = editor_owner->get_instance_id();
		if (main_plugin && !skip_main_plugin) {
			// Special case if current_obj is a script.
			Script *current_script = Object::cast_to<Script>(current_obj);
			if (current_script) {
				if (!changing_scene) {
					// Only update main editor screen if using in-engine editor.
					if (current_script->is_built_in() || (!bool(EDITOR_GET("text_editor/external/use_external_editor")) && !current_script->get_language()->overrides_external_editor())) {
						editor_main_screen->select(plugin_index);
					}

					main_plugin->edit(current_script);
				}
			} else if (main_plugin != editor_plugin_screen) {
				// Unedit previous plugin.
				editor_plugin_screen->edit(nullptr);
				active_plugins[editor_owner_id].erase(editor_plugin_screen);
				// Update screen main_plugin.
				editor_main_screen->select(plugin_index);
				main_plugin->edit(current_obj);
			} else {
				editor_plugin_screen->edit(current_obj);
			}
			is_main_screen_editing = true;
		} else if (!main_plugin && editor_plugin_screen && is_main_screen_editing) {
			editor_plugin_screen->edit(nullptr);
			is_main_screen_editing = false;
		}

		edit_item(current_obj, editor_owner);
	}

	InspectorDock::get_singleton()->update(current_obj);
}

