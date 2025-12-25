/**************************************************************************/
/*  editor_node_menu_build.cpp                                            */
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

#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/editor_title_bar.h"
#include "editor/settings/editor_settings.h"
#include "editor/version_control/version_control_editor_plugin.h"
#include "scene/gui/control.h"
#include "scene/gui/menu_bar.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/popup.h"
#include "servers/display/display_server.h"

void EditorNode::_build_file_menu() {
	if (!file_menu) {
		return;
	}
	file_menu->clear(false);

	file_menu->add_shortcut(ED_GET_SHORTCUT("editor/new_scene"), SCENE_NEW_SCENE);
	file_menu->add_shortcut(ED_GET_SHORTCUT("editor/new_inherited_scene"), SCENE_NEW_INHERITED_SCENE);
	file_menu->add_shortcut(ED_GET_SHORTCUT("editor/open_scene"), SCENE_OPEN_SCENE);
	file_menu->add_shortcut(ED_GET_SHORTCUT("editor/reopen_closed_scene"), SCENE_OPEN_PREV);
	if (!recent_scenes) {
		recent_scenes = memnew(PopupMenu);
		recent_scenes->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
		recent_scenes->connect(SceneStringName(id_pressed), callable_mp(this, &EditorNode::_open_recent_scene));
	}
	file_menu->add_submenu_node_item(TTRC("Open Recent"), recent_scenes, SCENE_OPEN_RECENT);
	file_menu->add_separator();

	file_menu->add_shortcut(ED_GET_SHORTCUT("editor/save_scene"), SCENE_SAVE_SCENE);
	file_menu->add_shortcut(ED_GET_SHORTCUT("editor/save_scene_as"), SCENE_SAVE_AS_SCENE);
	file_menu->add_shortcut(ED_GET_SHORTCUT("editor/save_all_scenes"), SCENE_SAVE_ALL_SCENES);
	file_menu->add_separator();

	file_menu->add_shortcut(ED_GET_SHORTCUT("editor/quick_open"), SCENE_QUICK_OPEN);
	file_menu->add_shortcut(ED_GET_SHORTCUT("editor/quick_open_scene"), SCENE_QUICK_OPEN_SCENE);
	file_menu->add_shortcut(ED_GET_SHORTCUT("editor/quick_open_script"), SCENE_QUICK_OPEN_SCRIPT);
	file_menu->add_separator();

	if (!export_as_menu) {
		export_as_menu = memnew(PopupMenu);
		export_as_menu->add_shortcut(ED_GET_SHORTCUT("editor/export_as_mesh_library"), FILE_EXPORT_MESH_LIBRARY);
		export_as_menu->connect("index_pressed", callable_mp(this, &EditorNode::_export_as_menu_option));
	}
	file_menu->add_submenu_node_item(TTRC("Export As..."), export_as_menu, SCENE_EXPORT_AS);
	file_menu->add_separator();

	file_menu->add_shortcut(ED_GET_SHORTCUT("ui_undo"), SCENE_UNDO, false, true);
	file_menu->add_shortcut(ED_GET_SHORTCUT("ui_redo"), SCENE_REDO, false, true);
	file_menu->add_separator();

	file_menu->add_shortcut(ED_GET_SHORTCUT("editor/reload_saved_scene"), SCENE_RELOAD_SAVED_SCENE);
	file_menu->add_shortcut(ED_GET_SHORTCUT("editor/close_scene"), SCENE_CLOSE);
	file_menu->add_shortcut(ED_GET_SHORTCUT("editor/close_all_scenes"), SCENE_CLOSE_ALL);
#ifdef MACOS_ENABLED
	if (menu_type != MENU_TYPE_GLOBAL) {
		// On macOS "Quit" option is in the "app" menu.
		file_menu->add_separator();
		file_menu->add_shortcut(ED_GET_SHORTCUT("editor/file_quit"), SCENE_QUIT, true);
	}
#else
	file_menu->add_separator();
	file_menu->add_shortcut(ED_GET_SHORTCUT("editor/file_quit"), SCENE_QUIT, true);
#endif
}

