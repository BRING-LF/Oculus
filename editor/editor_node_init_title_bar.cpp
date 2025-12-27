/**************************************************************************/
/*  editor_node_init_title_bar.cpp                                        */
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

#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_title_bar.h"
#include "editor/run/editor_run_bar.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/label.h"
#include "scene/gui/option_button.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server.h"

void EditorNode::_init_title_bar() {
	// Spacer to center 2D / 3D / Script buttons.
	left_spacer = memnew(HBoxContainer);
	left_spacer->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	left_spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	title_bar->add_child(left_spacer);

	project_title = memnew(Label);
	project_title->add_theme_font_override(SceneStringName(font), theme->get_font(SNAME("bold"), EditorStringName(EditorFonts)));
	project_title->add_theme_font_size_override(SceneStringName(font_size), theme->get_font_size(SNAME("bold_size"), EditorStringName(EditorFonts)));
	project_title->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	project_title->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	project_title->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	project_title->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	bool can_expand = bool(EDITOR_GET("interface/editor/expand_to_title")) && DisplayServer::get_singleton()->has_feature(DisplayServer::FEATURE_EXTEND_TO_TITLE);
	project_title->set_visible(can_expand && menu_type == MENU_TYPE_GLOBAL);
	left_spacer->add_child(project_title);

	HBoxContainer *main_editor_button_hb = memnew(HBoxContainer);
	main_editor_button_hb->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	main_editor_button_hb->set_name("EditorMainScreenButtons");
	editor_main_screen->set_button_container(main_editor_button_hb);
	title_bar->add_child(main_editor_button_hb);
	title_bar->set_center_control(main_editor_button_hb);

	// Spacer to center 2D / 3D / Script buttons.
	right_spacer = memnew(Control);
	right_spacer->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	right_spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	title_bar->add_child(right_spacer);

	project_run_bar = memnew(EditorRunBar);
	project_run_bar->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	title_bar->add_child(project_run_bar);
	project_run_bar->connect("play_pressed", callable_mp(this, &EditorNode::_project_run_started));
	project_run_bar->connect("stop_pressed", callable_mp(this, &EditorNode::_project_run_stopped));

	right_menu_hb = memnew(HBoxContainer);
	right_menu_hb->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	title_bar->add_child(right_menu_hb);

	renderer = memnew(OptionButton);
	renderer->set_visible(true);
	renderer->set_flat(true);
	renderer->set_theme_type_variation("TopBarOptionButton");
	renderer->set_fit_to_longest_item(false);
	renderer->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	renderer->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	renderer->set_tooltip_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);
	renderer->set_tooltip_text(TTRC("Choose a renderer.\n\nNotes:\n- On mobile platforms, the Mobile renderer is used if Forward+ is selected here.\n- On the web platform, the Compatibility renderer is always used."));
	renderer->set_accessibility_name(TTRC("Renderer"));

	right_menu_hb->add_child(renderer);

	if (can_expand) {
		// Add spacer to avoid other controls under the window minimize/maximize/close buttons (right side).
		right_menu_spacer = memnew(Control);
		right_menu_spacer->set_mouse_filter(Control::MOUSE_FILTER_PASS);
		title_bar->add_child(right_menu_spacer);
	}

	const String current_renderer_ps = String(GLOBAL_GET("rendering/renderer/rendering_method")).to_lower();
	const String current_renderer_os = OS::get_singleton()->get_current_rendering_method().to_lower();

	// Add the renderers name to the UI.
	if (current_renderer_ps == current_renderer_os) {
		renderer->connect(SceneStringName(item_selected), callable_mp(this, &EditorNode::_renderer_selected));
		// As we are doing string comparisons, keep in standard case to prevent problems with capitals
		// "vulkan" in particular uses lowercase "v" in the code, and uppercase in the UI.
		PackedStringArray renderers = ProjectSettings::get_singleton()->get_custom_property_info().get(StringName("rendering/renderer/rendering_method")).hint_string.split(",", false);
		for (int i = 0; i < renderers.size(); i++) {
			const String rendering_method = renderers[i].to_lower();
			if (rendering_method == "dummy") {
				continue;
			}
			renderer->add_item(String()); // Set in NOTIFICATION_TRANSLATION_CHANGED.
			renderer->set_item_metadata(-1, rendering_method);
			if (current_renderer_ps == rendering_method) {
				renderer->select(i);
			}
		}
	} else {
		// It's an CLI-overridden rendering method.
		renderer->add_item(String()); // Set in NOTIFICATION_TRANSLATION_CHANGED.
		renderer->set_item_metadata(-1, current_renderer_os);
	}
	_update_renderer_color();
}

