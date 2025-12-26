/**************************************************************************/
/*  editor_node_print.cpp                                                 */
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

#include "core/os/thread.h"
#include "editor/editor_log.h"

void EditorNode::_print_handler(void *p_this, const String &p_string, bool p_error, bool p_rich) {
	if (!Thread::is_main_thread()) {
		callable_mp_static(&EditorNode::_print_handler_impl).call_deferred(p_string, p_error, p_rich);
	} else {
		_print_handler_impl(p_string, p_error, p_rich);
	}
}

void EditorNode::_print_handler_impl(const String &p_string, bool p_error, bool p_rich) {
	if (!singleton) {
		return;
	}
	if (p_error) {
		singleton->log->add_message(p_string, EditorLog::MSG_TYPE_ERROR);
	} else if (p_rich) {
		singleton->log->add_message(p_string, EditorLog::MSG_TYPE_STD_RICH);
	} else {
		singleton->log->add_message(p_string, EditorLog::MSG_TYPE_STD);
	}
}

