/**************************************************************************/
/*  editor_node_init_editor_shortcuts.cpp                                */
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

#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"

void EditorNode::_init_editor_shortcuts() {
	ED_SHORTCUT("editor/next_tab", TTRC("Next Scene Tab"), KeyModifierMask::CTRL + Key::TAB);
	ED_SHORTCUT("editor/prev_tab", TTRC("Previous Scene Tab"), KeyModifierMask::CTRL + KeyModifierMask::SHIFT + Key::TAB);
	ED_SHORTCUT("editor/filter_files", TTRC("Focus FileSystem Filter"), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::ALT + Key::P);

	ED_SHORTCUT_AND_COMMAND("editor/new_scene", TTRC("New Scene"), KeyModifierMask::CMD_OR_CTRL + Key::N);
	ED_SHORTCUT_AND_COMMAND("editor/new_inherited_scene", TTRC("New Inherited Scene..."), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + Key::N);
	ED_SHORTCUT_AND_COMMAND("editor/open_scene", TTRC("Open Scene..."), KeyModifierMask::CMD_OR_CTRL + Key::O);
	ED_SHORTCUT_AND_COMMAND("editor/reopen_closed_scene", TTRC("Reopen Closed Scene"), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + Key::T);

	ED_SHORTCUT_AND_COMMAND("editor/save_scene", TTRC("Save Scene"), KeyModifierMask::CMD_OR_CTRL + Key::S);
	ED_SHORTCUT_AND_COMMAND("editor/save_scene_as", TTRC("Save Scene As..."), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + Key::S);
	ED_SHORTCUT_AND_COMMAND("editor/save_all_scenes", TTRC("Save All Scenes"), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + KeyModifierMask::ALT + Key::S);

	ED_SHORTCUT_ARRAY_AND_COMMAND("editor/quick_open", TTRC("Quick Open..."), { int32_t(KeyModifierMask::SHIFT + KeyModifierMask::ALT + Key::O), int32_t(KeyModifierMask::CMD_OR_CTRL + Key::P) });
	ED_SHORTCUT_OVERRIDE_ARRAY("editor/quick_open", "macos", { int32_t(KeyModifierMask::META + KeyModifierMask::CTRL + Key::O), int32_t(KeyModifierMask::CMD_OR_CTRL + Key::P) });
	ED_SHORTCUT_AND_COMMAND("editor/quick_open_scene", TTRC("Quick Open Scene..."), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + Key::O);
	ED_SHORTCUT_AND_COMMAND("editor/quick_open_script", TTRC("Quick Open Script..."), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::ALT + Key::O);

	ED_SHORTCUT("editor/export_as_mesh_library", TTRC("MeshLibrary..."));

	ED_SHORTCUT_AND_COMMAND("editor/reload_saved_scene", TTRC("Reload Saved Scene"));
	ED_SHORTCUT_AND_COMMAND("editor/close_scene", TTRC("Close Scene"), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + Key::W);
	ED_SHORTCUT_AND_COMMAND("editor/close_all_scenes", TTRC("Close All Scenes"));
	ED_SHORTCUT_OVERRIDE("editor/close_scene", "macos", KeyModifierMask::CMD_OR_CTRL + Key::W);

	ED_SHORTCUT_AND_COMMAND("editor/editor_settings", TTRC("Editor Settings..."));
	ED_SHORTCUT_OVERRIDE("editor/editor_settings", "macos", KeyModifierMask::META + Key::COMMA);

	ED_SHORTCUT_AND_COMMAND("editor/file_quit", TTRC("Quit"), KeyModifierMask::CMD_OR_CTRL + Key::Q);

	ED_SHORTCUT_AND_COMMAND("editor/project_settings", TTRC("Project Settings..."), Key::NONE, TTRC("Project Settings"));
	ED_SHORTCUT_AND_COMMAND("editor/find_in_files", TTRC("Find in Files..."), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::F);

	ED_SHORTCUT_AND_COMMAND("editor/export", TTRC("Export..."), Key::NONE, TTRC("Export"));

	ED_SHORTCUT_AND_COMMAND("editor/orphan_resource_explorer", TTRC("Orphan Resource Explorer..."));
	ED_SHORTCUT_AND_COMMAND("editor/engine_compilation_configuration_editor", TTRC("Engine Compilation Configuration Editor..."));
	ED_SHORTCUT_AND_COMMAND("editor/upgrade_project", TTRC("Upgrade Project Files..."));

	ED_SHORTCUT("editor/reload_current_project", TTRC("Reload Current Project"));
	ED_SHORTCUT_AND_COMMAND("editor/quit_to_project_list", TTRC("Quit to Project List"), KeyModifierMask::CTRL + KeyModifierMask::SHIFT + Key::Q);
	ED_SHORTCUT_OVERRIDE("editor/quit_to_project_list", "macos", KeyModifierMask::META + KeyModifierMask::CTRL + KeyModifierMask::ALT + Key::Q);

	ED_SHORTCUT("editor/command_palette", TTRC("Command Palette..."), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::P);

	ED_SHORTCUT_AND_COMMAND("editor/take_screenshot", TTRC("Take Screenshot"), KeyModifierMask::CTRL | Key::F12);
	ED_SHORTCUT_OVERRIDE("editor/take_screenshot", "macos", KeyModifierMask::META | Key::F12);

	ED_SHORTCUT_AND_COMMAND("editor/fullscreen_mode", TTRC("Toggle Fullscreen"), KeyModifierMask::SHIFT | Key::F11);
	ED_SHORTCUT_OVERRIDE("editor/fullscreen_mode", "macos", KeyModifierMask::META | KeyModifierMask::CTRL | Key::F);

	ED_SHORTCUT_AND_COMMAND("editor/editor_help", TTRC("Search Help..."), Key::F1);
	ED_SHORTCUT_OVERRIDE("editor/editor_help", "macos", KeyModifierMask::ALT | Key::SPACE);
	ED_SHORTCUT_AND_COMMAND("editor/online_docs", TTRC("Online Documentation"));
	ED_SHORTCUT_AND_COMMAND("editor/forum", TTRC("Forum"));
	ED_SHORTCUT_AND_COMMAND("editor/community", TTRC("Community"));

	ED_SHORTCUT_AND_COMMAND("editor/copy_system_info", TTRC("Copy System Info"));
	ED_SHORTCUT_AND_COMMAND("editor/report_a_bug", TTRC("Report a Bug"));
	ED_SHORTCUT_AND_COMMAND("editor/suggest_a_feature", TTRC("Suggest a Feature"));
	ED_SHORTCUT_AND_COMMAND("editor/send_docs_feedback", TTRC("Send Docs Feedback"));
	ED_SHORTCUT_AND_COMMAND("editor/about", TTRC("About Godot..."));
	ED_SHORTCUT_AND_COMMAND("editor/support_development", TTRC("Support Godot Development"));

	// Use the Ctrl modifier so F2 can be used to rename nodes in the scene tree dock.
	ED_SHORTCUT_AND_COMMAND("editor/editor_2d", TTRC("Open 2D Workspace"), KeyModifierMask::CTRL | Key::F1);
	ED_SHORTCUT_AND_COMMAND("editor/editor_3d", TTRC("Open 3D Workspace"), KeyModifierMask::CTRL | Key::F2);
	ED_SHORTCUT_AND_COMMAND("editor/editor_script", TTRC("Open Script Editor"), KeyModifierMask::CTRL | Key::F3);
	ED_SHORTCUT_AND_COMMAND("editor/editor_game", TTRC("Open Game View"), KeyModifierMask::CTRL | Key::F4);
	ED_SHORTCUT_AND_COMMAND("editor/editor_assetlib", TTRC("Open Asset Library"), KeyModifierMask::CTRL | Key::F5);

	ED_SHORTCUT_OVERRIDE("editor/editor_2d", "macos", KeyModifierMask::META | KeyModifierMask::CTRL | Key::KEY_1);
	ED_SHORTCUT_OVERRIDE("editor/editor_3d", "macos", KeyModifierMask::META | KeyModifierMask::CTRL | Key::KEY_2);
	ED_SHORTCUT_OVERRIDE("editor/editor_script", "macos", KeyModifierMask::META | KeyModifierMask::CTRL | Key::KEY_3);
	ED_SHORTCUT_OVERRIDE("editor/editor_game", "macos", KeyModifierMask::META | KeyModifierMask::CTRL | Key::KEY_4);
	ED_SHORTCUT_OVERRIDE("editor/editor_assetlib", "macos", KeyModifierMask::META | KeyModifierMask::CTRL | Key::KEY_5);

	ED_SHORTCUT_AND_COMMAND("editor/editor_next", TTRC("Open the next Editor"));
	ED_SHORTCUT_AND_COMMAND("editor/editor_prev", TTRC("Open the previous Editor"));
}