void EditorNode::_build_project_menu() {
	if (!project_menu) {
		return;
	}
	project_menu->clear(false);

	project_menu->add_shortcut(ED_GET_SHORTCUT("editor/project_settings"), PROJECT_OPEN_SETTINGS);
	project_menu->add_shortcut(ED_GET_SHORTCUT("editor/find_in_files"), PROJECT_FIND_IN_FILES);
	project_menu->add_separator();

	project_menu->add_item(TTRC("Version Control"), PROJECT_VERSION_CONTROL);
	if (!vcs_actions_menu) {
		vcs_actions_menu = VersionControlEditorPlugin::get_singleton()->get_version_control_actions_panel();
		vcs_actions_menu->connect("index_pressed", callable_mp(this, &EditorNode::_version_control_menu_option));
		vcs_actions_menu->add_item(TTRC("Create/Override Version Control Metadata..."), VCS_METADATA);
		vcs_actions_menu->add_item(TTRC("Version Control Settings..."), VCS_SETTINGS);
	}
	project_menu->set_item_submenu_node(project_menu->get_item_index(PROJECT_VERSION_CONTROL), vcs_actions_menu);

	project_menu->add_separator();
	project_menu->add_shortcut(ED_GET_SHORTCUT("editor/export"), PROJECT_EXPORT);
	project_menu->add_item(TTRC("Pack Project as ZIP..."), PROJECT_PACK_AS_ZIP);
	project_menu->add_item(TTRC("Install Android Build Template..."), PROJECT_INSTALL_ANDROID_SOURCE);
#ifndef ANDROID_ENABLED
	project_menu->add_item(TTRC("Open User Data Folder"), PROJECT_OPEN_USER_DATA_FOLDER);
#endif
	project_menu->add_separator();

	if (!tool_menu) {
		tool_menu = memnew(PopupMenu);
		tool_menu->connect("index_pressed", callable_mp(this, &EditorNode::_tool_menu_option));
		tool_menu->add_shortcut(ED_GET_SHORTCUT("editor/orphan_resource_explorer"), TOOLS_ORPHAN_RESOURCES);
		tool_menu->add_shortcut(ED_GET_SHORTCUT("editor/engine_compilation_configuration_editor"), TOOLS_BUILD_PROFILE_MANAGER);
		tool_menu->add_shortcut(ED_GET_SHORTCUT("editor/upgrade_project"), TOOLS_PROJECT_UPGRADE);
	}
	project_menu->add_submenu_node_item(TTRC("Tools"), tool_menu);

	project_menu->add_separator();
	project_menu->add_shortcut(ED_GET_SHORTCUT("editor/reload_current_project"), PROJECT_RELOAD_CURRENT_PROJECT);
	project_menu->add_shortcut(ED_GET_SHORTCUT("editor/quit_to_project_list"), PROJECT_QUIT_TO_PROJECT_MANAGER, true);
}

void EditorNode::_build_settings_menu() {
	if (!settings_menu) {
		return;
	}
	settings_menu->clear(false);

#ifdef MACOS_ENABLED
	if (menu_type != MENU_TYPE_GLOBAL) {
		// On macOS "Settings" option is in the "app" menu.
		settings_menu->add_shortcut(ED_GET_SHORTCUT("editor/editor_settings"), EDITOR_OPEN_SETTINGS);
	}
#else
	settings_menu->add_shortcut(ED_GET_SHORTCUT("editor/editor_settings"), EDITOR_OPEN_SETTINGS);
#endif
	settings_menu->add_shortcut(ED_GET_SHORTCUT("editor/command_palette"), EDITOR_COMMAND_PALETTE);
	settings_menu->add_separator();

	settings_menu->add_submenu_node_item(TTRC("Editor Docks"), editor_dock_manager->get_docks_menu());

	if (!editor_layouts) {
		editor_layouts = memnew(PopupMenu);
		editor_layouts->connect(SceneStringName(id_pressed), callable_mp(this, &EditorNode::_layout_menu_option));
	}
	settings_menu->add_submenu_node_item(TTRC("Editor Layout"), editor_layouts);
	settings_menu->add_separator();

	settings_menu->add_shortcut(ED_GET_SHORTCUT("editor/take_screenshot"), EDITOR_TAKE_SCREENSHOT);
	settings_menu->set_item_tooltip(-1, TTRC("Screenshots are stored in the user data folder (\"user://\")."));

	settings_menu->add_shortcut(ED_GET_SHORTCUT("editor/fullscreen_mode"), EDITOR_TOGGLE_FULLSCREEN);
	settings_menu->add_separator();

#ifndef ANDROID_ENABLED
	if (EditorPaths::get_singleton()->get_data_dir() == EditorPaths::get_singleton()->get_config_dir()) {
		// Configuration and data folders are located in the same place.
		settings_menu->add_item(TTRC("Open Editor Data/Settings Folder"), EDITOR_OPEN_DATA_FOLDER);
	} else {
		// Separate configuration and data folders.
		settings_menu->add_item(TTRC("Open Editor Data Folder"), EDITOR_OPEN_DATA_FOLDER);
		settings_menu->add_item(TTRC("Open Editor Settings Folder"), EDITOR_OPEN_CONFIG_FOLDER);
	}
	settings_menu->add_separator();
#endif

	settings_menu->add_item(TTRC("Manage Editor Features..."), EDITOR_MANAGE_FEATURE_PROFILES);
	settings_menu->add_item(TTRC("Manage Export Templates..."), EDITOR_MANAGE_EXPORT_TEMPLATES);
#if !defined(ANDROID_ENABLED) && !defined(WEB_ENABLED)
	settings_menu->add_item(TTRC("Configure FBX Importer..."), EDITOR_CONFIGURE_FBX_IMPORTER);
#endif
}

