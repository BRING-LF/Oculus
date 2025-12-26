/**************************************************************************/
/*  editor_node_execute.cpp                                               */
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
#include "editor/editor_interface.h"
#include "main/main.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/rich_text_label.h"
#include "servers/display/display_server.h"

static void _execute_thread(void *p_ud) {
	EditorNode::ExecuteThreadArgs *eta = (EditorNode::ExecuteThreadArgs *)p_ud;
	Error err = OS::get_singleton()->execute(eta->path, eta->args, &eta->output, &eta->exitcode, true, &eta->execute_output_mutex);
	print_verbose("Thread exit status: " + itos(eta->exitcode));
	if (err != OK) {
		eta->exitcode = err;
	}

	eta->done.set();
}

int EditorNode::execute_and_show_output(const String &p_title, const String &p_path, const List<String> &p_arguments, bool p_close_on_ok, bool p_close_on_errors, String *r_output) {
	if (execute_output_dialog) {
		execute_output_dialog->set_title(p_title);
		execute_output_dialog->get_ok_button()->set_disabled(true);
		execute_outputs->clear();
		execute_outputs->set_scroll_follow(true);
		EditorInterface::get_singleton()->popup_dialog_centered_ratio(execute_output_dialog);
	}

	ExecuteThreadArgs eta;
	eta.path = p_path;
	eta.args = p_arguments;
	eta.exitcode = 255;

	int prev_len = 0;

	eta.execute_output_thread.start(_execute_thread, &eta);

	while (!eta.done.is_set()) {
		{
			MutexLock lock(eta.execute_output_mutex);
			if (prev_len != eta.output.length()) {
				String to_add = eta.output.substr(prev_len);
				prev_len = eta.output.length();
				execute_outputs->add_text(to_add);
				DisplayServer::get_singleton()->process_events(); // Get rid of pending events.
				Main::iteration();
			}
		}
		OS::get_singleton()->delay_usec(1000);
	}

	eta.execute_output_thread.wait_to_finish();
	execute_outputs->add_text("\nExit Code: " + itos(eta.exitcode));

	if (execute_output_dialog) {
		if (p_close_on_errors && eta.exitcode != 0) {
			execute_output_dialog->hide();
		}
		if (p_close_on_ok && eta.exitcode == 0) {
			execute_output_dialog->hide();
		}

		execute_output_dialog->get_ok_button()->set_disabled(false);
	}

	if (r_output) {
		*r_output = eta.output;
	}
	return eta.exitcode;
}

