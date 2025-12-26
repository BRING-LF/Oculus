/**************************************************************************/
/*  editor_node_scene_helpers.cpp                                         */
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

#include "editor/editor_undo_redo_manager.h"
#include "scene/main/node.h"

int EditorNode::_next_unsaved_scene(bool p_valid_filename, int p_start) {
	for (int i = p_start; i < editor_data.get_edited_scene_count(); i++) {
		if (!editor_data.get_edited_scene_root(i)) {
			continue;
		}

		String scene_filename = editor_data.get_edited_scene_root(i)->get_scene_file_path();
		if (p_valid_filename && scene_filename.is_empty()) {
			continue;
		}

		bool unsaved = EditorUndoRedoManager::get_singleton()->is_history_unsaved(editor_data.get_scene_history_id(i));
		if (unsaved) {
			return i;
		} else {
			for (int j = 0; j < editor_data.get_editor_plugin_count(); j++) {
				if (!editor_data.get_editor_plugin(j)->get_unsaved_status(scene_filename).is_empty()) {
					return i;
				}
			}
		}
	}
	return -1;
}

bool EditorNode::has_previous_closed_scenes() const {
	return !prev_closed_scenes.is_empty();
}

void EditorNode::_inherit_request(String p_file) {
	current_menu_option = SCENE_NEW_INHERITED_SCENE;
	_dialog_action(p_file);
}

void EditorNode::_instantiate_request(const Vector<String> &p_files) {
	request_instantiate_scenes(p_files);
}

void EditorNode::_close_messages() {
	old_split_ofs = center_split->get_split_offset();
	center_split->set_split_offset(0);
}

void EditorNode::_show_messages() {
	center_split->set_split_offset(old_split_ofs);
}

void EditorNode::_quick_opened(const String &p_file_path) {
	load_scene_or_resource(p_file_path);
}

bool EditorNode::_find_scene_in_use(Node *p_node, const String &p_path) const {
	if (p_node->get_scene_file_path() == p_path) {
		return true;
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		if (_find_scene_in_use(p_node->get_child(i), p_path)) {
			return true;
		}
	}

	return false;
}

bool EditorNode::close_scene() {
	int tab_index = editor_data.get_edited_scene();
	if (tab_index == 0 && get_edited_scene() == nullptr && editor_data.get_scene_path(tab_index).is_empty()) {
		return false;
	}

	tab_closing_idx = tab_index;
	current_menu_option = SCENE_CLOSE;
	_discard_changes();
	changing_scene = false;
	return true;
}

bool EditorNode::is_scene_in_use(const String &p_path) {
	Node *es = get_edited_scene();
	if (es) {
		return _find_scene_in_use(es, p_path);
	}
	return false;
}