void EditorNode::_build_help_menu() {
	if (!help_menu) {
		return;
	}
	help_menu->clear(false);

	if (menu_type == MENU_TYPE_GLOBAL && NativeMenu::get_singleton()->has_system_menu(NativeMenu::HELP_MENU_ID)) {
		help_menu->set_system_menu(NativeMenu::HELP_MENU_ID);
	} else {
		help_menu->set_system_menu(NativeMenu::INVALID_MENU_ID);
	}
	bool dark_mode = DisplayServer::get_singleton()->is_dark_mode_supported() && DisplayServer::get_singleton()->is_dark_mode();
	help_menu->add_icon_shortcut(get_editor_theme_native_menu_icon(SNAME("HelpSearch"), menu_type == MENU_TYPE_GLOBAL, dark_mode), ED_GET_SHORTCUT("editor/editor_help"), HELP_SEARCH);
	help_menu->add_separator();
	help_menu->add_shortcut(ED_GET_SHORTCUT("editor/online_docs"), HELP_DOCS);
	help_menu->add_shortcut(ED_GET_SHORTCUT("editor/forum"), HELP_FORUM);
	help_menu->add_shortcut(ED_GET_SHORTCUT("editor/community"), HELP_COMMUNITY);
	help_menu->add_separator();
	help_menu->add_icon_shortcut(get_editor_theme_native_menu_icon(SNAME("ActionCopy"), menu_type == MENU_TYPE_GLOBAL, dark_mode), ED_GET_SHORTCUT("editor/copy_system_info"), HELP_COPY_SYSTEM_INFO);
	help_menu->set_item_tooltip(-1, TTRC("Copies the system info as a single-line text into the clipboard."));
	help_menu->add_shortcut(ED_GET_SHORTCUT("editor/report_a_bug"), HELP_REPORT_A_BUG);
	help_menu->add_shortcut(ED_GET_SHORTCUT("editor/suggest_a_feature"), HELP_SUGGEST_A_FEATURE);
	help_menu->add_shortcut(ED_GET_SHORTCUT("editor/send_docs_feedback"), HELP_SEND_DOCS_FEEDBACK);
	help_menu->add_separator();
#ifdef MACOS_ENABLED
	if (menu_type != MENU_TYPE_GLOBAL) {
		// On macOS "About" option is in the "app" menu.
		help_menu->add_icon_shortcut(get_editor_theme_native_menu_icon(SNAME("Godot"), menu_type == MENU_TYPE_GLOBAL, dark_mode), ED_GET_SHORTCUT("editor/about"), HELP_ABOUT);
	}
#else
	help_menu->add_icon_shortcut(get_editor_theme_native_menu_icon(SNAME("Godot"), menu_type == MENU_TYPE_GLOBAL, dark_mode), ED_GET_SHORTCUT("editor/about"), HELP_ABOUT);
#endif
	help_menu->add_icon_shortcut(get_editor_theme_native_menu_icon(SNAME("Heart"), menu_type == MENU_TYPE_GLOBAL, dark_mode), ED_GET_SHORTCUT("editor/support_development"), HELP_SUPPORT_GODOT_DEVELOPMENT);
}

void EditorNode::_add_to_main_menu(const String &p_name, PopupMenu *p_menu) {
	p_menu->set_name(p_name);
	main_menu_items.push_back(p_menu);
}

