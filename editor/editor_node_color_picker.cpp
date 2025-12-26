/**************************************************************************/
/*  editor_node_color_picker.cpp                                         */
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

#include "core/io/resource_loader.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/editor_quick_open_dialog.h"
#include "editor/settings/editor_settings.h"
#include "scene/gui/color_picker.h"

void EditorNode::_palette_quick_open_dialog() {
	quick_open_color_palette->popup_dialog({ "ColorPalette" }, palette_file_selected_callback);
	quick_open_color_palette->set_title(TTRC("Quick Open Color Palette..."));
}

void EditorNode::setup_color_picker(ColorPicker *p_picker) {
	p_picker->set_editor_settings(EditorSettings::get_singleton());
	int default_color_mode = EditorSettings::get_singleton()->get_project_metadata("color_picker", "color_mode", EDITOR_GET("interface/inspector/default_color_picker_mode"));
	int picker_shape = EditorSettings::get_singleton()->get_project_metadata("color_picker", "picker_shape", EDITOR_GET("interface/inspector/default_color_picker_shape"));
	bool show_intensity = EDITOR_GET("interface/inspector/color_picker_show_intensity");

	p_picker->set_color_mode((ColorPicker::ColorModeType)default_color_mode);
	p_picker->set_picker_shape((ColorPicker::PickerShapeType)picker_shape);
	p_picker->set_edit_intensity(show_intensity);

	p_picker->set_quick_open_callback(callable_mp(this, &EditorNode::_palette_quick_open_dialog));
	p_picker->set_palette_saved_callback(callable_mp(EditorFileSystem::get_singleton(), &EditorFileSystem::update_file));
	palette_file_selected_callback = callable_mp(p_picker, &ColorPicker::_quick_open_palette_file_selected);
}

