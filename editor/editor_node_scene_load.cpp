/**************************************************************************/
/*  editor_node_scene_load.cpp                                            */
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
#include "core/io/config_file.h"
#include "core/io/file_access.h"
#include "core/io/resource.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_uid.h"
#include "core/os/os.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/docks/inspector_dock.h"
#include "editor/file_system/dependency_editor.h"
#include "editor/scene/editor_scene_tabs.h"
#include "editor/file_system/editor_paths.h"
#include "editor/settings/editor_settings.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/script/script_editor_plugin.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/packed_scene.h"

Error EditorNode::load_resource(const String &p_resource, bool p_ignore_broken_deps) {
	dependency_errors.clear();

	Error err;

	Ref<Resource> res;
	if (force_textfile_extensions.has(p_resource.get_extension())) {
		res = ResourceCache::get_ref(p_resource);
		if (res.is_null() || !res->is_class("TextFile")) {
			res = ScriptEditor::get_singleton()->open_file(p_resource);
		}
	} else if (ResourceLoader::exists(p_resource, "")) {
		res = ResourceLoader::load(p_resource, "", ResourceFormatLoader::CACHE_MODE_REUSE, &err);
	} else if (textfile_extensions.has(p_resource.get_extension())) {
		res = ScriptEditor::get_singleton()->open_file(p_resource);
	} else if (other_file_extensions.has(p_resource.get_extension())) {
		OS::get_singleton()->shell_open(ProjectSettings::get_singleton()->globalize_path(p_resource));
		return OK;
	}
	ERR_FAIL_COND_V(res.is_null(), ERR_CANT_OPEN);

	if (!p_ignore_broken_deps && !dependency_errors.is_empty()) {
		dependency_error->show(p_resource, dependency_errors);
		dependency_errors.clear();

		return ERR_FILE_MISSING_DEPENDENCIES;
	}

	InspectorDock::get_singleton()->edit_resource(res);
	return OK;
}

Error EditorNode::load_scene_or_resource(const String &p_path, bool p_ignore_broken_deps, bool p_change_scene_tab_if_already_open) {
	if (ClassDB::is_parent_class(ResourceLoader::get_resource_type(p_path), "PackedScene")) {
		if (!p_change_scene_tab_if_already_open && EditorNode::get_singleton()->is_scene_open(p_path)) {
			return OK;
		}
		return EditorNode::get_singleton()->load_scene(p_path, p_ignore_broken_deps);
	}
	return EditorNode::get_singleton()->load_resource(p_path, p_ignore_broken_deps);
}

