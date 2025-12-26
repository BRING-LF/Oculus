/**************************************************************************/
/*  editor_node_theme.cpp                                                 */
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

#include "editor/editor_string_names.h"
#include "editor/gui/editor_bottom_panel.h"
#include "editor/editor_main_screen.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/themes/editor_color_map.h"
#include "editor/themes/editor_theme_manager.h"
#include "scene/gui/button.h"
#include "scene/gui/control.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/popup_menu.h"
#include "scene/main/window.h"
#include "scene/resources/dpi_texture.h"
#include "scene/resources/image_texture.h"
#include "scene/theme/theme_db.h"
#include "servers/display/display_server.h"

void EditorNode::_update_theme(bool p_skip_creation) {
	if (!p_skip_creation) {
		theme = EditorThemeManager::generate_theme(theme);
		DisplayServer::set_early_window_clear_color_override(true, theme->get_color(SNAME("background"), EditorStringName(Editor)));
	}

	Vector<Ref<Theme>> editor_themes;
	editor_themes.push_back(theme);
	editor_themes.push_back(ThemeDB::get_singleton()->get_default_theme());

	ThemeContext *node_tc = ThemeDB::get_singleton()->get_theme_context(this);
	if (node_tc) {
		node_tc->set_themes(editor_themes);
	} else {
		ThemeDB::get_singleton()->create_theme_context(this, editor_themes);
	}

	Window *window = get_window();
	if (window) {
		ThemeContext *window_tc = ThemeDB::get_singleton()->get_theme_context(window);
		if (window_tc) {
			window_tc->set_themes(editor_themes);
		} else {
			ThemeDB::get_singleton()->create_theme_context(window, editor_themes);
		}
	}

	if (CanvasItemEditor::get_singleton()->get_theme_preview() == CanvasItemEditor::THEME_PREVIEW_EDITOR) {
		update_preview_themes(CanvasItemEditor::THEME_PREVIEW_EDITOR);
	}

	// Update styles.
	{
		bool dark_mode = DisplayServer::get_singleton()->is_dark_mode_supported() && DisplayServer::get_singleton()->is_dark_mode();

		gui_base->add_theme_style_override(SceneStringName(panel), theme->get_stylebox(SNAME("Background"), EditorStringName(EditorStyles)));
		main_vbox->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE, theme->get_constant(SNAME("window_border_margin"), EditorStringName(Editor)));
		main_vbox->add_theme_constant_override("separation", theme->get_constant(SNAME("top_bar_separation"), EditorStringName(Editor)));

		if (main_menu_button != nullptr) {
			main_menu_button->set_button_icon(theme->get_icon(SNAME("TripleBar"), EditorStringName(EditorIcons)));
		}

		editor_main_screen->add_theme_style_override(SceneStringName(panel), theme->get_stylebox(SNAME("Content"), EditorStringName(EditorStyles)));
		bottom_panel->_theme_changed();
		distraction_free->set_button_icon(theme->get_icon(SNAME("DistractionFree"), EditorStringName(EditorIcons)));
		update_distraction_free_button_theme();

		help_menu->set_item_icon(help_menu->get_item_index(HELP_SEARCH), get_editor_theme_native_menu_icon(SNAME("HelpSearch"), menu_type == MENU_TYPE_GLOBAL, dark_mode));
		help_menu->set_item_icon(help_menu->get_item_index(HELP_COPY_SYSTEM_INFO), get_editor_theme_native_menu_icon(SNAME("ActionCopy"), menu_type == MENU_TYPE_GLOBAL, dark_mode));
		help_menu->set_item_icon(help_menu->get_item_index(HELP_ABOUT), get_editor_theme_native_menu_icon(SNAME("Godot"), menu_type == MENU_TYPE_GLOBAL, dark_mode));
		help_menu->set_item_icon(help_menu->get_item_index(HELP_SUPPORT_GODOT_DEVELOPMENT), get_editor_theme_native_menu_icon(SNAME("Heart"), menu_type == MENU_TYPE_GLOBAL, dark_mode));

		_update_renderer_color();
	}

	Ref<Texture2D> thumbnail_icon = gui_base->get_theme_icon(SNAME("file_thumbnail"), SNAME("FileDialog"));
	default_thumbnail.instantiate();
	default_thumbnail->set_image(thumbnail_icon->get_image());

	editor_dock_manager->update_tab_styles();
	editor_dock_manager->update_docks_menu();
	editor_dock_manager->set_tab_icon_max_width(theme->get_constant(SNAME("class_icon_size"), EditorStringName(Editor)));
