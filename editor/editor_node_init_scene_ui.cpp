/**************************************************************************/
/*  editor_node_init_scene_ui.cpp                                        */
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

#include "editor/editor_main_screen.h"
#include "editor/scene/editor_scene_tabs.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/split_container.h"
#include "scene/main/viewport.h"

void EditorNode::_init_scene_ui() {
	top_split = memnew(VSplitContainer);
	center_split->add_child(top_split);
	top_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	top_split->set_collapsed(true);

	VBoxContainer *srt = memnew(VBoxContainer);
	srt->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	srt->add_theme_constant_override("separation", 0);
	top_split->add_child(srt);

	scene_tabs = memnew(EditorSceneTabs);
	srt->add_child(scene_tabs);
	scene_tabs->connect("tab_changed", callable_mp(this, &EditorNode::_set_current_scene));
	scene_tabs->connect("tab_closed", callable_mp(this, &EditorNode::_scene_tab_closed));

	distraction_free = memnew(Button);
	distraction_free->set_theme_type_variation("FlatMenuButton");
	ED_SHORTCUT_AND_COMMAND("editor/distraction_free_mode", TTRC("Distraction Free Mode"), KeyModifierMask::CTRL | KeyModifierMask::SHIFT | Key::F11);
	ED_SHORTCUT_OVERRIDE("editor/distraction_free_mode", "macos", KeyModifierMask::META | KeyModifierMask::SHIFT | Key::D);
	ED_SHORTCUT_AND_COMMAND("editor/toggle_last_opened_bottom_panel", TTRC("Toggle Last Opened Bottom Panel"), KeyModifierMask::CMD_OR_CTRL | Key::J);
	distraction_free->set_shortcut(ED_GET_SHORTCUT("editor/distraction_free_mode"));
	distraction_free->set_tooltip_text(TTRC("Toggle distraction-free mode."));
	distraction_free->set_toggle_mode(true);
	scene_tabs->add_extra_button(distraction_free);
	distraction_free->connect(SceneStringName(pressed), callable_mp(this, &EditorNode::_toggle_distraction_free_mode));

	editor_main_screen = memnew(EditorMainScreen);
	editor_main_screen->set_custom_minimum_size(Size2(0, 80) * EDSCALE);
	editor_main_screen->set_draw_behind_parent(true);
	srt->add_child(editor_main_screen);
	editor_main_screen->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	scene_root = memnew(SubViewport);
	scene_root->set_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);
	scene_root->set_translation_domain(StringName());
	scene_root->set_embedding_subwindows(true);
	scene_root->set_disable_3d(true);
	scene_root->set_disable_input(true);
	scene_root->set_as_audio_listener_2d(true);
}

