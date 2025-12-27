/**************************************************************************/
/*  editor_node_init_ui.cpp                                               */
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

#include "core/input/input.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/os/os.h"
#include "editor/editor_interface.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/control.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/file_dialog.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/packed_scene.h"
#include "servers/display/display_server.h"

void EditorNode::_init_ui_settings() {
	{
		bool agile_input_event_flushing = EDITOR_GET("input/buffering/agile_event_flushing");
		bool use_accumulated_input = EDITOR_GET("input/buffering/use_accumulated_input");

		Input::get_singleton()->set_agile_input_event_flushing(agile_input_event_flushing);
		Input::get_singleton()->set_use_accumulated_input(use_accumulated_input);
	}

	{
		int display_scale = EDITOR_GET("interface/editor/display_scale");

		switch (display_scale) {
			case 0:
				// Try applying a suitable display scale automatically.
				EditorScale::set_scale(EditorSettings::get_auto_display_scale());
				break;
			case 1:
				EditorScale::set_scale(0.75);
				break;
			case 2:
				EditorScale::set_scale(1.0);
				break;
			case 3:
				EditorScale::set_scale(1.25);
				break;
			case 4:
				EditorScale::set_scale(1.5);
				break;
			case 5:
				EditorScale::set_scale(1.75);
				break;
			case 6:
				EditorScale::set_scale(2.0);
				break;
			default:
				EditorScale::set_scale(EDITOR_GET("interface/editor/custom_display_scale"));
				break;
		}
	}

	// Define a minimum window size to prevent UI elements from overlapping or being cut off.
	Window *w = Object::cast_to<Window>(SceneTree::get_singleton()->get_root());
	if (w) {
		const Size2 minimum_size = Size2(1024, 600) * EDSCALE;
		w->set_min_size(minimum_size); // Calling it this early doesn't sync the property with DS.
		DisplayServer::get_singleton()->window_set_min_size(minimum_size);
	}

	FileDialog::set_default_show_hidden_files(EDITOR_GET("filesystem/file_dialog/show_hidden_files"));
	FileDialog::set_default_display_mode(EDITOR_GET("filesystem/file_dialog/display_mode"));

	int swap_cancel_ok = EDITOR_GET("interface/editor/accept_dialog_cancel_ok_buttons");
	if (swap_cancel_ok != 0) { // 0 is auto, set in register_scene based on DisplayServer.
		// Swap on means OK first.
		AcceptDialog::set_swap_cancel_ok(swap_cancel_ok == 2);
	}

	int ed_root_dir = EDITOR_GET("interface/editor/ui_layout_direction");
	Control::set_root_layout_direction(ed_root_dir);
	Window::set_root_layout_direction(ed_root_dir);

	ResourceLoader::set_abort_on_missing_resources(false);
	ResourceLoader::set_error_notify_func(&EditorNode::add_io_error);
	ResourceLoader::set_dependency_error_notify_func(&EditorNode::_dependency_error_report);

	SceneState::set_instantiation_warning_notify_func([](const String &p_warning) {
		add_io_warning(p_warning);
		callable_mp(EditorInterface::get_singleton(), &EditorInterface::mark_scene_as_unsaved).call_deferred();
	});
}

