/**************************************************************************/
/*  editor_node_scene_management.cpp                                      */
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
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "editor_node.h"

#include "core/config/project_settings.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/script_language.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/debugger/script_editor_debugger.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/docks/inspector_dock.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_data.h"
#include "editor/settings/editor_folding.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/scene/editor_scene_tabs.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/gui/editor_file_dialog.h"
#include "scene/2d/node_2d.h"
#include "scene/3d/bone_attachment_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/gui/popup.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/packed_scene.h"
#include "servers/rendering/rendering_server.h"
#include "core/os/time.h"

static String _get_unsaved_scene_dialog_text(String p_scene_filename, uint64_t p_started_timestamp) {
	String unsaved_message;

	// Consider editor startup to be a point of saving, so that when you
	// close and reopen the editor, you don't get an excessively long
	// "modified X hours ago".
	const uint64_t last_modified_seconds = Time::get_singleton()->get_unix_time_from_system() - MAX(p_started_timestamp, FileAccess::get_modified_time(p_scene_filename));
	String last_modified_string;
	if (last_modified_seconds < 120) {
		last_modified_string = vformat(TTRN("%d second ago", "%d seconds ago", last_modified_seconds), last_modified_seconds);
	} else if (last_modified_seconds < 7200) {
		last_modified_string = vformat(TTRN("%d minute ago", "%d minutes ago", last_modified_seconds / 60), last_modified_seconds / 60);
	} else {
		last_modified_string = vformat(TTRN("%d hour ago", "%d hours ago", last_modified_seconds / 3600), last_modified_seconds / 3600);
	}
	unsaved_message = vformat(TTR("Scene \"%s\" has unsaved changes.\nLast saved: %s."), p_scene_filename, last_modified_string);

	return unsaved_message;
}

void EditorNode::_resave_externally_modified_scenes(String p_str) {
	for (const String &scene_path : disk_changed_scenes) {
		_save_scene(scene_path);
	}

	if (disk_changed_project) {
		ProjectSettings::get_singleton()->save();
	}

	disk_changed->hide();
}

void EditorNode::_reload_modified_scenes() {
	int current_idx = editor_data.get_edited_scene();

	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		if (editor_data.get_scene_path(i) == "") {
			continue;
		}

		uint64_t last_date = editor_data.get_scene_modified_time(i);
		uint64_t date = FileAccess::get_modified_time(editor_data.get_scene_path(i));

		if (date > last_date) {
			String filename = editor_data.get_scene_path(i);
			editor_data.set_edited_scene(i);
			_remove_edited_scene(false);

			Error err = load_scene(filename, false, false, false, true);
			if (err != OK) {
				ERR_PRINT(vformat("Failed to load scene: %s", filename));
			}
			editor_data.move_edited_scene_to_index(i);
		}
	}

	_set_current_scene(current_idx);
	scene_tabs->update_scene_tabs();
	disk_changed->hide();
}

void EditorNode::_remove_edited_scene(bool p_change_tab) {
	// When scene gets closed no node is edited anymore, so make sure the editors are notified before nodes are freed.
	hide_unused_editors(SceneTreeDock::get_singleton());
	SceneTreeDock::get_singleton()->clear_previous_node_selection();

	int new_index = editor_data.get_edited_scene();
	int old_index = new_index;

	if (new_index > 0) {
		new_index = new_index - 1;
	} else if (editor_data.get_edited_scene_count() > 1) {
		new_index = 1;
	} else {
		editor_data.add_edited_scene(-1);
		new_index = 1;
	}

	if (p_change_tab) {
		_set_current_scene(new_index);
	}
	editor_data.remove_scene(old_index);
	_update_title();
	scene_tabs->update_scene_tabs();
}

void EditorNode::_remove_scene(int index, bool p_change_tab) {
	// Clear icon cache in case some scripts are no longer needed or class icons are outdated.
	// FIXME: Ideally the cache should never be cleared and only updated on per-script basis, when an icon changes.
	editor_data.clear_script_icon_cache();
	class_icon_cache.clear();

	if (editor_data.get_edited_scene() == index) {
		// Scene to remove is current scene.
		_remove_edited_scene(p_change_tab);
	} else {
		// Scene to remove is not active scene.
		editor_data.remove_scene(index);
	}
}

void EditorNode::set_edited_scene(Node *p_scene) {
	set_edited_scene_root(p_scene, true);
}

void EditorNode::set_edited_scene_root(Node *p_scene, bool p_auto_add) {
	Node *old_edited_scene_root = get_editor_data().get_edited_scene_root();
	ERR_FAIL_COND_MSG(p_scene && p_scene != old_edited_scene_root && p_scene->get_parent(), "Non-null nodes that are set as edited scene should not have a parent node.");

	if (p_auto_add && old_edited_scene_root && old_edited_scene_root->get_parent() == scene_root) {
		scene_root->remove_child(old_edited_scene_root);
	}
	get_editor_data().set_edited_scene_root(p_scene);

	if (Object::cast_to<Popup>(p_scene)) {
		Object::cast_to<Popup>(p_scene)->show();
	}
	SceneTreeDock::get_singleton()->set_edited_scene(p_scene);
	if (get_tree()) {
		get_tree()->set_edited_scene_root(p_scene);
	}

	if (p_auto_add && p_scene) {
		scene_root->add_child(p_scene, true);
	}
}

Dictionary EditorNode::_get_main_scene_state() {
	Dictionary state;
	state["scene_tree_offset"] = SceneTreeDock::get_singleton()->get_tree_editor()->get_scene_tree()->get_vscroll_bar()->get_value();
	state["property_edit_offset"] = InspectorDock::get_inspector_singleton()->get_scroll_offset();
	state["node_filter"] = SceneTreeDock::get_singleton()->get_filter();
	return state;
}

