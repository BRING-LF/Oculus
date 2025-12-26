/**************************************************************************/
/*  editor_node_distraction_free.cpp                                      */
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

#include "core/string/string_name.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_main_screen.h"
#include "editor/settings/editor_settings.h"
#include "scene/gui/button.h"

void EditorNode::_toggle_distraction_free_mode() {
	if (EDITOR_GET("interface/editor/separate_distraction_mode")) {
		int screen = editor_main_screen->get_selected_index();

		if (screen == EditorMainScreen::EDITOR_SCRIPT) {
			script_distraction_free = !script_distraction_free;
			set_distraction_free_mode(script_distraction_free);
		} else {
			scene_distraction_free = !scene_distraction_free;
			set_distraction_free_mode(scene_distraction_free);
		}
	} else {
		set_distraction_free_mode(distraction_free->is_pressed());
	}
}

void EditorNode::update_distraction_free_mode() {
	if (!EDITOR_GET("interface/editor/separate_distraction_mode")) {
		return;
	}
	int screen = editor_main_screen->get_selected_index();
	if (screen == EditorMainScreen::EDITOR_SCRIPT) {
		set_distraction_free_mode(script_distraction_free);
	} else {
		set_distraction_free_mode(scene_distraction_free);
	}
}

void EditorNode::set_distraction_free_mode(bool p_enter) {
	distraction_free->set_pressed(p_enter);

	if (p_enter) {
		if (editor_dock_manager->are_docks_visible()) {
			editor_dock_manager->set_docks_visible(false);
		}
	} else {
		editor_dock_manager->set_docks_visible(true);
	}
}

bool EditorNode::is_distraction_free_mode_enabled() const {
	return distraction_free->is_pressed();
}

void EditorNode::update_distraction_free_button_theme() {
	if (distraction_free->get_meta("_scene_tabs_owned", true)) {
		distraction_free->set_theme_type_variation("FlatMenuButton");
		distraction_free->add_theme_style_override(SceneStringName(pressed), theme->get_stylebox(CoreStringName(normal), "FlatMenuButton"));
	} else {
		distraction_free->set_theme_type_variation("BottomPanelButton");
		distraction_free->remove_theme_style_override(SceneStringName(pressed));
	}
}

