/**************************************************************************/
/*  editor_node_notifications.cpp                                        */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/extension/gdextension_manager.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "core/string/translation_server.h"
#include "core/version.h"
#include "editor/doc/editor_help.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_data.h"
#include "editor/editor_log.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/editor_bottom_panel.h"
#include "editor/gui/editor_toaster.h"
#include "editor/gui/progress_dialog.h"
#include "editor/import/resource_importer_texture.h"
#include "editor/run/editor_run_bar.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/scene/editor_scene_tabs.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_feature_profile.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_theme_manager.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/option_button.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server.h"

#if defined(MODULE_GDSCRIPT_ENABLED) || defined(MODULE_MONO_ENABLED)
#include "editor/doc/editor_help_highlighter.h"
#endif

#ifdef ANDROID_ENABLED
#include "editor/gui/touch_actions_panel.h"
#endif

void EditorNode::_notification_translation_changed() {
	_update_title();
	callable_mp(this, &EditorNode::_titlebar_resized).call_deferred();

	// The rendering method selector.
	const String current_renderer_ps = String(GLOBAL_GET("rendering/renderer/rendering_method")).to_lower();
	const String current_renderer_os = OS::get_singleton()->get_current_rendering_method().to_lower();
	if (current_renderer_ps == current_renderer_os) {
		for (int i = 0; i < renderer->get_item_count(); i++) {
			renderer->set_item_text(i, _to_rendering_method_display_name(renderer->get_item_metadata(i)));
		}
	} else {
		// TRANSLATORS: The placeholder is the rendering method that has overridden the default one.
		renderer->set_item_text(0, vformat(TTR("%s (Overridden)"), _to_rendering_method_display_name(current_renderer_os)));
	}
}

void EditorNode::_notification_postinitialize() {
	EditorHelp::generate_doc();
#if defined(MODULE_GDSCRIPT_ENABLED) || defined(MODULE_MONO_ENABLED)
	EditorHelpHighlighter::create_singleton();
#endif
}

void EditorNode::_notification_process() {
	if (editor_data.is_scene_changed(-1)) {
		scene_tabs->update_scene_tabs();
	}

	// Update the animation frame of the update spinner.
	uint64_t frame = Engine::get_singleton()->get_frames_drawn();
	uint64_t tick = OS::get_singleton()->get_ticks_msec();

	if (frame != update_spinner_step_frame && (tick - update_spinner_step_msec) > (1000 / 8)) {
		update_spinner_step++;
		if (update_spinner_step >= 8) {
			update_spinner_step = 0;
		}

		update_spinner_step_msec = tick;
		update_spinner_step_frame = frame + 1;

		// Update the icon itself only when the spinner is visible.
		if (_should_display_update_spinner()) {
			update_spinner->set_button_icon(theme->get_icon("Progress" + itos(update_spinner_step + 1), EditorStringName(EditorIcons)));
		}
	}

	editor_selection->update();

	ResourceImporterTexture::get_singleton()->update_imports();

	if (requested_first_scan) {
		requested_first_scan = false;

		OS::get_singleton()->benchmark_begin_measure("Editor", "First Scan");

		EditorFileSystem::get_singleton()->connect("filesystem_changed", callable_mp(this, &EditorNode::_execute_upgrades), CONNECT_ONE_SHOT);
		EditorFileSystem::get_singleton()->scan();
	}

	if (settings_overrides_changed) {
		EditorSettings::get_singleton()->notify_changes();
		EditorSettings::get_singleton()->emit_signal(SNAME("settings_changed"));
		settings_overrides_changed = false;
	}
}

