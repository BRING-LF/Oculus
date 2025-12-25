/**************************************************************************/
/*  editor_node_layout.cpp                                               */
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

#include "core/config/engine.h"
#include "core/io/config_file.h"
#include "core/io/file_access.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/editor_bottom_panel.h"
#include "editor/debugger/script_editor_debugger.h"
#include "editor/editor_data.h"
#include "editor/editor_main_screen.h"
#include "editor/settings/editor_settings.h"
#include "scene/gui/tree.h"
#include "scene/main/timer.h"

static const String EDITOR_NODE_CONFIG_SECTION = "EditorNode";

void EditorNode::_save_editor_layout() {
	if (!load_editor_layout_done) {
		return;
	}
	Ref<ConfigFile> config;
	config.instantiate();
	// Load and amend existing config if it exists.
	config->load(EditorPaths::get_singleton()->get_project_settings_dir().path_join("editor_layout.cfg"));

	editor_dock_manager->save_docks_to_config(config, "docks");
	_save_open_scenes_to_config(config);
	_save_central_editor_layout_to_config(config);
	_save_window_settings_to_config(config, "EditorWindow");
	editor_data.get_plugin_window_layout(config);

	config->save(EditorPaths::get_singleton()->get_project_settings_dir().path_join("editor_layout.cfg"));
}

void EditorNode::_save_open_scenes_to_config(Ref<ConfigFile> p_layout) {
	PackedStringArray scenes;
	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		String path = editor_data.get_scene_path(i);
		if (path.is_empty()) {
			continue;
		}
		scenes.push_back(path);
	}
	p_layout->set_value(EDITOR_NODE_CONFIG_SECTION, "open_scenes", scenes);

	String currently_edited_scene_path = editor_data.get_scene_path(editor_data.get_edited_scene());
	p_layout->set_value(EDITOR_NODE_CONFIG_SECTION, "current_scene", currently_edited_scene_path);
}

void EditorNode::save_editor_layout_delayed() {
	editor_layout_save_delay_timer->start();
}

void EditorNode::_load_editor_layout() {
	EditorProgress ep("loading_editor_layout", TTR("Loading editor"), 5);
	ep.step(TTR("Loading editor layout..."), 0, true);
	Ref<ConfigFile> config;
	config.instantiate();
	Error err = config->load(EditorPaths::get_singleton()->get_project_settings_dir().path_join("editor_layout.cfg"));
	if (err != OK) { // No config.
		// If config is not found, expand the res:// folder and favorites by default.
		TreeItem *root = FileSystemDock::get_singleton()->get_tree_control()->get_item_with_metadata("res://", 0);
		if (root) {
			root->set_collapsed(false);
		}

		TreeItem *favorites = FileSystemDock::get_singleton()->get_tree_control()->get_item_with_metadata("Favorites", 0);
		if (favorites) {
			favorites->set_collapsed(false);
		}

		if (overridden_default_layout >= 0) {
			_layout_menu_option(overridden_default_layout);
		} else {
			ep.step(TTR("Loading docks..."), 1, true);
			// Initialize some default values.
			bottom_panel->load_layout_from_config(default_layout, EDITOR_NODE_CONFIG_SECTION);
		}
	} else {
		ep.step(TTR("Loading docks..."), 1, true);
		editor_dock_manager->load_docks_from_config(config, "docks", true);

		ep.step(TTR("Reopening scenes..."), 2, true);
		_load_open_scenes_from_config(config);

		ep.step(TTR("Loading central editor layout..."), 3, true);
		_load_central_editor_layout_from_config(config);

		ep.step(TTR("Loading plugin window layout..."), 4, true);
		editor_data.set_plugin_window_layout(config);

		ep.step(TTR("Editor layout ready."), 5, true);
	}
	load_editor_layout_done = true;
}

void EditorNode::_save_central_editor_layout_to_config(Ref<ConfigFile> p_config_file) {
	// Bottom panel.

	bottom_panel->save_layout_to_config(p_config_file, EDITOR_NODE_CONFIG_SECTION);

	// Debugger tab.

	int selected_default_debugger_tab_idx = EditorDebuggerNode::get_singleton()->get_default_debugger()->get_current_debugger_tab();
	p_config_file->set_value(EDITOR_NODE_CONFIG_SECTION, "selected_default_debugger_tab_idx", selected_default_debugger_tab_idx);

	// Main editor (plugin).

	editor_main_screen->save_layout_to_config(p_config_file, EDITOR_NODE_CONFIG_SECTION);
}

