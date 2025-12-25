/**************************************************************************/
/*  editor_node_screenshot.cpp                                            */
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
#include "core/io/dir_access.h"
#include "core/io/image.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_string_names.h"
#include "editor/run/editor_run.h"
#include "editor/settings/editor_settings.h"
#include "scene/gui/box_container.h"
#include "scene/main/viewport.h"

void EditorNode::_request_screenshot() {
	_screenshot();
}

void EditorNode::_screenshot(bool p_use_utc) {
	String name = "editor_screenshot_" + Time::get_singleton()->get_datetime_string_from_system(p_use_utc).remove_char(':') + ".png";
	String path = String("user://") + name;

	if (!EditorRun::request_screenshot(callable_mp(this, &EditorNode::_save_screenshot_with_embedded_process).bind(path))) {
		_save_screenshot(path);
	}
}

void EditorNode::_save_screenshot_with_embedded_process(int64_t p_w, int64_t p_h, const String &p_emb_path, const Rect2i &p_rect, const String &p_path) {
	VBoxContainer *main_screen_control = editor_main_screen->get_control();
	ERR_FAIL_NULL_MSG(main_screen_control, "Cannot get the editor main screen control.");
	Viewport *viewport = main_screen_control->get_viewport();
	ERR_FAIL_NULL_MSG(viewport, "Cannot get a viewport from the editor main screen.");
	Ref<ViewportTexture> texture = viewport->get_texture();
	ERR_FAIL_COND_MSG(texture.is_null(), "Cannot get a viewport texture from the editor main screen.");
	Ref<Image> img = texture->get_image();
	ERR_FAIL_COND_MSG(img.is_null(), "Cannot get an image from a viewport texture of the editor main screen.");
	img->convert(Image::FORMAT_RGBA8);
	ERR_FAIL_COND(p_emb_path.is_empty());
	Ref<Image> overlay = Image::load_from_file(p_emb_path);
	DirAccess::remove_absolute(p_emb_path);
	ERR_FAIL_COND_MSG(overlay.is_null(), "Cannot get an image from a embedded process.");
	overlay->convert(Image::FORMAT_RGBA8);
	overlay->resize(p_rect.size.x, p_rect.size.y);
	img->blend_rect(overlay, Rect2i(0, 0, p_w, p_h), p_rect.position);
	Error error = img->save_png(p_path);
	ERR_FAIL_COND_MSG(error != OK, "Cannot save screenshot to file '" + p_path + "'.");

	if (EDITOR_GET("interface/editor/automatically_open_screenshots")) {
		OS::get_singleton()->shell_show_in_file_manager(ProjectSettings::get_singleton()->globalize_path(p_path), true);
	}
}

void EditorNode::_save_screenshot(const String &p_path) {
	VBoxContainer *main_screen_control = editor_main_screen->get_control();
	ERR_FAIL_NULL_MSG(main_screen_control, "Cannot get the editor main screen control.");
	Viewport *viewport = main_screen_control->get_viewport();
	ERR_FAIL_NULL_MSG(viewport, "Cannot get a viewport from the editor main screen.");
	Ref<ViewportTexture> texture = viewport->get_texture();
	ERR_FAIL_COND_MSG(texture.is_null(), "Cannot get a viewport texture from the editor main screen.");
	Ref<Image> img = texture->get_image();
	ERR_FAIL_COND_MSG(img.is_null(), "Cannot get an image from a viewport texture of the editor main screen.");
	Error error = img->save_png(p_path);
	ERR_FAIL_COND_MSG(error != OK, "Cannot save screenshot to file '" + p_path + "'.");

	if (EDITOR_GET("interface/editor/automatically_open_screenshots")) {
		OS::get_singleton()->shell_show_in_file_manager(ProjectSettings::get_singleton()->globalize_path(p_path), true);
	}
}