void EditorNode::_notification_enter_tree() {
	get_tree()->set_disable_node_threading(true); // No node threading while running editor.

	Engine::get_singleton()->set_editor_hint(true);

	Window *window = get_window();
	if (window) {
		// Handle macOS fullscreen and extend-to-title changes.
		window->connect("titlebar_changed", callable_mp(this, &EditorNode::_titlebar_resized));
	}

	// Theme has already been created in the constructor, so we can skip that step.
	_update_theme(true);

	OS::get_singleton()->set_low_processor_usage_mode_sleep_usec(int(EDITOR_GET("interface/editor/low_processor_mode_sleep_usec")));
	get_tree()->get_root()->set_as_audio_listener_3d(false);
	get_tree()->get_root()->set_as_audio_listener_2d(false);
	get_tree()->get_root()->set_snap_2d_transforms_to_pixel(false);
	get_tree()->get_root()->set_snap_2d_vertices_to_pixel(false);
	get_tree()->set_auto_accept_quit(false);
#ifdef ANDROID_ENABLED
	get_tree()->set_quit_on_go_back(false);
	bool is_fullscreen = EDITOR_DEF("_is_editor_fullscreen", false);
	if (is_fullscreen) {
		DisplayServer::get_singleton()->window_set_mode(DisplayServer::WINDOW_MODE_FULLSCREEN);
	}
#endif
	get_tree()->get_root()->connect("files_dropped", callable_mp(this, &EditorNode::_dropped_files));

	command_palette->register_shortcuts_as_command();

	_begin_first_scan();

	last_dark_mode_state = DisplayServer::get_singleton()->is_dark_mode();
	last_system_accent_color = DisplayServer::get_singleton()->get_accent_color();
	last_system_base_color = DisplayServer::get_singleton()->get_base_color();
	DisplayServer::get_singleton()->set_system_theme_change_callback(callable_mp(this, &EditorNode::_check_system_theme_changed));

	get_viewport()->connect("size_changed", callable_mp(this, &EditorNode::_viewport_resized));

	/* DO NOT LOAD SCENES HERE, WAIT FOR FILE SCANNING AND REIMPORT TO COMPLETE */
}

void EditorNode::_notification_exit_tree() {
	singleton->active_plugins.clear();

	if (progress_dialog) {
		progress_dialog->queue_free();
	}
	if (load_error_dialog) {
		load_error_dialog->queue_free();
	}
	if (execute_output_dialog) {
		execute_output_dialog->queue_free();
	}
	if (warning) {
		warning->queue_free();
	}
	if (accept) {
		accept->queue_free();
	}
	if (save_accept) {
		save_accept->queue_free();
	}
	EditorHelp::save_script_doc_cache();
	editor_data.save_editor_external_data();
	EditorSettings::get_singleton()->save_project_metadata();
	FileAccess::set_file_close_fail_notify_callback(nullptr);
	log->deinit(); // Do not get messages anymore.
	editor_data.clear_edited_scenes();
	get_viewport()->disconnect("size_changed", callable_mp(this, &EditorNode::_viewport_resized));
}

