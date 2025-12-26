/**************************************************************************/
/*  editor_node_dialogs.cpp                                               */
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
#include "core/os/thread.h"
#include "core/object/object.h"
#include "editor/editor_interface.h"
#include "editor/editor_log.h"
#include "editor/editor_string_names.h"
#include "editor/gui/progress_dialog.h"
#include "servers/display/display_server.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/button.h"
#include "scene/gui/rich_text_label.h"
#include "scene/main/window.h"

void EditorNode::_dialog_display_save_error(String p_file, Error p_error) {
	if (p_error) {
		switch (p_error) {
			case ERR_FILE_CANT_WRITE: {
				show_accept(TTR("Can't open file for writing:") + " " + p_file.get_extension(), TTR("OK"));
			} break;
			case ERR_FILE_UNRECOGNIZED: {
				show_accept(TTR("Requested file format unknown:") + " " + p_file.get_extension(), TTR("OK"));
			} break;
			default: {
				show_accept(TTR("Error while saving."), TTR("OK"));
			} break;
		}
	}
}

void EditorNode::_dialog_display_load_error(String p_file, Error p_error) {
	if (p_error) {
		switch (p_error) {
			case ERR_CANT_OPEN: {
				show_accept(vformat(TTR("Can't open file '%s'. The file could have been moved or deleted."), p_file.get_file()), TTR("OK"));
			} break;
			case ERR_PARSE_ERROR: {
				show_accept(vformat(TTR("Error while parsing file '%s'."), p_file.get_file()), TTR("OK"));
			} break;
			case ERR_FILE_CORRUPT: {
				show_accept(vformat(TTR("Scene file '%s' appears to be invalid/corrupt."), p_file.get_file()), TTR("OK"));
			} break;
			case ERR_FILE_NOT_FOUND: {
				show_accept(vformat(TTR("Missing file '%s' or one of its dependencies."), p_file.get_file()), TTR("OK"));
			} break;
			case ERR_FILE_UNRECOGNIZED: {
				show_accept(vformat(TTR("File '%s' is saved in a format that is newer than the formats supported by this version of Godot, so it can't be opened."), p_file.get_file()), TTR("OK"));
			} break;
			default: {
				show_accept(vformat(TTR("Error while loading file '%s'."), p_file.get_file()), TTR("OK"));
			} break;
		}
	}
}

void EditorNode::show_accept(const String &p_text, const String &p_title) {
	if (!accept) {
		accept = memnew(AcceptDialog);
		accept->set_flag(Window::FLAG_POPUP, false);
		gui_base->add_child(accept);
		accept->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_close_messages));
		accept->connect("canceled", callable_mp(this, &EditorNode::_close_messages));
	}

	accept->set_title(p_title);
	accept->set_text(p_text);
	accept->popup_centered();
}

void EditorNode::show_save_accept(const String &p_text, const String &p_title) {
	if (!save_accept) {
		save_accept = memnew(AcceptDialog);
		save_accept->set_flag(Window::FLAG_POPUP, false);
		gui_base->add_child(save_accept);
		save_accept->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_close_messages));
		save_accept->connect("canceled", callable_mp(this, &EditorNode::_close_messages));
	}

	save_accept->set_title(p_title);
	save_accept->set_text(p_text);
	save_accept->popup_centered();
}

void EditorNode::show_warning(const String &p_text, const String &p_title) {
	if (!warning) {
		warning = memnew(AcceptDialog);
		warning->set_flag(Window::FLAG_POPUP, false);
		gui_base->add_child(warning);
		warning->set_title(TTR("Warning!"));
		warning->add_button(TTR("Copy"), true, "copy");
		warning->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_close_messages));
		warning->connect("custom_action", callable_mp(this, &EditorNode::_copy_warning));
	}

	warning->set_title(p_title);
	warning->set_text(p_text);
	warning->popup_centered();
}

void EditorNode::_copy_warning(const String &p_str) {
	DisplayServer::get_singleton()->clipboard_set(warning->get_text());
}

void EditorNode::add_io_error(const String &p_error) {
	DEV_ASSERT(Thread::get_caller_id() == Thread::get_main_id());
	singleton->load_errors->add_image(singleton->theme->get_icon(SNAME("Error"), EditorStringName(EditorIcons)));
	singleton->load_errors->add_text(p_error + "\n");
	// When a progress dialog is displayed, we will wait for it ot close before displaying
	// the io errors to prevent the io popup to set it's parent to the progress dialog.
	if (singleton->progress_dialog->is_visible()) {
		singleton->load_errors_queued_to_display = true;
	} else {
		Window *window = Object::cast_to<Window>(singleton->load_error_dialog);
		if (window) {
			EditorInterface::get_singleton()->popup_dialog_centered_ratio(window, 0.5);
		}
	}
}

void EditorNode::add_io_warning(const String &p_warning) {
	DEV_ASSERT(Thread::get_caller_id() == Thread::get_main_id());
	singleton->load_errors->add_image(singleton->theme->get_icon(SNAME("Warning"), EditorStringName(EditorIcons)));
	singleton->load_errors->add_text(p_warning + "\n");
	// When a progress dialog is displayed, we will wait for it ot close before displaying
	// the io errors to prevent the io popup to set it's parent to the progress dialog.
	if (singleton->progress_dialog->is_visible()) {
		singleton->load_errors_queued_to_display = true;
	} else {
		Window *window = Object::cast_to<Window>(singleton->load_error_dialog);
		if (window) {
			EditorInterface::get_singleton()->popup_dialog_centered_ratio(window, 0.5);
		}
	}
}

void EditorNode::_load_error_dialog_visibility_changed() {
	if (!load_error_dialog->is_visible()) {
		load_errors->clear();
	}
}

void EditorNode::_file_access_close_error_notify(const String &p_str) {
	callable_mp_static(&EditorNode::_file_access_close_error_notify_impl).call_deferred(p_str);
}

void EditorNode::_file_access_close_error_notify_impl(const String &p_str) {
	add_io_error(vformat(TTR("Unable to write to file '%s', file in use, locked or lacking permissions."), p_str));
}

