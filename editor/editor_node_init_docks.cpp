/**************************************************************************/
/*  editor_node_init_docks.cpp                                           */
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

#include "editor/docks/filesystem_dock.h"
#include "editor/docks/groups_dock.h"
#include "editor/docks/history_dock.h"
#include "editor/docks/import_dock.h"
#include "editor/docks/inspector_dock.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/docks/signals_dock.h"
#include "editor/settings/project_settings_editor.h"

void EditorNode::_init_docks() {
	// Instantiate and place editor docks.

	memnew(SceneTreeDock(scene_root, editor_selection, editor_data));
	editor_dock_manager->add_dock(SceneTreeDock::get_singleton());

	memnew(ImportDock);
	editor_dock_manager->add_dock(ImportDock::get_singleton());

	FileSystemDock *filesystem_dock = memnew(FileSystemDock);
	filesystem_dock->connect("inherit", callable_mp(this, &EditorNode::_inherit_request));
	filesystem_dock->connect("instantiate", callable_mp(this, &EditorNode::_instantiate_request));
	filesystem_dock->connect("display_mode_changed", callable_mp(this, &EditorNode::_save_editor_layout));
	get_project_settings()->connect_filesystem_dock_signals(filesystem_dock);
	editor_dock_manager->add_dock(filesystem_dock);

	memnew(InspectorDock(editor_data));
	editor_dock_manager->add_dock(InspectorDock::get_singleton());

	memnew(SignalsDock);
	editor_dock_manager->add_dock(SignalsDock::get_singleton());

	memnew(GroupsDock);
	editor_dock_manager->add_dock(GroupsDock::get_singleton());

	history_dock = memnew(HistoryDock);
	editor_dock_manager->add_dock(history_dock);
}