void EditorNode::_notification_ready() {
	started_timestamp = Time::get_singleton()->get_unix_time_from_system();

	// Store the default order of bottom docks. It can only be determined dynamically.
	PackedStringArray bottom_docks;
	bottom_docks.reserve_exact(bottom_panel->get_tab_count());
	for (int i = 0; i < bottom_panel->get_tab_count(); i++) {
		EditorDock *dock = Object::cast_to<EditorDock>(bottom_panel->get_tab_control(i));
		bottom_docks.append(dock->get_effective_layout_key());
	}
	default_layout->set_value("docks", "dock_9", String(",").join(bottom_docks));

	RenderingServer::get_singleton()->viewport_set_disable_2d(get_scene_root()->get_viewport_rid(), true);
	RenderingServer::get_singleton()->viewport_set_environment_mode(get_viewport()->get_viewport_rid(), RenderingServer::VIEWPORT_ENVIRONMENT_DISABLED);
	DisplayServer::get_singleton()->screen_set_keep_on(EDITOR_GET("interface/editor/keep_screen_on"));

	feature_profile_manager->notify_changed();

	// Save the project after opening to mark it as last modified, except in headless mode.
	// Also use this opportunity to ensure default settings are applied to new projects created from the command line
	// using `touch project.godot`.
	if (DisplayServer::get_singleton()->window_can_draw()) {
		const String project_settings_path = ProjectSettings::get_singleton()->get_resource_path().path_join("project.godot");
		// Check the file's size in bytes as an optimization. If it's under 10 bytes, the file is assumed to be empty.
		if (FileAccess::get_size(project_settings_path) < 10) {
			const HashMap<String, Variant> initial_settings = get_initial_settings();
			for (const KeyValue<String, Variant> &initial_setting : initial_settings) {
				ProjectSettings::get_singleton()->set_setting(initial_setting.key, initial_setting.value);
			}
		}
		ProjectSettings::get_singleton()->save();
	}

	_titlebar_resized();

	// Set up a theme context for the 2D preview viewport using the stored preview theme.
	CanvasItemEditor::ThemePreviewMode theme_preview_mode = (CanvasItemEditor::ThemePreviewMode)(int)EditorSettings::get_singleton()->get_project_metadata("2d_editor", "theme_preview", CanvasItemEditor::THEME_PREVIEW_PROJECT);
	update_preview_themes(theme_preview_mode);

	// Remember the selected locale to preview node translations.
	const String preview_locale = EditorSettings::get_singleton()->get_project_metadata("editor_metadata", "preview_locale", String());
	if (!preview_locale.is_empty() && TranslationServer::get_singleton()->has_translation_for_locale(preview_locale, true)) {
		set_preview_locale(preview_locale);
	}

	if (Engine::get_singleton()->is_recovery_mode_hint()) {
		EditorToaster::get_singleton()->popup_str(TTR("Recovery Mode is enabled. Editor functionality has been restricted."), EditorToaster::SEVERITY_WARNING);
	}

	/* DO NOT LOAD SCENES HERE, WAIT FOR FILE SCANNING AND REIMPORT TO COMPLETE */
}

void EditorNode::_notification_application_focus_in() {
	// Restore the original FPS cap after focusing back on the editor.
	OS::get_singleton()->set_low_processor_usage_mode_sleep_usec(int(EDITOR_GET("interface/editor/low_processor_mode_sleep_usec")));

	if (_is_project_data_missing()) {
		project_data_missing->popup_centered();
	} else {
		EditorFileSystem::get_singleton()->scan_changes();
	}
	_scan_external_changes();

	GDExtensionManager *gdextension_manager = GDExtensionManager::get_singleton();
	callable_mp(gdextension_manager, &GDExtensionManager::reload_extensions).call_deferred();
}

void EditorNode::_notification_application_focus_out() {
	// Save on focus loss before applying the FPS limit to avoid slowing down the saving process.
	if (EDITOR_GET("interface/editor/save_on_focus_loss")) {
		_save_scene_silently();
	}

	// Set a low FPS cap to decrease CPU/GPU usage while the editor is unfocused.
	if (unfocused_low_processor_usage_mode_enabled) {
		OS::get_singleton()->set_low_processor_usage_mode_sleep_usec(int(EDITOR_GET("interface/editor/unfocused_low_processor_mode_sleep_usec")));
	}
}

void EditorNode::_notification_wm_about() {
	show_about();
}

void EditorNode::_notification_wm_close_request() {
	_menu_option_confirm(SCENE_QUIT, false);
}

