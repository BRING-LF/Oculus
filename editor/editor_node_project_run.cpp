/**************************************************************************/
/*  editor_node_project_run.cpp                                          */
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

#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_log.h"
#include "editor/gui/editor_bottom_panel.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/run/editor_run_bar.h"
#include "editor/settings/editor_settings.h"
#include "main/main.h"
#include "scene/main/scene_tree.h"

void EditorNode::_project_run_started() {
	if (bool(EDITOR_GET("run/output/always_clear_output_on_play"))) {
		log->clear();
	}

	int action_on_play = EDITOR_GET("run/bottom_panel/action_on_play");
	if (action_on_play == ACTION_ON_PLAY_OPEN_OUTPUT) {
		editor_dock_manager->focus_dock(log);
	} else if (action_on_play == ACTION_ON_PLAY_OPEN_DEBUGGER) {
		editor_dock_manager->focus_dock(EditorDebuggerNode::get_singleton());
	}
}

void EditorNode::_project_run_stopped() {
	int action_on_stop = EDITOR_GET("run/bottom_panel/action_on_stop");
	if (action_on_stop == ACTION_ON_STOP_CLOSE_BUTTOM_PANEL) {
		bottom_panel->hide_bottom_panel();
	}
}

void EditorNode::notify_all_debug_sessions_exited() {
	project_run_bar->stop_playing();
}

OS::ProcessID EditorNode::has_child_process(OS::ProcessID p_pid) const {
	return project_run_bar->has_child_process(p_pid);
}

void EditorNode::stop_child_process(OS::ProcessID p_pid) {
	project_run_bar->stop_child_process(p_pid);
}

void EditorNode::_exit_editor(int p_exit_code) {
	exiting = true;
	waiting_for_first_scan = false;
	resource_preview->stop(); // Stop early to avoid crashes.
	_save_editor_layout();

	// Dim the editor window while it's quitting to make it clearer that it's busy.
	dim_editor(true);

	// Unload addons before quitting to allow cleanup.
	unload_editor_addons();

	get_tree()->quit(p_exit_code);
}

void EditorNode::unload_editor_addons() {
	for (const KeyValue<String, EditorPlugin *> &E : addon_name_to_plugin) {
		print_verbose(vformat("Unloading addon: %s", E.key));
		remove_editor_plugin(E.value, false);
		memdelete(E.value);
	}

	addon_name_to_plugin.clear();
}

void EditorNode::restart_editor(bool p_goto_project_manager) {
	_menu_option_confirm(p_goto_project_manager ? PROJECT_QUIT_TO_PROJECT_MANAGER : PROJECT_RELOAD_CURRENT_PROJECT, false);
}

void EditorNode::_restart_editor(bool p_goto_project_manager) {
	exiting = true;

	if (project_run_bar->is_playing()) {
		project_run_bar->stop_playing();
	}

	String to_reopen;
	if (!p_goto_project_manager && get_tree()->get_edited_scene_root()) {
		to_reopen = get_tree()->get_edited_scene_root()->get_scene_file_path();
	}

	_exit_editor(EXIT_SUCCESS);

	List<String> args;
	for (const String &a : Main::get_forwardable_cli_arguments(Main::CLI_SCOPE_TOOL)) {
		args.push_back(a);
	}

	if (p_goto_project_manager) {
		args.push_back("--project-manager");

		// Setup working directory.
		const String exec_dir = OS::get_singleton()->get_executable_path().get_base_dir();
		if (!exec_dir.is_empty()) {
			args.push_back("--path");
			args.push_back(exec_dir);
		}
	} else {
		args.push_back("--path");
		args.push_back(ProjectSettings::get_singleton()->get_resource_path());

		args.push_back("-e");
	}

	if (!to_reopen.is_empty()) {
		args.push_back(to_reopen);
	}

	OS::get_singleton()->set_restart_on_exit(true, args);
}

void EditorNode::_cancel_confirmation() {
	stop_project_confirmation = false;
}

