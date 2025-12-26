/**************************************************************************/
/*  editor_node_layout_menu.cpp                                           */
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

#include "core/error/error_macros.h"
#include "core/io/config_file.h"
#include "core/string/translation_server.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/settings/editor_layouts_dialog.h"
#include "editor/settings/editor_settings.h"
#include "scene/gui/popup_menu.h"

void EditorNode::_update_layouts_menu() {
	editor_layouts->clear();
	overridden_default_layout = -1;

	editor_layouts->reset_size();
	editor_layouts->add_shortcut(ED_SHORTCUT("layout/save", TTRC("Save Layout...")), LAYOUT_SAVE);
	editor_layouts->add_shortcut(ED_SHORTCUT("layout/delete", TTRC("Delete Layout...")), LAYOUT_DELETE);
	editor_layouts->add_separator();
	editor_layouts->add_shortcut(ED_SHORTCUT("layout/default", TTRC("Default")), LAYOUT_DEFAULT);

	Ref<ConfigFile> config;
	config.instantiate();
	Error err = config->load(EditorSettings::get_singleton()->get_editor_layouts_config());
	if (err != OK) {
		return; // No config.
	}

	Vector<String> layouts = config->get_sections();
	const String default_layout_name = TTR("Default");

	for (const String &layout : layouts) {
		if (layout.contains_char('/')) {
			continue;
		}

		if (layout == default_layout_name) {
			editor_layouts->remove_item(editor_layouts->get_item_index(LAYOUT_DEFAULT));
			overridden_default_layout = editor_layouts->get_item_count();
		}

		editor_layouts->add_item(layout);
		editor_layouts->set_item_auto_translate_mode(-1, AUTO_TRANSLATE_MODE_DISABLED);
	}
}

void EditorNode::_layout_menu_option(int p_id) {
	switch (p_id) {
		case LAYOUT_SAVE: {
			current_menu_option = p_id;
			layout_dialog->set_title(TTR("Save Layout"));
			layout_dialog->set_ok_button_text(TTR("Save"));
			layout_dialog->set_name_line_enabled(true);
			layout_dialog->popup_centered();
		} break;
		case LAYOUT_DELETE: {
			current_menu_option = p_id;
			layout_dialog->set_title(TTR("Delete Layout"));
			layout_dialog->set_ok_button_text(TTR("Delete"));
			layout_dialog->set_name_line_enabled(false);
			layout_dialog->popup_centered();
		} break;
		case LAYOUT_DEFAULT: {
			editor_dock_manager->load_docks_from_config(default_layout, "docks");
			_save_editor_layout();
		} break;
		default: {
			Ref<ConfigFile> config;
			config.instantiate();
			Error err = config->load(EditorSettings::get_singleton()->get_editor_layouts_config());
			if (err != OK) {
				return; // No config.
			}

			editor_dock_manager->load_docks_from_config(config, editor_layouts->get_item_text(p_id));
			_save_editor_layout();
		}
	}
}