void EditorNode::_notification_editor_settings_changed() {
	if (EditorSettings::get_singleton()->check_changed_settings_in_group("filesystem/file_dialog")) {
		FileDialog::set_default_show_hidden_files(EDITOR_GET("filesystem/file_dialog/show_hidden_files"));
		FileDialog::set_default_display_mode(EDITOR_GET("filesystem/file_dialog/display_mode"));
	}

	if (EditorSettings::get_singleton()->check_changed_settings_in_group("interface/editor/tablet_driver")) {
		String tablet_driver = GLOBAL_GET("input_devices/pen_tablet/driver");
		int tablet_driver_idx = EDITOR_GET("interface/editor/tablet_driver");
		if (tablet_driver_idx != -1) {
			tablet_driver = DisplayServer::get_singleton()->tablet_get_driver_name(tablet_driver_idx);
		}
		if (tablet_driver.is_empty()) {
			tablet_driver = DisplayServer::get_singleton()->tablet_get_driver_name(0);
		}
		DisplayServer::get_singleton()->tablet_set_current_driver(tablet_driver);
		print_verbose("Using \"" + DisplayServer::get_singleton()->tablet_get_current_driver() + "\" pen tablet driver...");
	}

	if (EDITOR_GET("interface/editor/import_resources_when_unfocused")) {
		scan_changes_timer->start();
	} else {
		scan_changes_timer->stop();
	}

	follow_system_theme = EDITOR_GET("interface/theme/follow_system_theme");
	use_system_accent_color = EDITOR_GET("interface/theme/use_system_accent_color");

	if (EditorThemeManager::is_generated_theme_outdated()) {
		class_icon_cache.clear();
		_update_theme();
		_build_icon_type_cache();
		recent_scenes->reset_size();
	}

	if (EditorSettings::get_singleton()->check_changed_settings_in_group("interface/editor")) {
		theme->set_constant("dragging_unfold_wait_msec", "Tree", (float)EDITOR_GET("interface/editor/dragging_hover_wait_seconds") * 1000);
		theme->set_constant("hover_switch_wait_msec", "TabBar", (float)EDITOR_GET("interface/editor/dragging_hover_wait_seconds") * 1000);
		editor_dock_manager->update_tab_styles();
	}

	if (EditorSettings::get_singleton()->check_changed_settings_in_group("interface/scene_tabs")) {
		scene_tabs->update_scene_tabs();
	}

	if (EditorSettings::get_singleton()->check_changed_settings_in_group("docks/filesystem")) {
		HashSet<String> updated_textfile_extensions;
		HashSet<String> updated_other_file_extensions;
		bool extensions_match = true;
		const Vector<String> textfile_ext = ((String)(EDITOR_GET("docks/filesystem/textfile_extensions"))).split(",", false);
		for (const String &E : textfile_ext) {
			updated_textfile_extensions.insert(E);
			if (extensions_match && !textfile_extensions.has(E)) {
				extensions_match = false;
			}
		}
		const Vector<String> other_file_ext = ((String)(EDITOR_GET("docks/filesystem/other_file_extensions"))).split(",", false);
		for (const String &E : other_file_ext) {
			updated_other_file_extensions.insert(E);
			if (extensions_match && !other_file_extensions.has(E)) {
				extensions_match = false;
			}
		}

		if (!extensions_match || updated_textfile_extensions.size() < textfile_extensions.size() || updated_other_file_extensions.size() < other_file_extensions.size()) {
			textfile_extensions = updated_textfile_extensions;
			other_file_extensions = updated_other_file_extensions;
			EditorFileSystem::get_singleton()->scan();
		}
	}

	if (EditorSettings::get_singleton()->check_changed_settings_in_group("interface/editor")) {
		_update_update_spinner();
		_update_vsync_mode();
		_update_main_menu_type();
		DisplayServer::get_singleton()->screen_set_keep_on(EDITOR_GET("interface/editor/keep_screen_on"));
	}

#if defined(MODULE_GDSCRIPT_ENABLED) || defined(MODULE_MONO_ENABLED)
	if (EditorSettings::get_singleton()->check_changed_settings_in_group("text_editor/theme/highlighting")) {
		EditorHelpHighlighter::get_singleton()->reset_cache();
	}
#endif
#ifdef ANDROID_ENABLED
	if (EditorSettings::get_singleton()->check_changed_settings_in_group("interface/touchscreen/touch_actions_panel")) {
		_touch_actions_panel_mode_changed();
	}
#endif
}