void EditorNode::_set_main_scene_state(Dictionary p_state, Node *p_for_scene) {
	if (get_edited_scene() != p_for_scene && p_for_scene != nullptr) {
		return; // Not for this scene.
	}

	changing_scene = false;

	if (get_edited_scene()) {
		if (editor_main_screen->can_auto_switch_screens()) {
			// Switch between 2D and 3D if currently in 2D or 3D.
			Node *selected_node = SceneTreeDock::get_singleton()->get_tree_editor()->get_selected();
			if (!selected_node) {
				selected_node = get_edited_scene();
			}
			const int plugin_index = editor_main_screen->get_plugin_index(editor_data.get_handling_main_editor(selected_node));
			if (plugin_index >= 0) {
				editor_main_screen->select(plugin_index);
			}
		}
	}

	if (p_state.has("scene_tree_offset")) {
		SceneTreeDock::get_singleton()->get_tree_editor()->get_scene_tree()->get_vscroll_bar()->set_value(p_state["scene_tree_offset"]);
	}
	if (p_state.has("property_edit_offset")) {
		InspectorDock::get_inspector_singleton()->set_scroll_offset(p_state["property_edit_offset"]);
	}

	if (p_state.has("node_filter")) {
		SceneTreeDock::get_singleton()->set_filter(p_state["node_filter"]);
	}

	// This should only happen at the very end.

	EditorDebuggerNode::get_singleton()->update_live_edit_root();
	ScriptEditor::get_singleton()->set_scene_root_script(editor_data.get_scene_root_script(editor_data.get_edited_scene()));
	editor_data.notify_edited_scene_changed();
	emit_signal(SNAME("scene_changed"));

	// Reset SDFGI after everything else so that any last-second scene modifications will be processed.
	RenderingServer::get_singleton()->sdfgi_reset();
}

bool EditorNode::is_changing_scene() const {
	return changing_scene;
}

void EditorNode::_set_current_scene(int p_idx) {
	if (p_idx == editor_data.get_edited_scene()) {
		return; // Pointless.
	}

	_set_current_scene_nocheck(p_idx);
}

void EditorNode::_set_current_scene_nocheck(int p_idx) {
	// Save the folding in case the scene gets reloaded.
	if (editor_data.get_scene_path(p_idx) != "" && editor_data.get_edited_scene_root(p_idx)) {
		editor_folding.save_scene_folding(editor_data.get_edited_scene_root(p_idx), editor_data.get_scene_path(p_idx));
	}

	changing_scene = true;
	editor_data.save_edited_scene_state(editor_selection, &editor_history, _get_main_scene_state());

	Node *old_scene = get_editor_data().get_edited_scene_root();

	resource_count.clear();
	editor_selection->clear();
	SceneTreeDock::get_singleton()->clear_previous_node_selection();
	editor_data.set_edited_scene(p_idx);

	Node *new_scene = editor_data.get_edited_scene_root();

	// Remove the scene only if it's a new scene, preventing performance issues when adding and removing scenes.
	if (old_scene && new_scene != old_scene && old_scene->get_parent() == scene_root) {
		scene_root->remove_child(old_scene);
	}

	if (Popup *p = Object::cast_to<Popup>(new_scene)) {
		p->show();
	}

	SceneTreeDock::get_singleton()->set_edited_scene(new_scene);
	if (get_tree()) {
		get_tree()->set_edited_scene_root(new_scene);
	}

	if (new_scene) {
		if (new_scene->get_parent() != scene_root) {
			scene_root->add_child(new_scene, true);
		}
	}

	if (editor_data.check_and_update_scene(p_idx)) {
		if (!editor_data.get_scene_path(p_idx).is_empty()) {
			editor_folding.load_scene_folding(editor_data.get_edited_scene_root(p_idx), editor_data.get_scene_path(p_idx));
		}

		EditorUndoRedoManager::get_singleton()->clear_history(editor_data.get_scene_history_id(p_idx), false);
	}

	Dictionary state = editor_data.restore_edited_scene_state(editor_selection, &editor_history);
	_edit_current(true);

	_update_title();
	callable_mp(scene_tabs, &EditorSceneTabs::update_scene_tabs).call_deferred();

	if (tabs_to_close.is_empty() && !restoring_scenes) {
		callable_mp(this, &EditorNode::_set_main_scene_state).call_deferred(state, get_edited_scene()); // Do after everything else is done setting up.
	}

	if (!select_current_scene_file_requested && EDITOR_GET("interface/scene_tabs/auto_select_current_scene_file")) {
		select_current_scene_file_requested = true;
		callable_mp(this, &EditorNode::_nav_to_selected_scene).call_deferred();
	}

	_update_undo_redo_allowed();
	_update_unsaved_cache();
}

void EditorNode::_nav_to_selected_scene() {
	select_current_scene_file_requested = false;
	const String scene_path = editor_data.get_scene_path(scene_tabs->get_current_tab());
	if (!scene_path.is_empty()) {
		FileSystemDock::get_singleton()->navigate_to_path(scene_path);
	}
}

bool EditorNode::is_scene_open(const String &p_path) {
	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		if (editor_data.get_scene_path(i) == p_path) {
			return true;
		}
	}

	return false;
}