void EditorNode::_load_central_editor_layout_from_config(Ref<ConfigFile> p_config_file) {
	// Bottom panel.

	bottom_panel->load_layout_from_config(p_config_file, EDITOR_NODE_CONFIG_SECTION);

	// Debugger tab.

	if (p_config_file->has_section_key(EDITOR_NODE_CONFIG_SECTION, "selected_default_debugger_tab_idx")) {
		int selected_default_debugger_tab_idx = p_config_file->get_value(EDITOR_NODE_CONFIG_SECTION, "selected_default_debugger_tab_idx");
		EditorDebuggerNode::get_singleton()->get_default_debugger()->switch_to_debugger(selected_default_debugger_tab_idx);
	}

	// Main editor (plugin).

	editor_main_screen->load_layout_from_config(p_config_file, EDITOR_NODE_CONFIG_SECTION);
}

void EditorNode::_save_window_settings_to_config(Ref<ConfigFile> p_layout, const String &p_section) {
	Window *w = get_window();
	if (w) {
		p_layout->set_value(p_section, "screen", w->get_current_screen());

		Window::Mode mode = w->get_mode();
		switch (mode) {
			case Window::MODE_WINDOWED:
				p_layout->set_value(p_section, "mode", "windowed");
				p_layout->set_value(p_section, "size", w->get_size());
				break;
			case Window::MODE_FULLSCREEN:
			case Window::MODE_EXCLUSIVE_FULLSCREEN:
				p_layout->set_value(p_section, "mode", "fullscreen");
				break;
			case Window::MODE_MINIMIZED:
				if (was_window_windowed_last) {
					p_layout->set_value(p_section, "mode", "windowed");
					p_layout->set_value(p_section, "size", w->get_size());
				} else {
					p_layout->set_value(p_section, "mode", "maximized");
				}
				break;
			default:
				p_layout->set_value(p_section, "mode", "maximized");
				break;
		}

		p_layout->set_value(p_section, "position", w->get_position());
	}
}

void EditorNode::_load_open_scenes_from_config(Ref<ConfigFile> p_layout) {
	if (Engine::get_singleton()->is_recovery_mode_hint()) {
		return;
	}

	if (!bool(EDITOR_GET("interface/scene_tabs/restore_scenes_on_load"))) {
		return;
	}

	if (!p_layout->has_section(EDITOR_NODE_CONFIG_SECTION) ||
			!p_layout->has_section_key(EDITOR_NODE_CONFIG_SECTION, "open_scenes")) {
		return;
	}

	restoring_scenes = true;

	PackedStringArray scenes = p_layout->get_value(EDITOR_NODE_CONFIG_SECTION, "open_scenes");
	for (int i = 0; i < scenes.size(); i++) {
		if (FileAccess::exists(scenes[i])) {
			load_scene(scenes[i]);
		}
	}

	if (p_layout->has_section_key(EDITOR_NODE_CONFIG_SECTION, "current_scene")) {
		String current_scene = p_layout->get_value(EDITOR_NODE_CONFIG_SECTION, "current_scene");
		for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
			if (editor_data.get_scene_path(i) == current_scene) {
				_set_current_scene(i);
				break;
			}
		}
	}

	save_editor_layout_delayed();

	restoring_scenes = false;
}

bool EditorNode::has_scenes_in_session() {
	if (!bool(EDITOR_GET("interface/scene_tabs/restore_scenes_on_load"))) {
		return false;
	}
	Ref<ConfigFile> config;
	config.instantiate();
	Error err = config->load(EditorPaths::get_singleton()->get_project_settings_dir().path_join("editor_layout.cfg"));
	if (err != OK) {
		return false;
	}
	if (!config->has_section(EDITOR_NODE_CONFIG_SECTION) || !config->has_section_key(EDITOR_NODE_CONFIG_SECTION, "open_scenes")) {
		return false;
	}
	Array scenes = config->get_value(EDITOR_NODE_CONFIG_SECTION, "open_scenes");
	return !scenes.is_empty();
}

