/**************************************************************************/
/*  editor_node.cpp                                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
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
#include "core/extension/gdextension_manager.h"
#include "core/input/input.h"
#include "core/io/config_file.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/class_db.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "core/string/print_string.h"
#include "core/string/translation_server.h"
#include "core/version.h"
#include "editor/editor_string_names.h"
#include "editor/inspector/editor_context_menu_plugin.h"
#include "editor/plugins/editor_plugin_list.h"
#include "main/main.h"
#include "scene/2d/node_2d.h"
#include "scene/3d/bone_attachment_3d.h"
#include "scene/animation/animation_tree.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/menu_bar.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/panel.h"
#include "scene/gui/popup.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"
#include "scene/main/timer.h"
#include "scene/main/window.h"
#include "scene/property_utils.h"
#include "scene/resources/dpi_texture.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/portable_compressed_texture.h"
#include "scene/theme/theme_db.h"
#include "servers/display/display_server.h"
#include "servers/navigation_2d/navigation_server_2d.h"
#include "servers/navigation_3d/navigation_server_3d.h"
#include "servers/rendering/rendering_server.h"

#include "editor/animation/animation_player_editor_plugin.h"
#include "editor/asset_library/asset_library_editor_plugin.h"
#include "editor/audio/audio_stream_preview.h"
#include "editor/audio/editor_audio_buses.h"
#include "editor/debugger/debugger_editor_plugin.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/debugger/script_editor_debugger.h"
#include "editor/doc/editor_help.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/docks/groups_dock.h"
#include "editor/docks/history_dock.h"
#include "editor/docks/import_dock.h"
#include "editor/docks/inspector_dock.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/docks/signals_dock.h"
#include "editor/editor_data.h"
#include "editor/editor_interface.h"
#include "editor/editor_log.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/export/editor_export.h"
#include "editor/export/export_template_manager.h"
#include "editor/export/project_export.h"
#include "editor/export/project_zip_packer.h"
#include "editor/export/register_exporters.h"
#include "editor/file_system/dependency_editor.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/editor_about.h"
#include "editor/gui/editor_bottom_panel.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_quick_open_dialog.h"
#include "editor/gui/editor_title_bar.h"
#include "editor/gui/editor_toaster.h"
#include "editor/gui/progress_dialog.h"
#include "editor/gui/window_wrapper.h"
#include "editor/import/3d/editor_import_collada.h"
#include "editor/import/3d/resource_importer_obj.h"
#include "editor/import/3d/resource_importer_scene.h"
#include "editor/import/3d/scene_import_settings.h"
#include "editor/import/audio_stream_import_settings.h"
#include "editor/import/dynamic_font_import_settings.h"
#include "editor/import/fbx_importer_manager.h"
#include "editor/import/resource_importer_bitmask.h"
#include "editor/import/resource_importer_bmfont.h"
#include "editor/import/resource_importer_csv_translation.h"
#include "editor/import/resource_importer_dynamic_font.h"
#include "editor/import/resource_importer_image.h"
#include "editor/import/resource_importer_imagefont.h"
#include "editor/import/resource_importer_layered_texture.h"
#include "editor/import/resource_importer_shader_file.h"
#include "editor/import/resource_importer_svg.h"
#include "editor/import/resource_importer_texture.h"
#include "editor/import/resource_importer_texture_atlas.h"
#include "editor/import/resource_importer_wav.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/inspector/editor_preview_plugins.h"
#include "editor/inspector/editor_properties.h"
#include "editor/inspector/editor_property_name_processor.h"
#include "editor/inspector/editor_resource_picker.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/inspector/multi_node_edit.h"
#include "editor/plugins/editor_plugin.h"
#include "editor/plugins/editor_resource_conversion_plugin.h"
#include "editor/plugins/plugin_config_dialog.h"
#include "editor/project_upgrade/project_upgrade_tool.h"
#include "editor/run/editor_run.h"
#include "editor/run/editor_run_bar.h"
#include "editor/run/game_view_plugin.h"

// Forward declaration for get_game_view_plugin defined in editor_node_game_view.cpp
extern GameViewPluginBase *get_game_view_plugin();
#include "editor/scene/3d/material_3d_conversion_plugins.h"
#include "editor/scene/3d/mesh_library_editor_plugin.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/scene/3d/root_motion_editor_plugin.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/scene/editor_scene_tabs.h"
#include "editor/scene/material_editor_plugin.h"
#include "editor/scene/particle_process_material_editor_plugin.h"
#include "editor/script/editor_script.h"
#include "editor/script/script_text_editor.h"
#include "editor/script/text_editor.h"
#include "editor/settings/editor_build_profile.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_feature_profile.h"
#include "editor/settings/editor_layouts_dialog.h"
#include "editor/settings/editor_settings.h"
#include "editor/settings/editor_settings_dialog.h"
#include "editor/settings/project_settings_editor.h"
#include "editor/shader/editor_native_shader_source_visualizer.h"
#include "editor/shader/visual_shader_editor_plugin.h"
#include "editor/themes/editor_color_map.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "editor/translations/editor_translation_parser.h"
#include "editor/translations/packed_scene_translation_parser_plugin.h"
#include "editor/version_control/version_control_editor_plugin.h"


#include "modules/modules_enabled.gen.h" // For gdscript, mono.

#ifndef PHYSICS_2D_DISABLED
#include "servers/physics_2d/physics_server_2d.h"
#endif // PHYSICS_2D_DISABLED

#ifndef PHYSICS_3D_DISABLED
#include "servers/physics_3d/physics_server_3d.h"
#endif // PHYSICS_3D_DISABLED

#ifdef ANDROID_ENABLED
#include "editor/gui/touch_actions_panel.h"
#endif // ANDROID_ENABLED

#include <cstdlib>

EditorNode *EditorNode::singleton = nullptr;

static const String EDITOR_NODE_CONFIG_SECTION = "EditorNode";

static const String REMOVE_ANDROID_BUILD_TEMPLATE_MESSAGE = TTRC("The Android build template is already installed in this project and it won't be overwritten.\nRemove the \"%s\" directory manually before attempting this operation again.");
static const String INSTALL_ANDROID_BUILD_TEMPLATE_MESSAGE = TTRC("This will set up your project for gradle Android builds by installing the source template to \"%s\".\nNote that in order to make gradle builds instead of using pre-built APKs, the \"Use Gradle Build\" option should be enabled in the Android export preset.");

void EditorNode::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_TRANSLATION_CHANGED: {
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
		} break;

		case NOTIFICATION_POSTINITIALIZE: {
			EditorHelp::generate_doc();
#if defined(MODULE_GDSCRIPT_ENABLED) || defined(MODULE_MONO_ENABLED)
			EditorHelpHighlighter::create_singleton();
#endif
		} break;

		case NOTIFICATION_PROCESS: {
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
		} break;

		case NOTIFICATION_ENTER_TREE: {
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
		} break;

		case NOTIFICATION_EXIT_TREE: {
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
		} break;

		case NOTIFICATION_READY: {
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
		} break;

		case NOTIFICATION_APPLICATION_FOCUS_IN: {
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
		} break;

		case NOTIFICATION_APPLICATION_FOCUS_OUT: {
			// Save on focus loss before applying the FPS limit to avoid slowing down the saving process.
			if (EDITOR_GET("interface/editor/save_on_focus_loss")) {
				_save_scene_silently();
			}

			// Set a low FPS cap to decrease CPU/GPU usage while the editor is unfocused.
			if (unfocused_low_processor_usage_mode_enabled) {
				OS::get_singleton()->set_low_processor_usage_mode_sleep_usec(int(EDITOR_GET("interface/editor/unfocused_low_processor_mode_sleep_usec")));
			}
		} break;

		case NOTIFICATION_WM_ABOUT: {
			show_about();
		} break;

		case NOTIFICATION_WM_CLOSE_REQUEST: {
			_menu_option_confirm(SCENE_QUIT, false);
		} break;

		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
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
		} break;
	}
}





















Vector<EditorNodeInitCallback> EditorNode::_init_callbacks;


























EditorNode::EditorNode() {
	DEV_ASSERT(!singleton);
	singleton = this;

	// Detecting headless mode, that means the editor is running in command line.
	if (!DisplayServer::get_singleton()->window_can_draw()) {
		cmdline_mode = true;
	}

	// Forward declaration for _resource_get_edited_scene defined in editor_node_bind_methods.cpp
	extern Node *_resource_get_edited_scene();
	Resource::_get_local_scene_func = _resource_get_edited_scene;

	_init_servers();

	_init_resources();

	_init_connections();

	// Load settings.
	if (!EditorSettings::get_singleton()) {
		EditorSettings::create();
	}

	_init_shortcuts();

	FileAccess::set_backup_save(EDITOR_GET("filesystem/on_save/safe_save_on_backup_then_rename"));

	_update_vsync_mode();

	// Warm up the project upgrade tool as early as possible.
	project_upgrade_tool = memnew(ProjectUpgradeTool);
	run_project_upgrade_tool = EditorSettings::get_singleton()->get_project_metadata(project_upgrade_tool->META_PROJECT_UPGRADE_TOOL, project_upgrade_tool->META_RUN_ON_RESTART, false);
	if (run_project_upgrade_tool) {
		project_upgrade_tool->begin_upgrade();
	}

	_init_ui_settings();

	_init_importers();

	_init_inspector_plugins();

	_init_file_system();

	_init_export();

	ED_SHORTCUT("canvas_item_editor/pan_view", TTRC("Pan View"), Key::SPACE);

	_init_file_extensions();

	_init_preview();

	gui_base = memnew(Panel);
	add_child(gui_base);

	// Take up all screen.
	gui_base->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	gui_base->set_anchor(SIDE_RIGHT, Control::ANCHOR_END);
	gui_base->set_anchor(SIDE_BOTTOM, Control::ANCHOR_END);
	gui_base->set_end(Point2(0, 0));

	main_vbox = memnew(VBoxContainer);

#ifdef ANDROID_ENABLED
	base_vbox = memnew(VBoxContainer);
	base_vbox->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_MINSIZE, theme->get_constant(SNAME("window_border_margin"), EditorStringName(Editor)));

	title_bar = memnew(EditorTitleBar);
	base_vbox->add_child(title_bar);

	main_hbox = memnew(HBoxContainer);
	main_hbox->add_child(main_vbox);
	main_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	main_hbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	base_vbox->add_child(main_hbox);

	_touch_actions_panel_mode_changed();

	gui_base->add_child(base_vbox);
#else
	gui_base->add_child(main_vbox);

	title_bar = memnew(EditorTitleBar);
	main_vbox->add_child(title_bar);
#endif

	_init_dock();

	_init_timers();

	_init_scene_ui();

	_init_editor_shortcuts();

	_init_menus();

	_init_title_bar();

	_init_dialogs();

	_init_docks();

	_init_layout();

	_init_file_dialogs();

	audio_preview_gen = memnew(AudioStreamPreviewGenerator);
	add_child(audio_preview_gen);

	_init_plugins();

	_init_preview_and_conversion_plugins();

	_init_export_plugins();

	_init_final();
}

