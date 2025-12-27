/**************************************************************************/
/*  editor_node_init_menus.cpp                                           */
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

#include "editor/gui/editor_title_bar.h"
#include "editor/settings/editor_settings.h"
#include "servers/display/display_server.h"
#include "scene/gui/popup_menu.h"

#ifdef MACOS_ENABLED
#include "platform/macos/native_menu.h"
#endif

void EditorNode::_init_menus() {
	// Editor menu and toolbar.
	bool can_expand = bool(EDITOR_GET("interface/editor/expand_to_title")) && DisplayServer::get_singleton()->has_feature(DisplayServer::FEATURE_EXTEND_TO_TITLE);

#ifdef MACOS_ENABLED
	if (NativeMenu::get_singleton()->has_system_menu(NativeMenu::APPLICATION_MENU_ID)) {
		apple_menu = memnew(PopupMenu);
		apple_menu->set_system_menu(NativeMenu::APPLICATION_MENU_ID);
		_add_to_main_menu("", apple_menu);

		apple_menu->add_shortcut(ED_GET_SHORTCUT("editor/editor_settings"), EDITOR_OPEN_SETTINGS);
		apple_menu->add_separator();
		apple_menu->connect(SceneStringName(id_pressed), callable_mp(this, &EditorNode::_menu_option));
	}
#endif

	if (can_expand) {
		// Add spacer to avoid other controls under window minimize/maximize/close buttons (left side).
		left_menu_spacer = memnew(Control);
		left_menu_spacer->set_mouse_filter(Control::MOUSE_FILTER_PASS);
		title_bar->add_child(left_menu_spacer);
	}

	file_menu = memnew(PopupMenu);
	file_menu->connect(SceneStringName(id_pressed), callable_mp(this, &EditorNode::_menu_option));
	file_menu->connect("about_to_popup", callable_mp(this, &EditorNode::_update_file_menu_opened));
	_add_to_main_menu(TTRC("Scene"), file_menu);

	project_menu = memnew(PopupMenu);
	project_menu->connect(SceneStringName(id_pressed), callable_mp(this, &EditorNode::_menu_option));
	_add_to_main_menu(TTRC("Project"), project_menu);

	debug_menu = memnew(PopupMenu);
	// Options are added and handled by DebuggerEditorPlugin, do not rebuild.
	_add_to_main_menu(TTRC("Debug"), debug_menu);

	settings_menu = memnew(PopupMenu);
	settings_menu->connect(SceneStringName(id_pressed), callable_mp(this, &EditorNode::_menu_option));
	_add_to_main_menu(TTRC("Editor"), settings_menu);

	help_menu = memnew(PopupMenu);
	help_menu->connect(SceneStringName(id_pressed), callable_mp(this, &EditorNode::_menu_option));
	_add_to_main_menu(TTRC("Help"), help_menu);

	_update_main_menu_type();
}

