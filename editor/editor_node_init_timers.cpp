/**************************************************************************/
/*  editor_node_init_timers.cpp                                          */
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

#include "editor/file_system/editor_file_system.h"
#include "editor/settings/editor_settings.h"
#include "scene/main/timer.h"

void EditorNode::_init_timers() {
	editor_layout_save_delay_timer = memnew(Timer);
	add_child(editor_layout_save_delay_timer);
	editor_layout_save_delay_timer->set_wait_time(0.5);
	editor_layout_save_delay_timer->set_one_shot(true);
	editor_layout_save_delay_timer->connect("timeout", callable_mp(this, &EditorNode::_save_editor_layout));

	scan_changes_timer = memnew(Timer);
	scan_changes_timer->set_wait_time(0.5);
	scan_changes_timer->set_autostart(EDITOR_GET("interface/editor/import_resources_when_unfocused"));
	scan_changes_timer->connect("timeout", callable_mp(EditorFileSystem::get_singleton(), &EditorFileSystem::scan_changes));
	add_child(scan_changes_timer);
}

