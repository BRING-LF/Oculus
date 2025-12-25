/**************************************************************************/
/*  editor_node_editor_states.cpp                                        */
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

#include "core/io/config_file.h"
#include "editor/file_system/editor_paths.h"
#include "scene/main/node.h"

void EditorNode::_load_editor_plugin_states_from_config(const Ref<ConfigFile> &p_config_file) {
	Node *scene = editor_data.get_edited_scene_root();

	if (!scene) {
		return;
	}

	Vector<String> esl = p_config_file->get_section_keys("editor_states");

	Dictionary md;
	for (const String &E : esl) {
		Variant st = p_config_file->get_value("editor_states", E);
		if (st.get_type() != Variant::NIL) {
			md[E] = st;
		}
	}

	editor_data.set_editor_plugin_states(md);
}

void EditorNode::_save_editor_states(const String &p_file, int p_idx) {
	Node *scene = editor_data.get_edited_scene_root(p_idx);

	if (!scene) {
		return;
	}

	String path = EditorPaths::get_singleton()->get_project_settings_dir().path_join(p_file.get_file() + "-editstate-" + p_file.md5_text() + ".cfg");

	Ref<ConfigFile> cf;
	cf.instantiate();

	Dictionary md;
	if (p_idx < 0 || editor_data.get_edited_scene() == p_idx) {
		md = editor_data.get_editor_plugin_states();
	} else {
		md = editor_data.get_scene_editor_states(p_idx);
	}

	for (const KeyValue<Variant, Variant> &kv : md) {
		cf->set_value("editor_states", kv.key, kv.value);
	}

	// Save the currently selected nodes.

	List<Node *> selection = editor_selection->get_full_selected_node_list();
	TypedArray<NodePath> selection_paths;
	for (Node *selected_node : selection) {
		selection_paths.push_back(selected_node->get_path());
	}
	cf->set_value("editor_states", "selected_nodes", selection_paths);

	Error err = cf->save(path);
	ERR_FAIL_COND_MSG(err != OK, "Cannot save config file to '" + path + "'.");
}

