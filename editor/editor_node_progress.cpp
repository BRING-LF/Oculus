/**************************************************************************/
/*  editor_node_progress.cpp                                              */
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

#include "core/os/os.h"
#include "core/string/print_string.h"
#include "editor/editor_interface.h"
#include "editor/gui/progress_dialog.h"
#include "scene/gui/dialogs.h"

// Used to track the progress of tasks in the CLI output (since we don't have any other frame of reference).
static HashMap<String, int> progress_total_steps;

static String last_progress_task;
static String last_progress_state;
static int last_progress_step = 0;
static double last_progress_time = 0;

void EditorNode::progress_add_task(const String &p_task, const String &p_label, int p_steps, bool p_can_cancel) {
	if (!singleton) {
		return;
	} else if (singleton->cmdline_mode) {
		print_line_rich(vformat("[   0%% ] [color=gray][b]%s[/b] | Started %s (%d steps)[/color]", p_task, p_label, p_steps));
		progress_total_steps[p_task] = p_steps;
	} else if (singleton->progress_dialog) {
		singleton->progress_dialog->add_task(p_task, p_label, p_steps, p_can_cancel);
	}
}

bool EditorNode::progress_task_step(const String &p_task, const String &p_state, int p_step, bool p_force_refresh) {
	if (!singleton) {
		return false;
	} else if (singleton->cmdline_mode) {
		double current_time = USEC_TO_SEC(OS::get_singleton()->get_ticks_usec());
		double elapsed_time = current_time - last_progress_time;
		if (p_task != last_progress_task || p_state != last_progress_state || p_step != last_progress_step || elapsed_time >= 1.0) {
			// Only print the progress if it's changed since the last print, or if one second has passed.
			// This prevents multithreaded import from printing the same progress too often, which would bloat the log file.
			const int percent = (p_step / float(progress_total_steps[p_task] + 1)) * 100;
			print_line_rich(vformat("[%4d%% ] [color=gray][b]%s[/b] | %s[/color]", percent, p_task, p_state));
			last_progress_task = p_task;
			last_progress_state = p_state;
			last_progress_step = p_step;
			last_progress_time = current_time;
		}
		return false;
	} else if (singleton->progress_dialog) {
		return singleton->progress_dialog->task_step(p_task, p_state, p_step, p_force_refresh);
	} else {
		return false;
	}
}

void EditorNode::progress_end_task(const String &p_task) {
	if (!singleton) {
		return;
	} else if (singleton->cmdline_mode) {
		progress_total_steps.erase(p_task);
		print_line_rich(vformat("[color=green][ DONE ][/color] [b]%s[/b]\n", p_task));
	} else if (singleton->progress_dialog) {
		singleton->progress_dialog->end_task(p_task);
	}
}

void EditorNode::progress_add_task_bg(const String &p_task, const String &p_label, int p_steps) {
	singleton->progress_hb->add_task(p_task, p_label, p_steps);
}

void EditorNode::progress_task_step_bg(const String &p_task, int p_step) {
	singleton->progress_hb->task_step(p_task, p_step);
}

void EditorNode::progress_end_task_bg(const String &p_task) {
	singleton->progress_hb->end_task(p_task);
}

void EditorNode::_progress_dialog_visibility_changed() {
	// Open the io errors after the progress dialog is closed.
	if (load_errors_queued_to_display && !progress_dialog->is_visible()) {
		EditorInterface::get_singleton()->popup_dialog_centered_ratio(singleton->load_error_dialog, 0.5);
		load_errors_queued_to_display = false;
	}
}

