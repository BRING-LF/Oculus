/**************************************************************************/
/*  editor_node_renderer.cpp                                              */
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
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "editor_node.h"

#include "core/config/project_settings.h"
#include "core/string/string_name.h"
#include "core/string/translation_server.h"
#include "editor/editor_string_names.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/option_button.h"

void EditorNode::_update_renderer_color() {
	String rendering_method = renderer->get_selected_metadata();

	if (rendering_method == "forward_plus") {
		renderer->add_theme_color_override(SceneStringName(font_color), theme->get_color(SNAME("forward_plus_color"), EditorStringName(Editor)));
	} else if (rendering_method == "mobile") {
		renderer->add_theme_color_override(SceneStringName(font_color), theme->get_color(SNAME("mobile_color"), EditorStringName(Editor)));
	} else if (rendering_method == "gl_compatibility") {
		renderer->add_theme_color_override(SceneStringName(font_color), theme->get_color(SNAME("gl_compatibility_color"), EditorStringName(Editor)));
	}
}

void EditorNode::_renderer_selected(int p_index) {
	const String rendering_method = renderer->get_item_metadata(p_index);
	const String current_renderer = GLOBAL_GET("rendering/renderer/rendering_method");
	if (rendering_method == current_renderer) {
		return;
	}

	// Don't change selection.
	for (int i = 0; i < renderer->get_item_count(); i++) {
		if (renderer->get_item_metadata(i) == current_renderer) {
			renderer->select(i);
			break;
		}
	}

	if (video_restart_dialog == nullptr) {
		video_restart_dialog = memnew(ConfirmationDialog);
		video_restart_dialog->set_ok_button_text(TTRC("Save & Restart"));
		video_restart_dialog->get_label()->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
		gui_base->add_child(video_restart_dialog);
	} else {
		video_restart_dialog->disconnect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_set_renderer_name_save_and_restart));
	}

	const String mobile_rendering_method = rendering_method == "forward_plus" ? "mobile" : rendering_method;
	const String web_rendering_method = "gl_compatibility";
	video_restart_dialog->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_set_renderer_name_save_and_restart).bind(rendering_method));
	video_restart_dialog->set_text(
			vformat(TTR("Changing the renderer requires restarting the editor.\n\nChoosing Save & Restart will change the renderer to:\n- Desktop platforms: %s\n- Mobile platforms: %s\n- Web platform: %s"),
					_to_rendering_method_display_name(rendering_method), _to_rendering_method_display_name(mobile_rendering_method), _to_rendering_method_display_name(web_rendering_method)));
	video_restart_dialog->popup_centered();

	_update_renderer_color();
}

String EditorNode::_to_rendering_method_display_name(const String &p_rendering_method) const {
	if (p_rendering_method == "forward_plus") {
		return TTR("Forward+");
	}
	if (p_rendering_method == "mobile") {
		return TTR("Mobile");
	}
	if (p_rendering_method == "gl_compatibility") {
		return TTR("Compatibility");
	}
	return p_rendering_method;
}

void EditorNode::_set_renderer_name_save_and_restart(const String &p_rendering_method) {
	ProjectSettings::get_singleton()->set("rendering/renderer/rendering_method", p_rendering_method);

	if (p_rendering_method == "mobile" || p_rendering_method == "gl_compatibility") {
		// Also change the mobile override if changing to a compatible renderer.
		// This prevents visual discrepancies between desktop and mobile platforms.
		ProjectSettings::get_singleton()->set("rendering/renderer/rendering_method.mobile", p_rendering_method);
	} else if (p_rendering_method == "forward_plus") {
		// Use the equivalent mobile renderer. This prevents the renderer from staying
		// on its old choice if moving from `gl_compatibility` to `forward_plus`.
		ProjectSettings::get_singleton()->set("rendering/renderer/rendering_method.mobile", "mobile");
	}

	ProjectSettings::get_singleton()->save();

	save_all_scenes();
	restart_editor();
}