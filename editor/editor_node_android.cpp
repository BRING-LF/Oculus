/**************************************************************************/
/*  editor_node_android.cpp                                               */
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
#include "editor/export/editor_export.h"
#include "editor/export/export_template_manager.h"
#include "editor/gui/editor_file_dialog.h"
#include "scene/gui/option_button.h"

static const String INSTALL_ANDROID_BUILD_TEMPLATE_MESSAGE = TTRC("This will set up your project for gradle Android builds by installing the source template to \"%s\".\nNote that in order to make gradle builds instead of using pre-built APKs, the \"Use Gradle Build\" option should be enabled in the Android export preset.");

void EditorNode::_android_build_source_selected(const String &p_file) {
	export_template_manager->install_android_template_from_file(p_file, android_export_preset);
}

void EditorNode::_android_export_preset_selected(int p_index) {
	if (p_index >= 0) {
		android_export_preset = EditorExport::get_singleton()->get_export_preset(choose_android_export_profile->get_item_id(p_index));
	} else {
		android_export_preset.unref();
	}
	install_android_build_template_message->set_text(vformat(TTR(INSTALL_ANDROID_BUILD_TEMPLATE_MESSAGE), export_template_manager->get_android_build_directory(android_export_preset)));
}

void EditorNode::_android_install_build_template() {
	gradle_build_manage_templates->hide();
	file_android_build_source->popup_centered_ratio();
}

void EditorNode::_android_explore_build_templates() {
	OS::get_singleton()->shell_show_in_file_manager(ProjectSettings::get_singleton()->globalize_path(export_template_manager->get_android_build_directory(android_export_preset).get_base_dir()), true);
}