int EditorNode::new_scene() {
	int idx = editor_data.add_edited_scene(-1);
	_set_current_scene(idx); // Before trying to remove an empty scene, set the current tab index to the newly added tab index.

	// Remove placeholder empty scene.
	if (editor_data.get_edited_scene_count() > 1) {
		for (int i = 0; i < editor_data.get_edited_scene_count() - 1; i++) {
			bool unsaved = EditorUndoRedoManager::get_singleton()->is_history_unsaved(editor_data.get_scene_history_id(i));
			if (!unsaved && editor_data.get_scene_path(i).is_empty() && editor_data.get_edited_scene_root(i) == nullptr) {
				editor_data.remove_scene(i);
				idx--;
			}
		}
	}

	editor_data.clear_editor_states();
	scene_tabs->update_scene_tabs();
	return idx;
}

void EditorNode::_proceed_closing_scene_tabs() {
	List<String>::Element *E = tabs_to_close.front();
	if (!E) {
		if (_is_closing_editor()) {
			current_menu_option = tab_closing_menu_option;
			_menu_option_confirm(tab_closing_menu_option, true);
		} else {
			current_menu_option = -1;
			save_confirmation->hide();
		}
		return;
	}
	String scene_to_close = E->get();
	tabs_to_close.pop_front();

	int tab_idx = -1;
	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		if (editor_data.get_scene_path(i) == scene_to_close) {
			tab_idx = i;
			break;
		}
	}
	ERR_FAIL_COND(tab_idx < 0);

	_scene_tab_closed(tab_idx);
}

void EditorNode::_proceed_save_asing_scene_tabs() {
	if (scenes_to_save_as.is_empty()) {
		return;
	}
	int scene_idx = scenes_to_save_as.front()->get();
	scenes_to_save_as.pop_front();
	_set_current_scene(scene_idx);
	_menu_option_confirm(SCENE_MULTI_SAVE_AS_SCENE, false);
}

void EditorNode::_scene_tab_closed(int p_tab) {
	current_menu_option = SCENE_TAB_CLOSE;
	tab_closing_idx = p_tab;
	Node *scene = editor_data.get_edited_scene_root(p_tab);
	if (!scene) {
		_discard_changes();
		return;
	}

	String scene_filename = scene->get_scene_file_path();
	String unsaved_message;

	if (EditorUndoRedoManager::get_singleton()->is_history_unsaved(editor_data.get_scene_history_id(p_tab))) {
		if (scene_filename.is_empty()) {
			unsaved_message = TTR("This scene was never saved.");
		} else {
			unsaved_message = _get_unsaved_scene_dialog_text(scene_filename, started_timestamp);
		}
	} else {
		// Check if any plugin has unsaved changes in that scene.
		for (int i = 0; i < editor_data.get_editor_plugin_count(); i++) {
			unsaved_message = editor_data.get_editor_plugin(i)->get_unsaved_status(scene_filename);
			if (!unsaved_message.is_empty()) {
				break;
			}
		}
	}

	if (!unsaved_message.is_empty()) {
		if (scene_tabs->get_current_tab() != p_tab) {
			_set_current_scene(p_tab);
		}

		save_confirmation->set_ok_button_text(TTR("Save & Close"));
		save_confirmation->set_text(unsaved_message + "\n\n" + TTR("Save before closing?"));
		save_confirmation->reset_size();
		save_confirmation->popup_centered();
	} else {
		_discard_changes();
	}

	save_editor_layout_delayed();
	scene_tabs->update_scene_tabs();
}

void EditorNode::_cancel_close_scene_tab() {
	if (_is_closing_editor()) {
		tab_closing_menu_option = -1;
	}
	changing_scene = false;
	tabs_to_close.clear();
}

void EditorNode::request_instantiate_scene(const String &p_path) {
	SceneTreeDock::get_singleton()->instantiate(p_path);
}

void EditorNode::request_instantiate_scenes(const Vector<String> &p_files) {
	SceneTreeDock::get_singleton()->instantiate_scenes(p_files);
}

void EditorNode::_pick_main_scene_custom_action(const String &p_custom_action_name) {
	if (p_custom_action_name == "select_current") {
		Node *scene = editor_data.get_edited_scene_root();

		if (!scene) {
			show_accept(TTR("There is no defined scene to run."), TTR("OK"));
			return;
		}

		pick_main_scene->hide();

		if (!FileAccess::exists(scene->get_scene_file_path())) {
			current_menu_option = SAVE_AND_RUN_MAIN_SCENE;
			_menu_option_confirm(SCENE_SAVE_AS_SCENE, true);
			file->set_title(TTR("Save scene before running..."));
		} else {
			current_menu_option = SETTINGS_PICK_MAIN_SCENE;
			_dialog_action(scene->get_scene_file_path());
		}
	}
}

void EditorNode::call_run_scene(const String &p_scene, Vector<String> &r_args) {
	for (int i = 0; i < editor_data.get_editor_plugin_count(); i++) {
		EditorPlugin *plugin = editor_data.get_editor_plugin(i);
		plugin->run_scene(p_scene, r_args);
	}
}

void EditorNode::get_scene_editor_data_for_node(Node *p_root, Node *p_node, HashMap<NodePath, SceneEditorDataEntry> &p_table) {
	SceneEditorDataEntry new_entry;
	new_entry.is_display_folded = p_node->is_displayed_folded();

	if (p_root != p_node) {
		new_entry.is_editable = p_root->is_editable_instance(p_node);
	}

	p_table.insert(p_root->get_path_to(p_node), new_entry);

	for (int i = 0; i < p_node->get_child_count(); i++) {
		get_scene_editor_data_for_node(p_root, p_node->get_child(i), p_table);
	}
}