#ifdef ANDROID_ENABLED
	DisplayServer::get_singleton()->window_set_color(theme->get_color(SNAME("background"), EditorStringName(Editor)));
#endif
}

Ref<Texture2D> EditorNode::get_editor_theme_native_menu_icon(const StringName &p_name, bool p_global_menu, bool p_dark_mode) const {
	Ref<Texture2D> tx = theme->get_icon(p_name, SNAME("EditorIcons"));
	if (!p_global_menu || p_dark_mode == EditorThemeManager::is_dark_icon_and_font()) {
		return tx;
	}

	Ref<DPITexture> new_tx = tx;
	if (new_tx.is_null()) {
		return tx;
	}
	new_tx = new_tx->duplicate();

	Dictionary color_conversion_map;
	if (!p_dark_mode) {
		for (KeyValue<Color, Color> &E : EditorColorMap::get_color_conversion_map()) {
			color_conversion_map[E.key] = E.value;
		}
	}
	new_tx->set_color_map(color_conversion_map);

	return new_tx;
}

void EditorNode::update_preview_themes(int p_mode) {
	if (!scene_root->is_inside_tree()) {
		return; // Too early.
	}

	Vector<Ref<Theme>> preview_themes;

	switch (p_mode) {
		case CanvasItemEditor::THEME_PREVIEW_PROJECT:
			preview_themes.push_back(ThemeDB::get_singleton()->get_project_theme());
			break;

		case CanvasItemEditor::THEME_PREVIEW_EDITOR:
			preview_themes.push_back(get_editor_theme());
			break;

		default:
			break;
	}

	preview_themes.push_back(ThemeDB::get_singleton()->get_default_theme());

	ThemeContext *preview_context = ThemeDB::get_singleton()->get_theme_context(scene_root);
	if (preview_context) {
		preview_context->set_themes(preview_themes);
	} else {
		ThemeDB::get_singleton()->create_theme_context(scene_root, preview_themes);
	}
}

void EditorNode::_check_system_theme_changed() {
	DisplayServer *display_server = DisplayServer::get_singleton();

	bool system_theme_changed = false;

	if (follow_system_theme) {
		if (display_server->get_base_color() != last_system_base_color) {
			system_theme_changed = true;
			last_system_base_color = display_server->get_base_color();
		}

		if (display_server->is_dark_mode_supported() && display_server->is_dark_mode() != last_dark_mode_state) {
			system_theme_changed = true;
			last_dark_mode_state = display_server->is_dark_mode();
		}
	}

	if (use_system_accent_color) {
		if (display_server->get_accent_color() != last_system_accent_color) {
			system_theme_changed = true;
			last_system_accent_color = display_server->get_accent_color();
		}
	}

	if (system_theme_changed) {
		_update_theme();
	} else if (menu_type == MENU_TYPE_GLOBAL && display_server->is_dark_mode_supported() && display_server->is_dark_mode() != last_dark_mode_state) {
		last_dark_mode_state = display_server->is_dark_mode();

		// Update system menus.
		bool dark_mode = DisplayServer::get_singleton()->is_dark_mode();

		help_menu->set_item_icon(help_menu->get_item_index(HELP_SEARCH), get_editor_theme_native_menu_icon(SNAME("HelpSearch"), menu_type == MENU_TYPE_GLOBAL, dark_mode));
		help_menu->set_item_icon(help_menu->get_item_index(HELP_COPY_SYSTEM_INFO), get_editor_theme_native_menu_icon(SNAME("ActionCopy"), menu_type == MENU_TYPE_GLOBAL, dark_mode));
		help_menu->set_item_icon(help_menu->get_item_index(HELP_ABOUT), get_editor_theme_native_menu_icon(SNAME("Godot"), menu_type == MENU_TYPE_GLOBAL, dark_mode));
		help_menu->set_item_icon(help_menu->get_item_index(HELP_SUPPORT_GODOT_DEVELOPMENT), get_editor_theme_native_menu_icon(SNAME("Heart"), menu_type == MENU_TYPE_GLOBAL, dark_mode));
		editor_dock_manager->update_docks_menu();
	}
}