Error EditorNode::load_scene(const String &p_scene, bool p_ignore_broken_deps, bool p_set_inherited, bool p_force_open_imported, bool p_silent_change_tab) {
	if (!is_inside_tree()) {
		defer_load_scene = p_scene;
		return OK;
	}

	String lpath = ProjectSettings::get_singleton()->localize_path(ResourceUID::ensure_path(p_scene));
	_update_prev_closed_scenes(lpath, false);

	if (!p_set_inherited) {
		for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
			if (editor_data.get_scene_path(i) == lpath) {
				_set_current_scene(i);
				return OK;
			}
		}

		if (!p_force_open_imported && FileAccess::exists(lpath + ".import")) {
			open_imported->set_text(vformat(TTR("Scene '%s' was automatically imported, so it can't be modified.\nTo make changes to it, a new inherited scene can be created."), lpath.get_file()));
			open_imported->popup_centered();
			new_inherited_button->grab_focus();
			open_import_request = lpath;
			return OK;
		}
	}

	if (!lpath.begins_with("res://")) {
		show_accept(TTR("Error loading scene, it must be inside the project path. Use 'Import' to open the scene, then save it inside the project path."), TTR("OK"));
		return ERR_FILE_NOT_FOUND;
	}

	int prev = editor_data.get_edited_scene();
	int idx = prev;

	if (prev == -1 || editor_data.get_edited_scene_root() || !editor_data.get_scene_path(prev).is_empty()) {
		idx = editor_data.add_edited_scene(-1);

		if (p_silent_change_tab) {
			_set_current_scene_nocheck(idx);
		} else {
			_set_current_scene(idx);
		}
	} else {
		EditorUndoRedoManager::get_singleton()->clear_history(editor_data.get_current_edited_scene_history_id(), false);

		Dictionary state = editor_data.restore_edited_scene_state(editor_selection, &editor_history);
		callable_mp(this, &EditorNode::_set_main_scene_state).call_deferred(state, get_edited_scene()); // Do after everything else is done setting up.
	}

	dependency_errors.clear();

	Error err;
	Ref<PackedScene> sdata = ResourceLoader::load(lpath, "", ResourceFormatLoader::CACHE_MODE_REPLACE, &err);

	if (!p_ignore_broken_deps && !dependency_errors.is_empty()) {
		current_menu_option = -1;
		dependency_error->show(lpath, dependency_errors);
		dependency_errors.clear();

		if (prev != -1 && prev != idx) {
			_set_current_scene(prev);
			editor_data.remove_scene(idx);
		}
		return ERR_FILE_MISSING_DEPENDENCIES;
	}

	if (sdata.is_null()) {
		_dialog_display_load_error(lpath, err);

		if (prev != -1 && prev != idx) {
			_set_current_scene(prev);
			editor_data.remove_scene(idx);
		}
		return ERR_FILE_NOT_FOUND;
	}

	dependency_errors.erase(lpath); // At least not self path.

	for (KeyValue<String, HashSet<String>> &E : dependency_errors) {
		String txt = vformat(TTR("Scene '%s' has broken dependencies:"), E.key) + "\n";
		for (const String &F : E.value) {
			txt += "\t" + F + "\n";
		}
		add_io_error(txt);
	}

	if (ResourceCache::has(lpath)) {
		// Used from somewhere else? No problem! Update state and replace sdata.
		Ref<PackedScene> ps = ResourceCache::get_ref(lpath);
		if (ps.is_valid()) {
			ps->replace_state(sdata->get_state());
			ps->set_last_modified_time(sdata->get_last_modified_time());
			sdata = ps;
		}

	} else {
		sdata->set_path(lpath, true); // Take over path.
	}

	Node *new_scene = sdata->instantiate(p_set_inherited ? PackedScene::GEN_EDIT_STATE_MAIN_INHERITED : PackedScene::GEN_EDIT_STATE_MAIN);
	if (!new_scene) {
		sdata.unref();
		_dialog_display_load_error(lpath, ERR_FILE_CORRUPT);
		if (prev != -1 && prev != idx) {
			_set_current_scene(prev);
			editor_data.remove_scene(idx);
		}
		return ERR_FILE_CORRUPT;
	}

	if (p_set_inherited) {
		Ref<SceneState> state = sdata->get_state();
		state->set_path(lpath);
		new_scene->set_scene_inherited_state(state);
		new_scene->set_scene_file_path(String());
	}

	new_scene->set_scene_instance_state(Ref<SceneState>());

	set_edited_scene(new_scene);
	// When editor plugins load in, they might use node transforms during their own setup, so make sure they're up to date.
	get_tree()->flush_transform_notifications();

	String config_file_path = EditorPaths::get_singleton()->get_project_settings_dir().path_join(lpath.get_file() + "-editstate-" + lpath.md5_text() + ".cfg");
	Ref<ConfigFile> editor_state_cf;
	editor_state_cf.instantiate();
	Error editor_state_cf_err = editor_state_cf->load(config_file_path);
	if (editor_state_cf_err == OK || editor_state_cf->has_section("editor_states")) {
		_load_editor_plugin_states_from_config(editor_state_cf);
	}

	if (editor_folding.has_folding_data(lpath)) {
		editor_folding.load_scene_folding(new_scene, lpath);
	} else if (EDITOR_GET("interface/inspector/auto_unfold_foreign_scenes")) {
		editor_folding.unfold_scene(new_scene);
		editor_folding.save_scene_folding(new_scene, lpath);
	}

	EditorDebuggerNode::get_singleton()->update_live_edit_root();

	if (restoring_scenes) {
		// Initialize history for restored scenes.
		ObjectID id = new_scene->get_instance_id();
		if (id != editor_history.get_current()) {
			editor_history.add_object(id);
		}
	}

	// Load the selected nodes.
	if (editor_state_cf->has_section_key("editor_states", "selected_nodes")) {
		TypedArray<NodePath> selected_node_list = editor_state_cf->get_value("editor_states", "selected_nodes", TypedArray<String>());

		for (int i = 0; i < selected_node_list.size(); i++) {
			Node *selected_node = new_scene->get_node_or_null(selected_node_list[i]);
			if (selected_node) {
				editor_selection->add_node(selected_node);
			}
		}
	}

	if (!restoring_scenes) {
		save_editor_layout_delayed();
	}

	if (p_set_inherited) {
		EditorUndoRedoManager::get_singleton()->set_history_as_unsaved(editor_data.get_current_edited_scene_history_id());
	}

	_update_title();
	scene_tabs->update_scene_tabs();
	if (!restoring_scenes) {
		_add_to_recent_scenes(lpath);
	}

	return OK;
}