void EditorNode::get_preload_scene_modification_table(
		Node *p_edited_scene,
		Node *p_reimported_root,
		Node *p_node, InstanceModificationsEntry &p_instance_modifications) {
	if (is_additional_node_in_scene(p_edited_scene, p_reimported_root, p_node)) {
		// Only save additional nodes which have an owner since this was causing issues transient ownerless nodes
		// which get recreated upon scene tree entry.
		// For now instead, assume all ownerless nodes are transient and will have to be recreated.
		if (p_node->get_owner()) {
			HashMap<StringName, Variant> modified_properties = get_modified_properties_for_node(p_node, true);
			if (p_node->get_owner() == p_edited_scene) {
				AdditiveNodeEntry new_additive_node_entry;
				new_additive_node_entry.node = p_node;
				new_additive_node_entry.parent = p_reimported_root->get_path_to(p_node->get_parent());
				new_additive_node_entry.owner = p_node->get_owner();
				new_additive_node_entry.index = p_node->get_index();

				Node2D *node_2d = Object::cast_to<Node2D>(p_node);
				if (node_2d) {
					new_additive_node_entry.transform_2d = node_2d->get_transform();
				}
				Node3D *node_3d = Object::cast_to<Node3D>(p_node);
				if (node_3d) {
					new_additive_node_entry.transform_3d = node_3d->get_transform();
				}

				p_instance_modifications.addition_list.push_back(new_additive_node_entry);
			}
			if (!modified_properties.is_empty()) {
				ModificationNodeEntry modification_node_entry;
				modification_node_entry.property_table = modified_properties;

				p_instance_modifications.modifications[p_reimported_root->get_path_to(p_node)] = modification_node_entry;
			}
		}
	} else {
		HashMap<StringName, Variant> modified_properties = get_modified_properties_for_node(p_node, false);

		// Find all valid connections to other nodes.
		List<Connection> connections_to;
		p_node->get_all_signal_connections(&connections_to);

		List<ConnectionWithNodePath> valid_connections_to;
		for (const Connection &c : connections_to) {
			Node *connection_target_node = Object::cast_to<Node>(c.callable.get_object());
			if (connection_target_node) {
				// TODO: add support for reinstating custom callables
				if (!c.callable.is_custom()) {
					ConnectionWithNodePath connection_to;
					connection_to.connection = c;
					connection_to.node_path = p_node->get_path_to(connection_target_node);
					valid_connections_to.push_back(connection_to);
				}
			}
		}

		// Find all valid connections from other nodes.
		List<Connection> connections_from;
		p_node->get_signals_connected_to_this(&connections_from);

		List<Connection> valid_connections_from;
		for (const Connection &c : connections_from) {
			Node *source_node = Object::cast_to<Node>(c.signal.get_object());

			Node *valid_source_owner = nullptr;
			if (source_node) {
				valid_source_owner = source_node->get_owner();
				while (valid_source_owner) {
					if (valid_source_owner == p_reimported_root) {
						break;
					}
					valid_source_owner = valid_source_owner->get_owner();
				}
			}

			if (!source_node || valid_source_owner == nullptr) {
				// TODO: add support for reinstating custom callables
				if (!c.callable.is_custom()) {
					valid_connections_from.push_back(c);
				}
			}
		}

		// Find all node groups.
		List<Node::GroupInfo> groups;
		p_node->get_groups(&groups);

		if (!modified_properties.is_empty() || !valid_connections_to.is_empty() || !valid_connections_from.is_empty() || !groups.is_empty()) {
			ModificationNodeEntry modification_node_entry;
			modification_node_entry.property_table = modified_properties;
			modification_node_entry.connections_to = valid_connections_to;
			modification_node_entry.connections_from = valid_connections_from;
			modification_node_entry.groups = groups;

			p_instance_modifications.modifications[p_reimported_root->get_path_to(p_node)] = modification_node_entry;
		}
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		get_preload_scene_modification_table(p_edited_scene, p_reimported_root, p_node->get_child(i), p_instance_modifications);
	}
}

void EditorNode::get_preload_modifications_reference_to_nodes(
		Node *p_root,
		Node *p_node,
		HashSet<Node *> &p_excluded_nodes,
		List<Node *> &p_instance_list_with_children,
		HashMap<NodePath, ModificationNodeEntry> &p_modification_table) {
	if (!p_excluded_nodes.find(p_node)) {
		HashMap<StringName, Variant> modified_properties = get_modified_properties_reference_to_nodes(p_node, p_instance_list_with_children);

		if (!modified_properties.is_empty()) {
			ModificationNodeEntry modification_node_entry;
			modification_node_entry.property_table = modified_properties;

			p_modification_table[p_root->get_path_to(p_node)] = modification_node_entry;
		}

		for (int i = 0; i < p_node->get_child_count(); i++) {
			get_preload_modifications_reference_to_nodes(p_root, p_node->get_child(i), p_excluded_nodes, p_instance_list_with_children, p_modification_table);
		}
	}
}

void EditorNode::get_children_nodes(Node *p_node, List<Node *> &p_nodes) {
	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *child = p_node->get_child(i);
		p_nodes.push_back(child);
		get_children_nodes(child, p_nodes);
	}
}

void EditorNode::replace_history_reimported_nodes(Node *p_original_root_node, Node *p_new_root_node, Node *p_node) {
	NodePath scene_path_to_node = p_original_root_node->get_path_to(p_node);
	Node *new_node = p_new_root_node->get_node_or_null(scene_path_to_node);
	if (new_node) {
		editor_history.replace_object(p_node->get_instance_id(), new_node->get_instance_id());
	} else {
		editor_history.replace_object(p_node->get_instance_id(), ObjectID());
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		replace_history_reimported_nodes(p_original_root_node, p_new_root_node, p_node->get_child(i));
	}
}