void EditorNode::_update_main_menu_type() {
	bool can_expand = bool(EDITOR_GET("interface/editor/expand_to_title")) && DisplayServer::get_singleton()->has_feature(DisplayServer::FEATURE_EXTEND_TO_TITLE);
	bool use_menu_button = EDITOR_GET("interface/editor/collapse_main_menu");
	bool global_menu = !bool(EDITOR_GET("interface/editor/use_embedded_menu")) && NativeMenu::get_singleton()->has_feature(NativeMenu::FEATURE_GLOBAL_MENU);
	MenuType new_menu_type;
	if (global_menu) {
		new_menu_type = MENU_TYPE_GLOBAL;
	} else if (use_menu_button) {
		new_menu_type = MENU_TYPE_COMPACT;
	} else {
		new_menu_type = MENU_TYPE_FULL;
	}

	if (new_menu_type == menu_type) {
		return; // Nothing to do.
	}
	menu_type = new_menu_type;

	// Update menu items.
	_build_file_menu();
	_build_project_menu();
	_build_settings_menu();
	_build_help_menu();

	// Delete all menu.
	if (main_menu_bar) {
		for (PopupMenu *menu : main_menu_items) {
			if (menu->get_parent() == main_menu_bar) {
				main_menu_bar->remove_child(menu);
			}
		}
		memdelete(main_menu_bar);
		main_menu_bar = nullptr;
	}
	if (main_menu_button) {
		PopupMenu *popup = main_menu_button->get_popup();
		popup->clear(false);
		for (PopupMenu *menu : main_menu_items) {
			if (menu->get_parent() == popup) {
				popup->remove_child(menu);
			}
		}
		memdelete(main_menu_button);
		main_menu_button = nullptr;
	}
	memdelete_notnull(menu_btn_spacer);
	menu_btn_spacer = nullptr;

	// Create new menu.
	if (new_menu_type == MENU_TYPE_COMPACT) {
		main_menu_button = memnew(MenuButton);
		main_menu_button->set_text(TTRC("Main Menu"));
		main_menu_button->set_theme_type_variation("MainScreenButton");
		main_menu_button->set_focus_mode(Control::FOCUS_NONE);
		if (is_inside_tree()) {
			main_menu_button->set_button_icon(theme->get_icon(SNAME("TripleBar"), EditorStringName(EditorIcons)));
		}
		main_menu_button->set_switch_on_hover(true);

		for (PopupMenu *menu : main_menu_items) {
			if (menu != apple_menu) {
				main_menu_button->get_popup()->add_submenu_node_item(menu->get_name(), menu);
			}
		}

#ifdef ANDROID_ENABLED
		// Align main menu icon visually with TouchActionsPanel buttons.
		menu_btn_spacer = memnew(Control);
		menu_btn_spacer->set_custom_minimum_size(Vector2(8, 0) * EDSCALE);
		title_bar->add_child(menu_btn_spacer);
		title_bar->move_child(menu_btn_spacer, left_menu_spacer ? left_menu_spacer->get_index() + 1 : 0);
#endif
		title_bar->add_child(main_menu_button);
		if (menu_btn_spacer == nullptr) {
			title_bar->move_child(main_menu_button, left_menu_spacer ? left_menu_spacer->get_index() + 1 : 0);
		} else {
			title_bar->move_child(main_menu_button, menu_btn_spacer->get_index() + 1);
		}
	} else {
		main_menu_bar = memnew(MenuBar);
		main_menu_bar->set_mouse_filter(Control::MOUSE_FILTER_STOP);
		main_menu_bar->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
		main_menu_bar->set_theme_type_variation("MainMenuBar");
		main_menu_bar->set_start_index(0); // Main menu, add to the start of global menu.
		main_menu_bar->set_prefer_global_menu(menu_type == MENU_TYPE_GLOBAL);
		main_menu_bar->set_switch_on_hover(true);

		for (PopupMenu *menu : main_menu_items) {
			if (menu != apple_menu || menu_type == MENU_TYPE_GLOBAL) {
				main_menu_bar->add_child(menu);
			}
		}

		title_bar->add_child(main_menu_bar);
		title_bar->move_child(main_menu_bar, left_menu_spacer ? left_menu_spacer->get_index() + 1 : 0);
	}

	// Show/hide project title.
	if (project_title) {
		project_title->set_visible(can_expand && menu_type == MENU_TYPE_GLOBAL);
	}
}