void EditorNode::_notify_nodes_scene_reimported(Node *p_node, Array p_reimported_nodes) {
	Skeleton3D *skel_3d = Object::cast_to<Skeleton3D>(p_node);
	if (skel_3d) {
		skel_3d->reset_bone_poses();
	} else {
		BoneAttachment3D *attachment = Object::cast_to<BoneAttachment3D>(p_node);
		if (attachment) {
			attachment->notify_rebind_required();
		}
	}

	if (p_node->has_method("_nodes_scene_reimported")) {
		p_node->call("_nodes_scene_reimported", p_reimported_nodes);
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		_notify_nodes_scene_reimported(p_node->get_child(i), p_reimported_nodes);
	}
}

void EditorNode::reload_scene(const String &p_path) {
	int scene_idx = -1;

	String lpath = ProjectSettings::get_singleton()->localize_path(p_path);

	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		if (editor_data.get_scene_path(i) == lpath) {
			scene_idx = i;
			break;
		}
	}

	int current_tab = editor_data.get_edited_scene();

	if (scene_idx == -1) {
		if (get_edited_scene()) {
			int current_history_id = editor_data.get_current_edited_scene_history_id();
			bool is_unsaved = EditorUndoRedoManager::get_singleton()->is_history_unsaved(current_history_id);

			// Scene is not open, so at it might be instantiated. We'll refresh the whole scene later.
			EditorUndoRedoManager::get_singleton()->clear_history(current_history_id, false);
			if (is_unsaved) {
				EditorUndoRedoManager::get_singleton()->set_history_as_unsaved(current_history_id);
			}
		}
		return;
	}

	if (current_tab == scene_idx) {
		editor_data.apply_changes_in_editors();
		_save_editor_states(p_path);
	}

	// Reload scene.
	_remove_scene(scene_idx, false);
	load_scene(p_path, true, false, true);

	// Adjust index so tab is back a the previous position.
	editor_data.move_edited_scene_to_index(scene_idx);
	EditorUndoRedoManager::get_singleton()->clear_history(editor_data.get_scene_history_id(scene_idx), false);

	// Recover the tab.
	scene_tabs->set_current_tab(current_tab);
}

void EditorNode::find_all_instances_inheriting_path_in_node(Node *p_root, Node *p_node, const String &p_instance_path, HashSet<Node *> &p_instance_list) {
	String scene_file_path = p_node->get_scene_file_path();

	bool valid_instance_found = false;

	// Attempt to find all the instances matching path we're going to reload.
	if (p_node->get_scene_file_path() == p_instance_path) {
		valid_instance_found = true;
	} else {
		Node *current_node = p_node;

		Ref<SceneState> inherited_state = current_node->get_scene_inherited_state();
		while (inherited_state.is_valid()) {
			String inherited_path = inherited_state->get_path();
			if (inherited_path == p_instance_path) {
				valid_instance_found = true;
				break;
			}

			inherited_state = inherited_state->get_base_scene_state();
		}
	}

	// Instead of adding this instance directly, if its not owned by the scene, walk its ancestors
	// and find the first node still owned by the scene. This is what we will reloading instead.
	if (valid_instance_found) {
		Node *current_node = p_node;
		while (true) {
			if (current_node->get_owner() == p_root || current_node->get_owner() == nullptr) {
				p_instance_list.insert(current_node);
				break;
			}
			current_node = current_node->get_parent();
		}
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		find_all_instances_inheriting_path_in_node(p_root, p_node->get_child(i), p_instance_path, p_instance_list);
	}
}

void EditorNode::preload_reimporting_with_path_in_edited_scenes(const List<String> &p_scenes) {
	EditorProgress progress("preload_reimporting_scene", TTR("Preparing scenes for reload"), editor_data.get_edited_scene_count());

	int original_edited_scene_idx = editor_data.get_edited_scene();

	// Walk through each opened scene to get a global list of all instances which match
	// the current reimported scenes.
	for (int current_scene_idx = 0; current_scene_idx < editor_data.get_edited_scene_count(); current_scene_idx++) {
		progress.step(vformat(TTR("Analyzing scene %s"), editor_data.get_scene_title(current_scene_idx)), current_scene_idx);

		Node *edited_scene_root = editor_data.get_edited_scene_root(current_scene_idx);

		if (edited_scene_root) {
			SceneModificationsEntry scene_modifications;

			for (const String &instance_path : p_scenes) {
				if (editor_data.get_scene_path(current_scene_idx) == instance_path) {
					continue;
				}

				HashSet<Node *> instances_to_reimport;
				find_all_instances_inheriting_path_in_node(edited_scene_root, edited_scene_root, instance_path, instances_to_reimport);
				if (instances_to_reimport.size() > 0) {
					editor_data.set_edited_scene(current_scene_idx);

					List<Node *> instance_list_with_children;
					for (Node *original_node : instances_to_reimport) {
						InstanceModificationsEntry instance_modifications;

						// Fetching all the modified properties of the nodes reimported scene.
						get_preload_scene_modification_table(edited_scene_root, original_node, original_node, instance_modifications);

						instance_modifications.original_node = original_node;
						instance_modifications.instance_path = instance_path;
						scene_modifications.instance_list.push_back(instance_modifications);

						instance_list_with_children.push_back(original_node);
						get_children_nodes(original_node, instance_list_with_children);
					}

					// Search the scene to find nodes that references the nodes will be recreated.
					get_preload_modifications_reference_to_nodes(edited_scene_root, edited_scene_root, instances_to_reimport, instance_list_with_children, scene_modifications.other_instances_modifications);
				}
			}

			if (scene_modifications.instance_list.size() > 0) {
				scenes_modification_table[current_scene_idx] = scene_modifications;
			}
		}
	}

	editor_data.set_edited_scene(original_edited_scene_idx);

	progress.step(TTR("Preparation done."), editor_data.get_edited_scene_count());
}

void EditorNode::reload_instances_with_path_in_edited_scenes() {
	if (scenes_modification_table.is_empty()) {
		return;
	}
	EditorProgress progress("reloading_scene", TTR("Scenes reloading"), editor_data.get_edited_scene_count());
	progress.step(TTR("Reloading..."), 0, true);

	Error err;
	Array replaced_nodes;
	HashMap<String, Ref<PackedScene>> local_scene_cache;

	// Reload the new instances.
	for (KeyValue<int, SceneModificationsEntry> &scene_modifications_elem : scenes_modification_table) {
		for (InstanceModificationsEntry instance_modifications : scene_modifications_elem.value.instance_list) {
			if (!local_scene_cache.has(instance_modifications.instance_path)) {
				Ref<PackedScene> instance_scene_packed_scene = ResourceLoader::load(instance_modifications.instance_path, "", ResourceFormatLoader::CACHE_MODE_REPLACE, &err);

				ERR_FAIL_COND(err != OK);
				ERR_FAIL_COND(instance_scene_packed_scene.is_null());

				local_scene_cache[instance_modifications.instance_path] = instance_scene_packed_scene;
			}
		}
	}

	// Save the current scene state/selection in case of lost.
	Dictionary editor_state = _get_main_scene_state();
	editor_data.save_edited_scene_state(editor_selection, &editor_history, editor_state);
	editor_selection->clear();

	int original_edited_scene_idx = editor_data.get_edited_scene();

	for (KeyValue<int, SceneModificationsEntry> &scene_modifications_elem : scenes_modification_table) {
		// Set the current scene.
		int current_scene_idx = scene_modifications_elem.key;
		SceneModificationsEntry *scene_modifications = &scene_modifications_elem.value;

		editor_data.set_edited_scene(current_scene_idx);
		Node *current_edited_scene = editor_data.get_edited_scene_root(current_scene_idx);

		// Make sure the node is in the tree so that editor_selection can add node smoothly.
		if (original_edited_scene_idx != current_scene_idx) {
			// Prevent scene roots with the same name from being in the tree at the same time.
			Node *original_edited_scene_root = editor_data.get_edited_scene_root(original_edited_scene_idx);
			if (original_edited_scene_root && original_edited_scene_root->get_name() == current_edited_scene->get_name()) {
				scene_root->remove_child(original_edited_scene_root);
			}
			scene_root->add_child(current_edited_scene);
		}

		// Restore the state so that the selection can be updated.
		editor_state = editor_data.restore_edited_scene_state(editor_selection, &editor_history);

		int current_history_id = editor_data.get_current_edited_scene_history_id();
		bool is_unsaved = EditorUndoRedoManager::get_singleton()->is_history_unsaved(current_history_id);

		// Clear the history for this affected tab.
		EditorUndoRedoManager::get_singleton()->clear_history(current_history_id, false);

		// Update the version
		editor_data.is_scene_changed(current_scene_idx);

		for (InstanceModificationsEntry instance_modifications : scene_modifications->instance_list) {
			Node *original_node = instance_modifications.original_node;
			String original_node_file_path = original_node->get_scene_file_path();
			Ref<PackedScene> instance_scene_packed_scene = local_scene_cache[instance_modifications.instance_path];

			// Load a replacement scene for the node.
			Ref<PackedScene> current_packed_scene;
			Ref<PackedScene> base_packed_scene;
			if (original_node_file_path == instance_modifications.instance_path) {
				// If the node file name directly matches the scene we're replacing,
				// just load it since we already cached it.
				current_packed_scene = instance_scene_packed_scene;
			} else {
				// Otherwise, check the inheritance chain, reloading and caching any scenes
				// we require along the way.
				List<String> required_load_paths;

				// Do we need to check if the paths are empty?
				if (!original_node_file_path.is_empty()) {
					required_load_paths.push_front(original_node_file_path);
				}
				Ref<SceneState> inherited_state = original_node->get_scene_inherited_state();
				while (inherited_state.is_valid()) {
					String inherited_path = inherited_state->get_path();
					// Do we need to check if the paths are empty?
					if (!inherited_path.is_empty()) {
						required_load_paths.push_front(inherited_path);
					}
					inherited_state = inherited_state->get_base_scene_state();
				}

				// Ensure the inheritance chain is loaded in the correct order so that cache can
				// be properly updated.
				for (String path : required_load_paths) {
					if (current_packed_scene.is_valid()) {
						base_packed_scene = current_packed_scene;
					}
					if (!local_scene_cache.find(path)) {
						current_packed_scene = ResourceLoader::load(path, "", ResourceFormatLoader::CACHE_MODE_REPLACE, &err);
						local_scene_cache[path] = current_packed_scene;
					} else {
						current_packed_scene = local_scene_cache[path];
					}
				}
			}

			ERR_FAIL_COND(current_packed_scene.is_null());

			// Instantiate early so that caches cleared on load in SceneState can be rebuilt early.
			Node *instantiated_node = nullptr;

			// If we are in a inherited scene, it's easier to create a new base scene and
			// grab the node from there.
			// When scene_path_to_node is '.' and we have scene_inherited_state, it's because
			// it's a multi-level inheritance scene. We should use
			NodePath scene_path_to_node = current_edited_scene->get_path_to(original_node);
			Ref<SceneState> scene_state = current_edited_scene->get_scene_inherited_state();
			if (String(scene_path_to_node) != "." && scene_state.is_valid() && scene_state->get_path() != instance_modifications.instance_path && scene_state->find_node_by_path(scene_path_to_node) >= 0) {
				Node *root_node = scene_state->instantiate(SceneState::GenEditState::GEN_EDIT_STATE_INSTANCE);
				instantiated_node = root_node->get_node(scene_path_to_node);

				if (instantiated_node) {
					if (instantiated_node->get_parent()) {
						// Remove from the root so we can delete it from memory.
						instantiated_node->get_parent()->remove_child(instantiated_node);
						// No need of the additional children that could have been added to the node
						// in the base scene. That will be managed by the 'addition_list' later.
						_remove_all_not_owned_children(instantiated_node, instantiated_node);
						memdelete(root_node);
					}
				} else {
					// Should not happen because we checked with find_node_by_path before, just in case.
					memdelete(root_node);
				}
			}

			if (!instantiated_node) {
				// If no base scene was found to create the node, we will use the reimported packed scene directly.
				// But, when the current edited scene is the reimported scene, it's because it's an inherited scene
				// derived from the reimported scene. In that case, we will not instantiate current_packed_scene, because
				// we would reinstantiate ourself. Using the base scene is better.
				if (current_edited_scene == original_node) {
					if (base_packed_scene.is_valid()) {
						instantiated_node = base_packed_scene->instantiate(PackedScene::GEN_EDIT_STATE_INSTANCE);
					} else {
						instantiated_node = instance_scene_packed_scene->instantiate(PackedScene::GEN_EDIT_STATE_INSTANCE);
					}
				} else {
					instantiated_node = current_packed_scene->instantiate(PackedScene::GEN_EDIT_STATE_INSTANCE);
				}
			}
			ERR_FAIL_NULL(instantiated_node);

			// Disconnect all relevant connections, all connections from and persistent connections to.
			for (const KeyValue<NodePath, ModificationNodeEntry> &modification_table_entry : instance_modifications.modifications) {
				for (Connection conn : modification_table_entry.value.connections_from) {
					conn.signal.get_object()->disconnect(conn.signal.get_name(), conn.callable);
				}
				for (ConnectionWithNodePath cwnp : modification_table_entry.value.connections_to) {
					Connection conn = cwnp.connection;
					if (conn.flags & CONNECT_PERSIST) {
						conn.signal.get_object()->disconnect(conn.signal.get_name(), conn.callable);
					}
				}
			}

			// Store all the paths for any selected nodes which are ancestors of the node we're replacing.
			List<NodePath> selected_node_paths;
			for (Node *selected_node : editor_selection->get_top_selected_node_list()) {
				if (selected_node == original_node || original_node->is_ancestor_of(selected_node)) {
					selected_node_paths.push_back(original_node->get_path_to(selected_node));
					editor_selection->remove_node(selected_node);
				}
			}

			// Remove all nodes which were added as additional elements (they will be restored later).
			for (AdditiveNodeEntry additive_node_entry : instance_modifications.addition_list) {
				Node *addition_node = additive_node_entry.node;
				addition_node->get_parent()->remove_child(addition_node);
			}

			// Clear ownership of the nodes (kind of hack to workaround an issue with
			// replace_by when called on nodes in other tabs).
			List<Node *> nodes_owned_by_original_node;
			original_node->get_owned_by(original_node, &nodes_owned_by_original_node);
			for (Node *owned_node : nodes_owned_by_original_node) {
				owned_node->set_owner(nullptr);
			}

			// Replace the old nodes in the history with the new ones.
			// Otherwise, the history will contain old nodes, and some could still be
			// instantiated if used elsewhere, causing the "current edited item" to be
			// linked to a node that will be destroyed later. This caused the editor to
			// crash when reimporting scenes with animations when "Editable children" was enabled.
			replace_history_reimported_nodes(original_node, instantiated_node, original_node);

			// Reset the editable instance state.
			HashMap<NodePath, SceneEditorDataEntry> scene_editor_data_table;
			Node *owner = original_node->get_owner();
			if (!owner) {
				owner = original_node;
			}

			get_scene_editor_data_for_node(owner, original_node, scene_editor_data_table);

			// The current node being reloaded may also be an additional node for another node
			// that is in the process of being reloaded.
			// Replacing the additional node with the new one prevents a crash where nodes
			// in 'addition_list' are removed from the scene tree and queued for deletion.
			for (InstanceModificationsEntry &im : scene_modifications->instance_list) {
				for (AdditiveNodeEntry &additive_node_entry : im.addition_list) {
					if (additive_node_entry.node == original_node) {
						additive_node_entry.node = instantiated_node;
					}
				}
			}

			bool original_node_scene_instance_load_placeholder = original_node->get_scene_instance_load_placeholder();

			// Delete all the remaining node children.
			while (original_node->get_child_count()) {
				Node *child = original_node->get_child(0);

				original_node->remove_child(child);
				child->queue_free();
			}

			// Update the name to match
			instantiated_node->set_name(original_node->get_name());

			// Is this replacing the edited root node?

			if (current_edited_scene == original_node) {
				// Set the instance as un inherited scene of itself.
				instantiated_node->set_scene_inherited_state(instantiated_node->get_scene_instance_state());
				instantiated_node->set_scene_instance_state(nullptr);
				instantiated_node->set_scene_file_path(original_node_file_path);
				current_edited_scene = instantiated_node;
				editor_data.set_edited_scene_root(current_edited_scene);

				if (original_edited_scene_idx == current_scene_idx) {
					// How that the editor executes a redraw while destroying or progressing the EditorProgress,
					// it crashes when the root scene has been replaced because the edited scene
					// was freed and no longer in the scene tree.
					SceneTreeDock::get_singleton()->set_edited_scene(current_edited_scene);
					if (get_tree()) {
						get_tree()->set_edited_scene_root(current_edited_scene);
					}
				}
			}

			// Replace the original node with the instantiated version.
			original_node->replace_by(instantiated_node, false);

			// Mark the old node for deletion.
			original_node->queue_free();

			// Restore the placeholder state from the original node.
			instantiated_node->set_scene_instance_load_placeholder(original_node_scene_instance_load_placeholder);

			// Attempt to re-add all the additional nodes.
			for (AdditiveNodeEntry additive_node_entry : instance_modifications.addition_list) {
				Node *parent_node = instantiated_node->get_node_or_null(additive_node_entry.parent);

				if (!parent_node) {
					parent_node = current_edited_scene;
				}

				parent_node->add_child(additive_node_entry.node);
				parent_node->move_child(additive_node_entry.node, additive_node_entry.index);
				// If the additive node's owner was the node which got replaced, update it.
				if (additive_node_entry.owner == original_node) {
					additive_node_entry.owner = instantiated_node;
				}

				additive_node_entry.node->set_owner(additive_node_entry.owner);

				// If the parent node was lost, attempt to restore the original global transform.
				{
					Node2D *node_2d = Object::cast_to<Node2D>(additive_node_entry.node);
					if (node_2d) {
						node_2d->set_transform(additive_node_entry.transform_2d);
					}

					Node3D *node_3d = Object::cast_to<Node3D>(additive_node_entry.node);
					if (node_3d) {
						node_3d->set_transform(additive_node_entry.transform_3d);
					}
				}
			}

			// Restore the scene's editable instance and folded states.
			for (HashMap<NodePath, SceneEditorDataEntry>::Iterator I = scene_editor_data_table.begin(); I; ++I) {
				Node *node = owner->get_node_or_null(I->key);
				if (node) {
					if (owner != node) {
						owner->set_editable_instance(node, I->value.is_editable);
					}
					node->set_display_folded(I->value.is_display_folded);
				}
			}

			// Restore the selection.
			if (selected_node_paths.size()) {
				for (NodePath selected_node_path : selected_node_paths) {
					Node *selected_node = instantiated_node->get_node_or_null(selected_node_path);
					if (selected_node) {
						editor_selection->add_node(selected_node);
					}
				}
				editor_selection->update();
			}

			// Attempt to restore the modified properties and signals for the instantitated node and all its owned children.
			for (KeyValue<NodePath, ModificationNodeEntry> &E : instance_modifications.modifications) {
				NodePath new_current_path = E.key;
				Node *modifiable_node = instantiated_node->get_node_or_null(new_current_path);

				update_node_from_node_modification_entry(modifiable_node, E.value);
			}
			// Add the newly instantiated node to the edited scene's replaced node list.
			replaced_nodes.push_back(instantiated_node);
		}

		// Attempt to restore the modified properties and signals for the instantitated node and all its owned children.
		for (KeyValue<NodePath, ModificationNodeEntry> &E : scene_modifications->other_instances_modifications) {
			NodePath new_current_path = E.key;
			Node *modifiable_node = current_edited_scene->get_node_or_null(new_current_path);

			if (modifiable_node) {
				update_node_from_node_modification_entry(modifiable_node, E.value);
			}
		}

		if (is_unsaved) {
			EditorUndoRedoManager::get_singleton()->set_history_as_unsaved(current_history_id);
		}

		// Save the current handled scene state.
		editor_data.save_edited_scene_state(editor_selection, &editor_history, editor_state);
		editor_selection->clear();

		// Cleanup the history of the changes.
		editor_history.cleanup_history();

		if (original_edited_scene_idx != current_scene_idx) {
			scene_root->remove_child(current_edited_scene);

			// Ensure the current edited scene is re-added if removed earlier because it has the same name
			// as the reimported scene. The editor could crash when reloading SceneTreeDock if the current
			// edited scene is not in the scene tree.
			Node *original_edited_scene_root = editor_data.get_edited_scene_root(original_edited_scene_idx);
			if (original_edited_scene_root && !original_edited_scene_root->get_parent()) {
				scene_root->add_child(original_edited_scene_root);
			}
		}
	}

	// For the whole editor, call the _notify_nodes_scene_reimported with a list of replaced nodes.
	// To inform anything that depends on them that they should update as appropriate.
	_notify_nodes_scene_reimported(this, replaced_nodes);

	editor_data.set_edited_scene(original_edited_scene_idx);

	editor_data.restore_edited_scene_state(editor_selection, &editor_history);

	progress.step(TTR("Reloading done."), editor_data.get_edited_scene_count());
}

void EditorNode::_remove_all_not_owned_children(Node *p_node, Node *p_owner) {
	Vector<Node *> nodes_to_remove;
	if (p_node != p_owner && p_node->get_owner() != p_owner) {
		nodes_to_remove.push_back(p_node);
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *child_node = p_node->get_child(i);
		_remove_all_not_owned_children(child_node, p_owner);
	}

	for (Node *node : nodes_to_remove) {
		node->get_parent()->remove_child(node);
		node->queue_free();
	}
}

