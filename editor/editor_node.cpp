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
#include "editor/export/dedicated_server_export_plugin.h"
#include "editor/export/editor_export.h"
#include "editor/export/export_template_manager.h"
#include "editor/export/gdextension_export_plugin.h"
#include "editor/export/project_export.h"
#include "editor/export/project_zip_packer.h"
#include "editor/export/register_exporters.h"
#include "editor/export/shader_baker_export_plugin.h"
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

#ifdef VULKAN_ENABLED
#include "editor/shader/shader_baker/shader_baker_export_plugin_platform_vulkan.h"
#endif

#ifdef D3D12_ENABLED
#include "editor/shader/shader_baker/shader_baker_export_plugin_platform_d3d12.h"
#endif

#ifdef METAL_ENABLED
#include "editor/shader/shader_baker/shader_baker_export_plugin_platform_metal.h"
#endif

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

	SceneState::set_disable_placeholders(true);
	ResourceLoader::clear_translation_remaps(); // Using no remaps if in editor.
	ResourceLoader::set_create_missing_resources_if_class_unavailable(true);

	EditorPropertyNameProcessor *epnp = memnew(EditorPropertyNameProcessor);
	add_child(epnp);

	_init_connections();

	Ref<TranslationDomain> domain = TranslationServer::get_singleton()->get_main_domain();
	domain->set_enabled(false);
	domain->set_locale_override(TranslationServer::get_singleton()->get_fallback_locale());

	// Load settings.
	if (!EditorSettings::get_singleton()) {
		EditorSettings::create();
	}

	ED_SHORTCUT("editor/lock_selected_nodes", TTRC("Lock Selected Node(s)"), KeyModifierMask::CMD_OR_CTRL | Key::L);
	ED_SHORTCUT("editor/unlock_selected_nodes", TTRC("Unlock Selected Node(s)"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::L);
	ED_SHORTCUT("editor/group_selected_nodes", TTRC("Group Selected Node(s)"), KeyModifierMask::CMD_OR_CTRL | Key::G);
	ED_SHORTCUT("editor/ungroup_selected_nodes", TTRC("Ungroup Selected Node(s)"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::G);

	FileAccess::set_backup_save(EDITOR_GET("filesystem/on_save/safe_save_on_backup_then_rename"));

	_update_vsync_mode();

	// Warm up the project upgrade tool as early as possible.
	project_upgrade_tool = memnew(ProjectUpgradeTool);
	run_project_upgrade_tool = EditorSettings::get_singleton()->get_project_metadata(project_upgrade_tool->META_PROJECT_UPGRADE_TOOL, project_upgrade_tool->META_RUN_ON_RESTART, false);
	if (run_project_upgrade_tool) {
		project_upgrade_tool->begin_upgrade();
	}

	{
		bool agile_input_event_flushing = EDITOR_GET("input/buffering/agile_event_flushing");
		bool use_accumulated_input = EDITOR_GET("input/buffering/use_accumulated_input");

		Input::get_singleton()->set_agile_input_event_flushing(agile_input_event_flushing);
		Input::get_singleton()->set_use_accumulated_input(use_accumulated_input);
	}

	{
		int display_scale = EDITOR_GET("interface/editor/display_scale");

		switch (display_scale) {
			case 0:
				// Try applying a suitable display scale automatically.
				EditorScale::set_scale(EditorSettings::get_auto_display_scale());
				break;
			case 1:
				EditorScale::set_scale(0.75);
				break;
			case 2:
				EditorScale::set_scale(1.0);
				break;
			case 3:
				EditorScale::set_scale(1.25);
				break;
			case 4:
				EditorScale::set_scale(1.5);
				break;
			case 5:
				EditorScale::set_scale(1.75);
				break;
			case 6:
				EditorScale::set_scale(2.0);
				break;
			default:
				EditorScale::set_scale(EDITOR_GET("interface/editor/custom_display_scale"));
				break;
		}
	}

	// Define a minimum window size to prevent UI elements from overlapping or being cut off.
	Window *w = Object::cast_to<Window>(SceneTree::get_singleton()->get_root());
	if (w) {
		const Size2 minimum_size = Size2(1024, 600) * EDSCALE;
		w->set_min_size(minimum_size); // Calling it this early doesn't sync the property with DS.
		DisplayServer::get_singleton()->window_set_min_size(minimum_size);
	}

	FileDialog::set_default_show_hidden_files(EDITOR_GET("filesystem/file_dialog/show_hidden_files"));
	FileDialog::set_default_display_mode(EDITOR_GET("filesystem/file_dialog/display_mode"));

	int swap_cancel_ok = EDITOR_GET("interface/editor/accept_dialog_cancel_ok_buttons");
	if (swap_cancel_ok != 0) { // 0 is auto, set in register_scene based on DisplayServer.
		// Swap on means OK first.
		AcceptDialog::set_swap_cancel_ok(swap_cancel_ok == 2);
	}

	int ed_root_dir = EDITOR_GET("interface/editor/ui_layout_direction");
	Control::set_root_layout_direction(ed_root_dir);
	Window::set_root_layout_direction(ed_root_dir);

	ResourceLoader::set_abort_on_missing_resources(false);
	ResourceLoader::set_error_notify_func(&EditorNode::add_io_error);
	ResourceLoader::set_dependency_error_notify_func(&EditorNode::_dependency_error_report);

	SceneState::set_instantiation_warning_notify_func([](const String &p_warning) {
		add_io_warning(p_warning);
		callable_mp(EditorInterface::get_singleton(), &EditorInterface::mark_scene_as_unsaved).call_deferred();
	});

	{
		// Register importers at the beginning, so dialogs are created with the right extensions.
		Ref<ResourceImporterTexture> import_texture = memnew(ResourceImporterTexture(true));
		ResourceFormatImporter::get_singleton()->add_importer(import_texture);

		Ref<ResourceImporterLayeredTexture> import_cubemap;
		import_cubemap.instantiate();
		import_cubemap->set_mode(ResourceImporterLayeredTexture::MODE_CUBEMAP);
		ResourceFormatImporter::get_singleton()->add_importer(import_cubemap);

		Ref<ResourceImporterLayeredTexture> import_array;
		import_array.instantiate();
		import_array->set_mode(ResourceImporterLayeredTexture::MODE_2D_ARRAY);
		ResourceFormatImporter::get_singleton()->add_importer(import_array);

		Ref<ResourceImporterLayeredTexture> import_cubemap_array;
		import_cubemap_array.instantiate();
		import_cubemap_array->set_mode(ResourceImporterLayeredTexture::MODE_CUBEMAP_ARRAY);
		ResourceFormatImporter::get_singleton()->add_importer(import_cubemap_array);

		Ref<ResourceImporterLayeredTexture> import_3d = memnew(ResourceImporterLayeredTexture(true));
		import_3d->set_mode(ResourceImporterLayeredTexture::MODE_3D);
		ResourceFormatImporter::get_singleton()->add_importer(import_3d);

		Ref<ResourceImporterImage> import_image;
		import_image.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_image);

		Ref<ResourceImporterSVG> import_svg;
		import_svg.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_svg);

		Ref<ResourceImporterTextureAtlas> import_texture_atlas;
		import_texture_atlas.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_texture_atlas);

		Ref<ResourceImporterDynamicFont> import_font_data_dynamic;
		import_font_data_dynamic.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_font_data_dynamic);

		Ref<ResourceImporterBMFont> import_font_data_bmfont;
		import_font_data_bmfont.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_font_data_bmfont);

		Ref<ResourceImporterImageFont> import_font_data_image;
		import_font_data_image.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_font_data_image);

		Ref<ResourceImporterCSVTranslation> import_csv_translation;
		import_csv_translation.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_csv_translation);

		Ref<ResourceImporterWAV> import_wav;
		import_wav.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_wav);

		Ref<ResourceImporterOBJ> import_obj;
		import_obj.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_obj);

		Ref<ResourceImporterShaderFile> import_shader_file;
		import_shader_file.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_shader_file);

		Ref<ResourceImporterScene> import_model_as_scene;
		import_model_as_scene.instantiate("PackedScene");
		ResourceFormatImporter::get_singleton()->add_importer(import_model_as_scene);

		Ref<ResourceImporterScene> import_model_as_animation;
		import_model_as_animation.instantiate("AnimationLibrary");
		ResourceFormatImporter::get_singleton()->add_importer(import_model_as_animation);

		{
			Ref<EditorSceneFormatImporterCollada> import_collada;
			import_collada.instantiate();
			ResourceImporterScene::add_scene_importer(import_collada);

			Ref<EditorOBJImporter> import_obj2;
			import_obj2.instantiate();
			ResourceImporterScene::add_scene_importer(import_obj2);

			Ref<EditorSceneFormatImporterESCN> import_escn;
			import_escn.instantiate();
			ResourceImporterScene::add_scene_importer(import_escn);
		}

		Ref<ResourceImporterBitMap> import_bitmap;
		import_bitmap.instantiate();
		ResourceFormatImporter::get_singleton()->add_importer(import_bitmap);
	}

	{
		Ref<EditorInspectorDefaultPlugin> eidp;
		eidp.instantiate();
		EditorInspector::add_inspector_plugin(eidp);

		Ref<EditorInspectorRootMotionPlugin> rmp;
		rmp.instantiate();
		EditorInspector::add_inspector_plugin(rmp);

		Ref<EditorInspectorVisualShaderModePlugin> smp;
		smp.instantiate();
		EditorInspector::add_inspector_plugin(smp);

		Ref<EditorInspectorParticleProcessMaterialPlugin> ppm;
		ppm.instantiate();
		EditorInspector::add_inspector_plugin(ppm);
	}

	editor_selection = memnew(EditorSelection);

	EditorFileSystem *efs = memnew(EditorFileSystem);
	add_child(efs);

	EditorContextMenuPluginManager::create();

	// Used for previews.
	FileDialog::set_get_icon_callback(callable_mp_static(_file_dialog_get_icon));
	FileDialog::set_get_thumbnail_callback(callable_mp_static(_file_dialog_get_thumbnail));
	FileDialog::register_func = _file_dialog_register;
	FileDialog::unregister_func = _file_dialog_unregister;

	editor_export = memnew(EditorExport);
	add_child(editor_export);

	// Exporters might need the theme.
	EditorThemeManager::initialize();
	theme = EditorThemeManager::generate_theme();
	DisplayServer::set_early_window_clear_color_override(true, theme->get_color(SNAME("background"), EditorStringName(Editor)));

	EDITOR_DEF("_export_preset_advanced_mode", false); // Could be accessed in EditorExportPreset.

	register_exporters();

	ED_SHORTCUT("canvas_item_editor/pan_view", TTRC("Pan View"), Key::SPACE);

	const Vector<String> textfile_ext = ((String)(EDITOR_GET("docks/filesystem/textfile_extensions"))).split(",", false);
	for (const String &E : textfile_ext) {
		textfile_extensions.insert(E);
	}
	const Vector<String> other_file_ext = ((String)(EDITOR_GET("docks/filesystem/other_file_extensions"))).split(",", false);
	for (const String &E : other_file_ext) {
		other_file_extensions.insert(E);
	}

	force_textfile_extensions.insert("csv"); // CSV translation source, has `Translation` resource type, but not loadable as resource.

	resource_preview = memnew(EditorResourcePreview);
	add_child(resource_preview);
	progress_dialog = memnew(ProgressDialog);
	add_child(progress_dialog);
	progress_dialog->connect(SceneStringName(visibility_changed), callable_mp(this, &EditorNode::_progress_dialog_visibility_changed));

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

	main_hsplit = memnew(DockSplitContainer);
	main_hsplit->set_name("DockHSplitMain");
	main_hsplit->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_vbox->add_child(main_hsplit);

	left_l_vsplit = memnew(DockSplitContainer);
	left_l_vsplit->set_name("DockVSplitLeftL");
	left_l_vsplit->set_vertical(true);
	main_hsplit->add_child(left_l_vsplit);

	TabContainer *dock_slot[DockConstants::DOCK_SLOT_MAX];
	dock_slot[DockConstants::DOCK_SLOT_LEFT_UL] = memnew(TabContainer);
	dock_slot[DockConstants::DOCK_SLOT_LEFT_UL]->set_name("DockSlotLeftUL");
	left_l_vsplit->add_child(dock_slot[DockConstants::DOCK_SLOT_LEFT_UL]);
	dock_slot[DockConstants::DOCK_SLOT_LEFT_BL] = memnew(TabContainer);
	dock_slot[DockConstants::DOCK_SLOT_LEFT_BL]->set_name("DockSlotLeftBL");
	left_l_vsplit->add_child(dock_slot[DockConstants::DOCK_SLOT_LEFT_BL]);

	left_r_vsplit = memnew(DockSplitContainer);
	left_r_vsplit->set_name("DockVSplitLeftR");
	left_r_vsplit->set_vertical(true);
	main_hsplit->add_child(left_r_vsplit);
	dock_slot[DockConstants::DOCK_SLOT_LEFT_UR] = memnew(TabContainer);
	dock_slot[DockConstants::DOCK_SLOT_LEFT_UR]->set_name("DockSlotLeftUR");
	left_r_vsplit->add_child(dock_slot[DockConstants::DOCK_SLOT_LEFT_UR]);
	dock_slot[DockConstants::DOCK_SLOT_LEFT_BR] = memnew(TabContainer);
	dock_slot[DockConstants::DOCK_SLOT_LEFT_BR]->set_name("DockSlotLeftBR");
	left_r_vsplit->add_child(dock_slot[DockConstants::DOCK_SLOT_LEFT_BR]);

	VBoxContainer *center_vb = memnew(VBoxContainer);
	center_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	main_hsplit->add_child(center_vb);

	center_split = memnew(DockSplitContainer);
	center_split->set_name("DockVSplitCenter");
	center_split->set_vertical(true);
	center_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	center_split->set_collapsed(true);
	center_vb->add_child(center_split);
	center_split->connect("drag_ended", callable_mp(this, &EditorNode::_bottom_panel_resized));

	right_l_vsplit = memnew(DockSplitContainer);
	right_l_vsplit->set_name("DockVSplitRightL");
	right_l_vsplit->set_vertical(true);
	main_hsplit->add_child(right_l_vsplit);
	dock_slot[DockConstants::DOCK_SLOT_RIGHT_UL] = memnew(TabContainer);
	dock_slot[DockConstants::DOCK_SLOT_RIGHT_UL]->set_name("DockSlotRightUL");
	right_l_vsplit->add_child(dock_slot[DockConstants::DOCK_SLOT_RIGHT_UL]);
	dock_slot[DockConstants::DOCK_SLOT_RIGHT_BL] = memnew(TabContainer);
	dock_slot[DockConstants::DOCK_SLOT_RIGHT_BL]->set_name("DockSlotRightBL");
	right_l_vsplit->add_child(dock_slot[DockConstants::DOCK_SLOT_RIGHT_BL]);

	right_r_vsplit = memnew(DockSplitContainer);
	right_r_vsplit->set_name("DockVSplitRightR");
	right_r_vsplit->set_vertical(true);
	main_hsplit->add_child(right_r_vsplit);
	dock_slot[DockConstants::DOCK_SLOT_RIGHT_UR] = memnew(TabContainer);
	dock_slot[DockConstants::DOCK_SLOT_RIGHT_UR]->set_name("DockSlotRightUR");
	right_r_vsplit->add_child(dock_slot[DockConstants::DOCK_SLOT_RIGHT_UR]);
	dock_slot[DockConstants::DOCK_SLOT_RIGHT_BR] = memnew(TabContainer);
	dock_slot[DockConstants::DOCK_SLOT_RIGHT_BR]->set_name("DockSlotRightBR");
	right_r_vsplit->add_child(dock_slot[DockConstants::DOCK_SLOT_RIGHT_BR]);

	editor_dock_manager = memnew(EditorDockManager);

	// Save the splits for easier access.
	editor_dock_manager->add_vsplit(left_l_vsplit);
	editor_dock_manager->add_vsplit(left_r_vsplit);
	editor_dock_manager->add_vsplit(right_l_vsplit);
	editor_dock_manager->add_vsplit(right_r_vsplit);

	editor_dock_manager->set_hsplit(main_hsplit);

	for (int i = 0; i < DockConstants::DOCK_SLOT_BOTTOM; i++) {
		editor_dock_manager->register_dock_slot((DockConstants::DockSlot)i, dock_slot[i], DockConstants::DOCK_LAYOUT_VERTICAL);
	}

	editor_layout_save_delay_timer = memnew(Timer);
	add_child(editor_layout_save_delay_timer);
	editor_layout_save_delay_timer->set_wait_time(0.5);
	editor_layout_save_delay_timer->set_one_shot(true);
	editor_layout_save_delay_timer->connect("timeout", callable_mp(this, &EditorNode::_save_editor_layout));

	scan_changes_timer = memnew(Timer);
	scan_changes_timer->set_wait_time(0.5);
	scan_changes_timer->set_autostart(EDITOR_GET("interface/editor/import_resources_when_unfocused"));
	scan_changes_timer->connect("timeout", callable_mp(EditorFileSystem::get_singleton(), &EditorFileSystem::scan_changes));
	add_child(scan_changes_timer);

	top_split = memnew(VSplitContainer);
	center_split->add_child(top_split);
	top_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	top_split->set_collapsed(true);

	VBoxContainer *srt = memnew(VBoxContainer);
	srt->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	srt->add_theme_constant_override("separation", 0);
	top_split->add_child(srt);

	scene_tabs = memnew(EditorSceneTabs);
	srt->add_child(scene_tabs);
	scene_tabs->connect("tab_changed", callable_mp(this, &EditorNode::_set_current_scene));
	scene_tabs->connect("tab_closed", callable_mp(this, &EditorNode::_scene_tab_closed));

	distraction_free = memnew(Button);
	distraction_free->set_theme_type_variation("FlatMenuButton");
	ED_SHORTCUT_AND_COMMAND("editor/distraction_free_mode", TTRC("Distraction Free Mode"), KeyModifierMask::CTRL | KeyModifierMask::SHIFT | Key::F11);
	ED_SHORTCUT_OVERRIDE("editor/distraction_free_mode", "macos", KeyModifierMask::META | KeyModifierMask::SHIFT | Key::D);
	ED_SHORTCUT_AND_COMMAND("editor/toggle_last_opened_bottom_panel", TTRC("Toggle Last Opened Bottom Panel"), KeyModifierMask::CMD_OR_CTRL | Key::J);
	distraction_free->set_shortcut(ED_GET_SHORTCUT("editor/distraction_free_mode"));
	distraction_free->set_tooltip_text(TTRC("Toggle distraction-free mode."));
	distraction_free->set_toggle_mode(true);
	scene_tabs->add_extra_button(distraction_free);
	distraction_free->connect(SceneStringName(pressed), callable_mp(this, &EditorNode::_toggle_distraction_free_mode));

	editor_main_screen = memnew(EditorMainScreen);
	editor_main_screen->set_custom_minimum_size(Size2(0, 80) * EDSCALE);
	editor_main_screen->set_draw_behind_parent(true);
	srt->add_child(editor_main_screen);
	editor_main_screen->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	scene_root = memnew(SubViewport);
	scene_root->set_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);
	scene_root->set_translation_domain(StringName());
	scene_root->set_embedding_subwindows(true);
	scene_root->set_disable_3d(true);
	scene_root->set_disable_input(true);
	scene_root->set_as_audio_listener_2d(true);

	accept = memnew(AcceptDialog);
	accept->set_autowrap(true);
	accept->set_min_size(Vector2i(600, 0));
	accept->set_unparent_when_invisible(true);

	save_accept = memnew(AcceptDialog);
	save_accept->set_unparent_when_invisible(true);
	save_accept->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_menu_option).bind((int)MenuOptions::SCENE_SAVE_AS_SCENE));

	project_export = memnew(ProjectExportDialog);
	gui_base->add_child(project_export);

	dependency_error = memnew(DependencyErrorDialog);
	gui_base->add_child(dependency_error);

	editor_settings_dialog = memnew(EditorSettingsDialog);
	gui_base->add_child(editor_settings_dialog);
	editor_settings_dialog->connect("restart_requested", callable_mp(this, &EditorNode::_restart_editor).bind(false));

	project_settings_editor = memnew(ProjectSettingsEditor(&editor_data));
	gui_base->add_child(project_settings_editor);

	scene_import_settings = memnew(SceneImportSettingsDialog);
	gui_base->add_child(scene_import_settings);

	audio_stream_import_settings = memnew(AudioStreamImportSettingsDialog);
	gui_base->add_child(audio_stream_import_settings);

	fontdata_import_settings = memnew(DynamicFontImportSettingsDialog);
	gui_base->add_child(fontdata_import_settings);

	export_template_manager = memnew(ExportTemplateManager);
	gui_base->add_child(export_template_manager);

	feature_profile_manager = memnew(EditorFeatureProfileManager);
	gui_base->add_child(feature_profile_manager);

	build_profile_manager = memnew(EditorBuildProfileManager);
	gui_base->add_child(build_profile_manager);

	about = memnew(EditorAbout);
	gui_base->add_child(about);
	feature_profile_manager->connect("current_feature_profile_changed", callable_mp(this, &EditorNode::_feature_profile_changed));

#if !defined(ANDROID_ENABLED) && !defined(WEB_ENABLED)
	fbx_importer_manager = memnew(FBXImporterManager);
	gui_base->add_child(fbx_importer_manager);
#endif

	warning = memnew(AcceptDialog);
	warning->set_unparent_when_invisible(true);
	warning->add_button(TTRC("Copy Text"), true, "copy");
	warning->connect("custom_action", callable_mp(this, &EditorNode::_copy_warning));

	// Command palette and editor shortcuts.
	command_palette = EditorCommandPalette::get_singleton();
	command_palette->set_title(TTR("Command Palette"));
	gui_base->add_child(command_palette);

	ED_SHORTCUT("editor/next_tab", TTRC("Next Scene Tab"), KeyModifierMask::CTRL + Key::TAB);
	ED_SHORTCUT("editor/prev_tab", TTRC("Previous Scene Tab"), KeyModifierMask::CTRL + KeyModifierMask::SHIFT + Key::TAB);
	ED_SHORTCUT("editor/filter_files", TTRC("Focus FileSystem Filter"), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::ALT + Key::P);

	ED_SHORTCUT_AND_COMMAND("editor/new_scene", TTRC("New Scene"), KeyModifierMask::CMD_OR_CTRL + Key::N);
	ED_SHORTCUT_AND_COMMAND("editor/new_inherited_scene", TTRC("New Inherited Scene..."), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + Key::N);
	ED_SHORTCUT_AND_COMMAND("editor/open_scene", TTRC("Open Scene..."), KeyModifierMask::CMD_OR_CTRL + Key::O);
	ED_SHORTCUT_AND_COMMAND("editor/reopen_closed_scene", TTRC("Reopen Closed Scene"), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + Key::T);

	ED_SHORTCUT_AND_COMMAND("editor/save_scene", TTRC("Save Scene"), KeyModifierMask::CMD_OR_CTRL + Key::S);
	ED_SHORTCUT_AND_COMMAND("editor/save_scene_as", TTRC("Save Scene As..."), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + Key::S);
	ED_SHORTCUT_AND_COMMAND("editor/save_all_scenes", TTRC("Save All Scenes"), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + KeyModifierMask::ALT + Key::S);

	ED_SHORTCUT_ARRAY_AND_COMMAND("editor/quick_open", TTRC("Quick Open..."), { int32_t(KeyModifierMask::SHIFT + KeyModifierMask::ALT + Key::O), int32_t(KeyModifierMask::CMD_OR_CTRL + Key::P) });
	ED_SHORTCUT_OVERRIDE_ARRAY("editor/quick_open", "macos", { int32_t(KeyModifierMask::META + KeyModifierMask::CTRL + Key::O), int32_t(KeyModifierMask::CMD_OR_CTRL + Key::P) });
	ED_SHORTCUT_AND_COMMAND("editor/quick_open_scene", TTRC("Quick Open Scene..."), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + Key::O);
	ED_SHORTCUT_AND_COMMAND("editor/quick_open_script", TTRC("Quick Open Script..."), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::ALT + Key::O);

	ED_SHORTCUT("editor/export_as_mesh_library", TTRC("MeshLibrary..."));

	ED_SHORTCUT_AND_COMMAND("editor/reload_saved_scene", TTRC("Reload Saved Scene"));
	ED_SHORTCUT_AND_COMMAND("editor/close_scene", TTRC("Close Scene"), KeyModifierMask::CMD_OR_CTRL + KeyModifierMask::SHIFT + Key::W);
	ED_SHORTCUT_AND_COMMAND("editor/close_all_scenes", TTRC("Close All Scenes"));
	ED_SHORTCUT_OVERRIDE("editor/close_scene", "macos", KeyModifierMask::CMD_OR_CTRL + Key::W);

	ED_SHORTCUT_AND_COMMAND("editor/editor_settings", TTRC("Editor Settings..."));
	ED_SHORTCUT_OVERRIDE("editor/editor_settings", "macos", KeyModifierMask::META + Key::COMMA);

	ED_SHORTCUT_AND_COMMAND("editor/file_quit", TTRC("Quit"), KeyModifierMask::CMD_OR_CTRL + Key::Q);

	ED_SHORTCUT_AND_COMMAND("editor/project_settings", TTRC("Project Settings..."), Key::NONE, TTRC("Project Settings"));
	ED_SHORTCUT_AND_COMMAND("editor/find_in_files", TTRC("Find in Files..."), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::F);

	ED_SHORTCUT_AND_COMMAND("editor/export", TTRC("Export..."), Key::NONE, TTRC("Export"));

	ED_SHORTCUT_AND_COMMAND("editor/orphan_resource_explorer", TTRC("Orphan Resource Explorer..."));
	ED_SHORTCUT_AND_COMMAND("editor/engine_compilation_configuration_editor", TTRC("Engine Compilation Configuration Editor..."));
	ED_SHORTCUT_AND_COMMAND("editor/upgrade_project", TTRC("Upgrade Project Files..."));

	ED_SHORTCUT("editor/reload_current_project", TTRC("Reload Current Project"));
	ED_SHORTCUT_AND_COMMAND("editor/quit_to_project_list", TTRC("Quit to Project List"), KeyModifierMask::CTRL + KeyModifierMask::SHIFT + Key::Q);
	ED_SHORTCUT_OVERRIDE("editor/quit_to_project_list", "macos", KeyModifierMask::META + KeyModifierMask::CTRL + KeyModifierMask::ALT + Key::Q);

	ED_SHORTCUT("editor/command_palette", TTRC("Command Palette..."), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::P);

	ED_SHORTCUT_AND_COMMAND("editor/take_screenshot", TTRC("Take Screenshot"), KeyModifierMask::CTRL | Key::F12);
	ED_SHORTCUT_OVERRIDE("editor/take_screenshot", "macos", KeyModifierMask::META | Key::F12);

	ED_SHORTCUT_AND_COMMAND("editor/fullscreen_mode", TTRC("Toggle Fullscreen"), KeyModifierMask::SHIFT | Key::F11);
	ED_SHORTCUT_OVERRIDE("editor/fullscreen_mode", "macos", KeyModifierMask::META | KeyModifierMask::CTRL | Key::F);

	ED_SHORTCUT_AND_COMMAND("editor/editor_help", TTRC("Search Help..."), Key::F1);
	ED_SHORTCUT_OVERRIDE("editor/editor_help", "macos", KeyModifierMask::ALT | Key::SPACE);
	ED_SHORTCUT_AND_COMMAND("editor/online_docs", TTRC("Online Documentation"));
	ED_SHORTCUT_AND_COMMAND("editor/forum", TTRC("Forum"));
	ED_SHORTCUT_AND_COMMAND("editor/community", TTRC("Community"));

	ED_SHORTCUT_AND_COMMAND("editor/copy_system_info", TTRC("Copy System Info"));
	ED_SHORTCUT_AND_COMMAND("editor/report_a_bug", TTRC("Report a Bug"));
	ED_SHORTCUT_AND_COMMAND("editor/suggest_a_feature", TTRC("Suggest a Feature"));
	ED_SHORTCUT_AND_COMMAND("editor/send_docs_feedback", TTRC("Send Docs Feedback"));
	ED_SHORTCUT_AND_COMMAND("editor/about", TTRC("About Godot..."));
	ED_SHORTCUT_AND_COMMAND("editor/support_development", TTRC("Support Godot Development"));

	// Use the Ctrl modifier so F2 can be used to rename nodes in the scene tree dock.
	ED_SHORTCUT_AND_COMMAND("editor/editor_2d", TTRC("Open 2D Workspace"), KeyModifierMask::CTRL | Key::F1);
	ED_SHORTCUT_AND_COMMAND("editor/editor_3d", TTRC("Open 3D Workspace"), KeyModifierMask::CTRL | Key::F2);
	ED_SHORTCUT_AND_COMMAND("editor/editor_script", TTRC("Open Script Editor"), KeyModifierMask::CTRL | Key::F3);
	ED_SHORTCUT_AND_COMMAND("editor/editor_game", TTRC("Open Game View"), KeyModifierMask::CTRL | Key::F4);
	ED_SHORTCUT_AND_COMMAND("editor/editor_assetlib", TTRC("Open Asset Library"), KeyModifierMask::CTRL | Key::F5);

	ED_SHORTCUT_OVERRIDE("editor/editor_2d", "macos", KeyModifierMask::META | KeyModifierMask::CTRL | Key::KEY_1);
	ED_SHORTCUT_OVERRIDE("editor/editor_3d", "macos", KeyModifierMask::META | KeyModifierMask::CTRL | Key::KEY_2);
	ED_SHORTCUT_OVERRIDE("editor/editor_script", "macos", KeyModifierMask::META | KeyModifierMask::CTRL | Key::KEY_3);
	ED_SHORTCUT_OVERRIDE("editor/editor_game", "macos", KeyModifierMask::META | KeyModifierMask::CTRL | Key::KEY_4);
	ED_SHORTCUT_OVERRIDE("editor/editor_assetlib", "macos", KeyModifierMask::META | KeyModifierMask::CTRL | Key::KEY_5);

	ED_SHORTCUT_AND_COMMAND("editor/editor_next", TTRC("Open the next Editor"));
	ED_SHORTCUT_AND_COMMAND("editor/editor_prev", TTRC("Open the previous Editor"));

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

	// Spacer to center 2D / 3D / Script buttons.
	left_spacer = memnew(HBoxContainer);
	left_spacer->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	left_spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	title_bar->add_child(left_spacer);

	project_title = memnew(Label);
	project_title->add_theme_font_override(SceneStringName(font), theme->get_font(SNAME("bold"), EditorStringName(EditorFonts)));
	project_title->add_theme_font_size_override(SceneStringName(font_size), theme->get_font_size(SNAME("bold_size"), EditorStringName(EditorFonts)));
	project_title->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	project_title->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	project_title->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	project_title->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	project_title->set_visible(can_expand && menu_type == MENU_TYPE_GLOBAL);
	left_spacer->add_child(project_title);

	HBoxContainer *main_editor_button_hb = memnew(HBoxContainer);
	main_editor_button_hb->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	main_editor_button_hb->set_name("EditorMainScreenButtons");
	editor_main_screen->set_button_container(main_editor_button_hb);
	title_bar->add_child(main_editor_button_hb);
	title_bar->set_center_control(main_editor_button_hb);

	// Spacer to center 2D / 3D / Script buttons.
	right_spacer = memnew(Control);
	right_spacer->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	right_spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	title_bar->add_child(right_spacer);

	project_run_bar = memnew(EditorRunBar);
	project_run_bar->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	title_bar->add_child(project_run_bar);
	project_run_bar->connect("play_pressed", callable_mp(this, &EditorNode::_project_run_started));
	project_run_bar->connect("stop_pressed", callable_mp(this, &EditorNode::_project_run_stopped));

	right_menu_hb = memnew(HBoxContainer);
	right_menu_hb->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	title_bar->add_child(right_menu_hb);

	renderer = memnew(OptionButton);
	renderer->set_visible(true);
	renderer->set_flat(true);
	renderer->set_theme_type_variation("TopBarOptionButton");
	renderer->set_fit_to_longest_item(false);
	renderer->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	renderer->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	renderer->set_tooltip_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);
	renderer->set_tooltip_text(TTRC("Choose a renderer.\n\nNotes:\n- On mobile platforms, the Mobile renderer is used if Forward+ is selected here.\n- On the web platform, the Compatibility renderer is always used."));
	renderer->set_accessibility_name(TTRC("Renderer"));

	right_menu_hb->add_child(renderer);

	if (can_expand) {
		// Add spacer to avoid other controls under the window minimize/maximize/close buttons (right side).
		right_menu_spacer = memnew(Control);
		right_menu_spacer->set_mouse_filter(Control::MOUSE_FILTER_PASS);
		title_bar->add_child(right_menu_spacer);
	}

	const String current_renderer_ps = String(GLOBAL_GET("rendering/renderer/rendering_method")).to_lower();
	const String current_renderer_os = OS::get_singleton()->get_current_rendering_method().to_lower();

	// Add the renderers name to the UI.
	if (current_renderer_ps == current_renderer_os) {
		renderer->connect(SceneStringName(item_selected), callable_mp(this, &EditorNode::_renderer_selected));
		// As we are doing string comparisons, keep in standard case to prevent problems with capitals
		// "vulkan" in particular uses lowercase "v" in the code, and uppercase in the UI.
		PackedStringArray renderers = ProjectSettings::get_singleton()->get_custom_property_info().get(StringName("rendering/renderer/rendering_method")).hint_string.split(",", false);
		for (int i = 0; i < renderers.size(); i++) {
			const String rendering_method = renderers[i].to_lower();
			if (rendering_method == "dummy") {
				continue;
			}
			renderer->add_item(String()); // Set in NOTIFICATION_TRANSLATION_CHANGED.
			renderer->set_item_metadata(-1, rendering_method);
			if (current_renderer_ps == rendering_method) {
				renderer->select(i);
			}
		}
	} else {
		// It's an CLI-overridden rendering method.
		renderer->add_item(String()); // Set in NOTIFICATION_TRANSLATION_CHANGED.
		renderer->set_item_metadata(-1, current_renderer_os);
	}
	_update_renderer_color();

	progress_hb = memnew(BackgroundProgress);

	layout_dialog = memnew(EditorLayoutsDialog);
	gui_base->add_child(layout_dialog);
	layout_dialog->set_hide_on_ok(false);
	layout_dialog->set_size(Size2(225, 270) * EDSCALE);
	layout_dialog->connect("name_confirmed", callable_mp(this, &EditorNode::_dialog_action));

	update_spinner = memnew(MenuButton);
	right_menu_hb->add_child(update_spinner);
	update_spinner->set_button_icon(theme->get_icon(SNAME("Progress1"), EditorStringName(EditorIcons)));
	update_spinner->get_popup()->connect(SceneStringName(id_pressed), callable_mp(this, &EditorNode::_menu_option));
	update_spinner->set_accessibility_name(TTRC("Update Mode"));
	PopupMenu *p = update_spinner->get_popup();
	p->add_radio_check_item(TTRC("Update Continuously"), SPINNER_UPDATE_CONTINUOUSLY);
	p->add_radio_check_item(TTRC("Update When Changed"), SPINNER_UPDATE_WHEN_CHANGED);
	p->add_separator();
	p->add_item(TTRC("Hide Update Spinner"), SPINNER_UPDATE_SPINNER_HIDE);
	_update_update_spinner();

	// Instantiate and place editor docks.

	memnew(SceneTreeDock(scene_root, editor_selection, editor_data));
	editor_dock_manager->add_dock(SceneTreeDock::get_singleton());

	memnew(ImportDock);
	editor_dock_manager->add_dock(ImportDock::get_singleton());

	FileSystemDock *filesystem_dock = memnew(FileSystemDock);
	filesystem_dock->connect("inherit", callable_mp(this, &EditorNode::_inherit_request));
	filesystem_dock->connect("instantiate", callable_mp(this, &EditorNode::_instantiate_request));
	filesystem_dock->connect("display_mode_changed", callable_mp(this, &EditorNode::_save_editor_layout));
	get_project_settings()->connect_filesystem_dock_signals(filesystem_dock);
	editor_dock_manager->add_dock(filesystem_dock);

	memnew(InspectorDock(editor_data));
	editor_dock_manager->add_dock(InspectorDock::get_singleton());

	memnew(SignalsDock);
	editor_dock_manager->add_dock(SignalsDock::get_singleton());

	memnew(GroupsDock);
	editor_dock_manager->add_dock(GroupsDock::get_singleton());

	history_dock = memnew(HistoryDock);
	editor_dock_manager->add_dock(history_dock);

	// Add some offsets to make LEFT_R and RIGHT_L docks wider than minsize.
	const int dock_hsize = 280;
	// By default there is only 3 visible, so set 2 split offsets for them.
	const int dock_hsize_scaled = dock_hsize * EDSCALE;
	main_hsplit->set_split_offsets({ dock_hsize_scaled, -dock_hsize_scaled });

	// Define corresponding default layout.

	const String docks_section = "docks";
	default_layout.instantiate();
	// Dock numbers are based on DockSlot enum value + 1.
	default_layout->set_value(docks_section, "dock_3", "Scene,Import");
	default_layout->set_value(docks_section, "dock_4", "FileSystem,History");
	default_layout->set_value(docks_section, "dock_5", "Inspector,Signals,Groups");

	int hsplits[] = { 0, dock_hsize, -dock_hsize, 0 };
	for (int i = 0; i < (int)std_size(hsplits); i++) {
		default_layout->set_value(docks_section, "dock_hsplit_" + itos(i + 1), hsplits[i]);
	}
	for (int i = 0; i < editor_dock_manager->get_vsplit_count(); i++) {
		default_layout->set_value(docks_section, "dock_split_" + itos(i + 1), 0);
	}

	{
		Dictionary offsets;
		offsets["Audio"] = -450;
		default_layout->set_value(EDITOR_NODE_CONFIG_SECTION, "bottom_panel_offsets", offsets);
	}

	_update_layouts_menu();

	// Bottom panels.

	bottom_panel = memnew(EditorBottomPanel);
	editor_dock_manager->register_dock_slot(DockConstants::DOCK_SLOT_BOTTOM, bottom_panel, DockConstants::DOCK_LAYOUT_HORIZONTAL);
	bottom_panel->set_theme_type_variation("BottomPanel");
	center_split->add_child(bottom_panel);
	center_split->set_dragger_visibility(SplitContainer::DRAGGER_HIDDEN);

	log = memnew(EditorLog);
	editor_dock_manager->add_dock(log);

	center_split->connect(SceneStringName(resized), callable_mp(this, &EditorNode::_vp_resized));

	native_shader_source_visualizer = memnew(EditorNativeShaderSourceVisualizer);
	gui_base->add_child(native_shader_source_visualizer);

	orphan_resources = memnew(OrphanResourcesDialog);
	gui_base->add_child(orphan_resources);

	confirmation = memnew(ConfirmationDialog);
	confirmation_button = confirmation->add_button(TTRC("Don't Save"), DisplayServer::get_singleton()->get_swap_cancel_ok(), "discard");
	gui_base->add_child(confirmation);
	confirmation->set_min_size(Vector2(450.0 * EDSCALE, 0));
	confirmation->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_menu_confirm_current));
	confirmation->connect("custom_action", callable_mp(this, &EditorNode::_discard_changes));
	confirmation->connect("canceled", callable_mp(this, &EditorNode::_cancel_confirmation));

	save_confirmation = memnew(ConfirmationDialog);
	save_confirmation->add_button(TTRC("Don't Save"), DisplayServer::get_singleton()->get_swap_cancel_ok(), "discard");
	gui_base->add_child(save_confirmation);
	save_confirmation->set_min_size(Vector2(450.0 * EDSCALE, 0));
	save_confirmation->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_menu_confirm_current));
	save_confirmation->connect("custom_action", callable_mp(this, &EditorNode::_discard_changes));
	save_confirmation->connect("canceled", callable_mp(this, &EditorNode::_cancel_close_scene_tab));
	save_confirmation->connect("about_to_popup", callable_mp(this, &EditorNode::_prepare_save_confirmation_popup));

	gradle_build_manage_templates = memnew(ConfirmationDialog);
	gradle_build_manage_templates->set_text(TTR("Android build template is missing, please install relevant templates."));
	gradle_build_manage_templates->set_ok_button_text(TTR("Manage Templates"));
	gradle_build_manage_templates->add_button(TTR("Install from file"))->connect(SceneStringName(pressed), callable_mp(this, &EditorNode::_android_install_build_template));
	gradle_build_manage_templates->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_menu_option).bind(EDITOR_MANAGE_EXPORT_TEMPLATES));
	gui_base->add_child(gradle_build_manage_templates);

	file_android_build_source = memnew(EditorFileDialog);
	file_android_build_source->set_title(TTR("Select Android sources file"));
	file_android_build_source->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
	file_android_build_source->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
	file_android_build_source->add_filter("*.zip");
	file_android_build_source->connect("file_selected", callable_mp(this, &EditorNode::_android_build_source_selected));
	gui_base->add_child(file_android_build_source);

	{
		VBoxContainer *vbox = memnew(VBoxContainer);
		install_android_build_template_message = memnew(Label);
		install_android_build_template_message->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
		install_android_build_template_message->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		install_android_build_template_message->set_custom_minimum_size(Size2(300 * EDSCALE, 1));
		vbox->add_child(install_android_build_template_message);

		choose_android_export_profile = memnew(OptionButton);
		choose_android_export_profile->connect(SceneStringName(item_selected), callable_mp(this, &EditorNode::_android_export_preset_selected));
		vbox->add_child(choose_android_export_profile);

		install_android_build_template = memnew(ConfirmationDialog);
		install_android_build_template->set_ok_button_text(TTR("Install"));
		install_android_build_template->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_menu_confirm_current));
		install_android_build_template->add_child(vbox);
		install_android_build_template->set_min_size(Vector2(500.0 * EDSCALE, 0));
		gui_base->add_child(install_android_build_template);
	}

	remove_android_build_template = memnew(ConfirmationDialog);
	remove_android_build_template->set_ok_button_text(TTR("Show in File Manager"));
	remove_android_build_template->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_android_explore_build_templates));
	gui_base->add_child(remove_android_build_template);

	file_templates = memnew(EditorFileDialog);
	file_templates->set_title(TTR("Import Templates From ZIP File"));

	gui_base->add_child(file_templates);
	file_templates->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
	file_templates->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
	file_templates->clear_filters();
	file_templates->add_filter("*.tpz", TTR("Template Package"));

	file = memnew(EditorFileDialog);
	gui_base->add_child(file);
	file->set_current_dir("res://");
	file->set_transient_to_focused(true);

	file_export_lib = memnew(EditorFileDialog);
	file_export_lib->set_title(TTR("Export Library"));
	file_export_lib->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
	file_export_lib->connect("file_selected", callable_mp(this, &EditorNode::_dialog_action));
	file_export_lib->add_option(TTR("Merge With Existing"), Vector<String>(), true);
	file_export_lib->add_option(TTR("Apply MeshInstance Transforms"), Vector<String>(), false);
	gui_base->add_child(file_export_lib);

	file_pack_zip = memnew(EditorFileDialog);
	file_pack_zip->connect("file_selected", callable_mp(this, &EditorNode::_dialog_action));
	file_pack_zip->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
	file_pack_zip->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
	file_pack_zip->add_filter("*.zip", "ZIP Archive");
	file_pack_zip->set_title(TTR("Pack Project as ZIP..."));
	gui_base->add_child(file_pack_zip);

	file->connect("file_selected", callable_mp(this, &EditorNode::_dialog_action));
	file_templates->connect("file_selected", callable_mp(this, &EditorNode::_dialog_action));

	audio_preview_gen = memnew(AudioStreamPreviewGenerator);
	add_child(audio_preview_gen);

	add_editor_plugin(memnew(DebuggerEditorPlugin(debug_menu)));

	disk_changed = memnew(ConfirmationDialog);
	{
		disk_changed->set_title(TTR("Files have been modified outside Godot"));

		VBoxContainer *vbc = memnew(VBoxContainer);
		disk_changed->add_child(vbc);

		Label *dl = memnew(Label);
		dl->set_text(TTR("The following files are newer on disk:"));
		vbc->add_child(dl);

		disk_changed_list = memnew(Tree);
		disk_changed_list->set_accessibility_name(TTRC("The following files are newer on disk:"));
		vbc->add_child(disk_changed_list);
		disk_changed_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);

		Label *what_action_label = memnew(Label);
		what_action_label->set_text(TTR("What action should be taken?"));
		vbc->add_child(what_action_label);

		disk_changed->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_reload_modified_scenes));
		disk_changed->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_reload_project_settings));
		disk_changed->set_ok_button_text(TTR("Reload from disk"));

		disk_changed->add_button(TTR("Ignore external changes"), !DisplayServer::get_singleton()->get_swap_cancel_ok(), "resave");
		disk_changed->connect("custom_action", callable_mp(this, &EditorNode::_resave_externally_modified_scenes));
	}

	gui_base->add_child(disk_changed);

	project_data_missing = memnew(ConfirmationDialog);
	project_data_missing->set_text(TTRC("Project data folder (.godot) is missing. Please restart editor."));
	project_data_missing->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::restart_editor).bind(false));
	project_data_missing->set_ok_button_text(TTRC("Restart"));

	gui_base->add_child(project_data_missing);

	add_editor_plugin(memnew(CanvasItemEditorPlugin));
	add_editor_plugin(memnew(Node3DEditorPlugin));
	add_editor_plugin(memnew(ScriptEditorPlugin));

	if (!Engine::get_singleton()->is_recovery_mode_hint()) {
		add_editor_plugin(get_game_view_plugin());
	}

	EditorAudioBuses *audio_bus_editor = EditorAudioBuses::register_editor();

	ScriptTextEditor::register_editor(); // Register one for text scripts.
	TextEditor::register_editor();

	if (AssetLibraryEditorPlugin::is_available()) {
		add_editor_plugin(memnew(AssetLibraryEditorPlugin));
	} else {
		print_verbose("Asset Library not available (due to using Web editor, or SSL support disabled).");
	}

	// More visually meaningful to have this later.
	add_editor_plugin(memnew(AnimationPlayerEditorPlugin));
	add_editor_plugin(memnew(AnimationTrackKeyEditEditorPlugin));
	add_editor_plugin(memnew(AnimationMarkerKeyEditEditorPlugin));

	add_editor_plugin(VersionControlEditorPlugin::get_singleton());

	add_editor_plugin(memnew(AudioBusesEditorPlugin(audio_bus_editor)));

	for (int i = 0; i < EditorPlugins::get_plugin_count(); i++) {
		add_editor_plugin(EditorPlugins::create(i));
	}

	for (const StringName &extension_class_name : GDExtensionEditorPlugins::get_extension_classes()) {
		add_extension_editor_plugin(extension_class_name);
	}
	GDExtensionEditorPlugins::editor_node_add_plugin = &EditorNode::add_extension_editor_plugin;
	GDExtensionEditorPlugins::editor_node_remove_plugin = &EditorNode::remove_extension_editor_plugin;

	for (int i = 0; i < plugin_init_callback_count; i++) {
		plugin_init_callbacks[i]();
	}

	resource_preview->add_preview_generator(Ref<EditorTexturePreviewPlugin>(memnew(EditorTexturePreviewPlugin)));
	resource_preview->add_preview_generator(Ref<EditorImagePreviewPlugin>(memnew(EditorImagePreviewPlugin)));
	resource_preview->add_preview_generator(Ref<EditorPackedScenePreviewPlugin>(memnew(EditorPackedScenePreviewPlugin)));
	resource_preview->add_preview_generator(Ref<EditorMaterialPreviewPlugin>(memnew(EditorMaterialPreviewPlugin)));
	resource_preview->add_preview_generator(Ref<EditorScriptPreviewPlugin>(memnew(EditorScriptPreviewPlugin)));
	resource_preview->add_preview_generator(Ref<EditorAudioStreamPreviewPlugin>(memnew(EditorAudioStreamPreviewPlugin)));
	resource_preview->add_preview_generator(Ref<EditorMeshPreviewPlugin>(memnew(EditorMeshPreviewPlugin)));
	resource_preview->add_preview_generator(Ref<EditorBitmapPreviewPlugin>(memnew(EditorBitmapPreviewPlugin)));
	resource_preview->add_preview_generator(Ref<EditorFontPreviewPlugin>(memnew(EditorFontPreviewPlugin)));
	resource_preview->add_preview_generator(Ref<EditorGradientPreviewPlugin>(memnew(EditorGradientPreviewPlugin)));

	{
		Ref<StandardMaterial3DConversionPlugin> spatial_mat_convert;
		spatial_mat_convert.instantiate();
		resource_conversion_plugins.push_back(spatial_mat_convert);

		Ref<ORMMaterial3DConversionPlugin> orm_mat_convert;
		orm_mat_convert.instantiate();
		resource_conversion_plugins.push_back(orm_mat_convert);

		Ref<CanvasItemMaterialConversionPlugin> canvas_item_mat_convert;
		canvas_item_mat_convert.instantiate();
		resource_conversion_plugins.push_back(canvas_item_mat_convert);

		Ref<ParticleProcessMaterialConversionPlugin> particles_mat_convert;
		particles_mat_convert.instantiate();
		resource_conversion_plugins.push_back(particles_mat_convert);

		Ref<ProceduralSkyMaterialConversionPlugin> procedural_sky_mat_convert;
		procedural_sky_mat_convert.instantiate();
		resource_conversion_plugins.push_back(procedural_sky_mat_convert);

		Ref<PanoramaSkyMaterialConversionPlugin> panorama_sky_mat_convert;
		panorama_sky_mat_convert.instantiate();
		resource_conversion_plugins.push_back(panorama_sky_mat_convert);

		Ref<PhysicalSkyMaterialConversionPlugin> physical_sky_mat_convert;
		physical_sky_mat_convert.instantiate();
		resource_conversion_plugins.push_back(physical_sky_mat_convert);

		Ref<FogMaterialConversionPlugin> fog_mat_convert;
		fog_mat_convert.instantiate();
		resource_conversion_plugins.push_back(fog_mat_convert);

		Ref<VisualShaderConversionPlugin> vshader_convert;
		vshader_convert.instantiate();
		resource_conversion_plugins.push_back(vshader_convert);
	}

	update_spinner_step_msec = OS::get_singleton()->get_ticks_msec();
	update_spinner_step_frame = Engine::get_singleton()->get_frames_drawn();

	editor_plugins_over = memnew(EditorPluginList);
	editor_plugins_force_over = memnew(EditorPluginList);
	editor_plugins_force_input_forwarding = memnew(EditorPluginList);

	Ref<GDExtensionExportPlugin> gdextension_export_plugin;
	gdextension_export_plugin.instantiate();

	EditorExport::get_singleton()->add_export_plugin(gdextension_export_plugin);

	Ref<DedicatedServerExportPlugin> dedicated_server_export_plugin;
	dedicated_server_export_plugin.instantiate();

	EditorExport::get_singleton()->add_export_plugin(dedicated_server_export_plugin);

	Ref<ShaderBakerExportPlugin> shader_baker_export_plugin;
	shader_baker_export_plugin.instantiate();

#ifdef VULKAN_ENABLED
	Ref<ShaderBakerExportPluginPlatformVulkan> shader_baker_export_plugin_platform_vulkan;
	shader_baker_export_plugin_platform_vulkan.instantiate();
	shader_baker_export_plugin->add_platform(shader_baker_export_plugin_platform_vulkan);
#endif

#ifdef D3D12_ENABLED
	Ref<ShaderBakerExportPluginPlatformD3D12> shader_baker_export_plugin_platform_d3d12;
	shader_baker_export_plugin_platform_d3d12.instantiate();
	shader_baker_export_plugin->add_platform(shader_baker_export_plugin_platform_d3d12);
#endif

#ifdef METAL_ENABLED
	Ref<ShaderBakerExportPluginPlatformMetal> shader_baker_export_plugin_platform_metal;
	shader_baker_export_plugin_platform_metal.instantiate();
	shader_baker_export_plugin->add_platform(shader_baker_export_plugin_platform_metal);
#endif

	EditorExport::get_singleton()->add_export_plugin(shader_baker_export_plugin);

	Ref<PackedSceneEditorTranslationParserPlugin> packed_scene_translation_parser_plugin;
	packed_scene_translation_parser_plugin.instantiate();
	EditorTranslationParser::get_singleton()->add_parser(packed_scene_translation_parser_plugin, EditorTranslationParser::STANDARD);

	_edit_current();
	saving_resource = Ref<Resource>();

	set_process(true);

	open_imported = memnew(ConfirmationDialog);
	open_imported->set_ok_button_text(TTR("Open Anyway"));
	new_inherited_button = open_imported->add_button(TTR("New Inherited"), !DisplayServer::get_singleton()->get_swap_cancel_ok(), "inherit");
	open_imported->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_open_imported));
	open_imported->connect("custom_action", callable_mp(this, &EditorNode::_inherit_imported));
	gui_base->add_child(open_imported);

	quick_open_dialog = memnew(EditorQuickOpenDialog);
	gui_base->add_child(quick_open_dialog);

	quick_open_color_palette = memnew(EditorQuickOpenDialog);
	gui_base->add_child(quick_open_color_palette);

	_update_recent_scenes();

	set_process_shortcut_input(true);

	load_errors = memnew(RichTextLabel);
	load_error_dialog = memnew(AcceptDialog);
	load_error_dialog->set_unparent_when_invisible(true);
	load_error_dialog->add_child(load_errors);
	load_error_dialog->set_title(TTR("Load Errors"));
	load_error_dialog->connect(SceneStringName(visibility_changed), callable_mp(this, &EditorNode::_load_error_dialog_visibility_changed));

	execute_outputs = memnew(RichTextLabel);
	execute_outputs->set_selection_enabled(true);
	execute_outputs->set_context_menu_enabled(true);
	execute_output_dialog = memnew(AcceptDialog);
	execute_output_dialog->set_unparent_when_invisible(true);
	execute_output_dialog->add_child(execute_outputs);
	execute_output_dialog->set_title("");

	EditorFileSystem::get_singleton()->connect("sources_changed", callable_mp(this, &EditorNode::_sources_changed));
	EditorFileSystem::get_singleton()->connect("filesystem_changed", callable_mp(this, &EditorNode::_fs_changed));
	EditorFileSystem::get_singleton()->connect("resources_reimporting", callable_mp(this, &EditorNode::_resources_reimporting));
	EditorFileSystem::get_singleton()->connect("resources_reimported", callable_mp(this, &EditorNode::_resources_reimported));
	EditorFileSystem::get_singleton()->connect("resources_reload", callable_mp(this, &EditorNode::_resources_changed));

	_build_icon_type_cache();

	pick_main_scene = memnew(ConfirmationDialog);
	gui_base->add_child(pick_main_scene);
	pick_main_scene->set_ok_button_text(TTR("Select"));
	pick_main_scene->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_menu_option).bind(SETTINGS_PICK_MAIN_SCENE));
	select_current_scene_button = pick_main_scene->add_button(TTR("Select Current"), true, "select_current");
	pick_main_scene->connect("custom_action", callable_mp(this, &EditorNode::_pick_main_scene_custom_action));

	open_project_settings = memnew(ConfirmationDialog);
	gui_base->add_child(open_project_settings);
	open_project_settings->set_ok_button_text(TTRC("Open Project Settings"));
	open_project_settings->connect(SceneStringName(confirmed), callable_mp(this, &EditorNode::_menu_option).bind(PROJECT_OPEN_SETTINGS));

	for (int i = 0; i < _init_callbacks.size(); i++) {
		_init_callbacks[i]();
	}

	editor_data.add_edited_scene(-1);
	editor_data.set_edited_scene(0);
	scene_tabs->update_scene_tabs();

	ImportDock::get_singleton()->initialize_import_options();

	FileAccess::set_file_close_fail_notify_callback(_file_access_close_error_notify);

	print_handler.printfunc = _print_handler;
	print_handler.userdata = this;
	add_print_handler(&print_handler);

	ResourceSaver::set_save_callback(_resource_saved);
	ResourceLoader::set_load_callback(_resource_loaded);

	// Apply setting presets in case the editor_settings file is missing values.
	EditorSettingsDialog::update_navigation_preset();

	screenshot_timer = memnew(Timer);
	screenshot_timer->set_one_shot(true);
	screenshot_timer->set_wait_time(settings_menu->get_submenu_popup_delay() + 0.1f);
	screenshot_timer->connect("timeout", callable_mp(this, &EditorNode::_request_screenshot));
	add_child(screenshot_timer);
	screenshot_timer->set_owner(get_owner());

	// Extend menu bar to window title.
	if (can_expand) {
		DisplayServer::get_singleton()->process_events();
		DisplayServer::get_singleton()->window_set_flag(DisplayServer::WINDOW_FLAG_EXTEND_TO_TITLE, true, DisplayServer::MAIN_WINDOW_ID);
		title_bar->set_can_move_window(true);
	}

	{
		const String exec = OS::get_singleton()->get_executable_path();
		const String old_exec = EditorSettings::get_singleton()->get_project_metadata("editor_metadata", "executable_path", "");
		// Save editor executable path for third-party tools.
		if (exec != old_exec) {
			EditorSettings::get_singleton()->set_project_metadata("editor_metadata", "executable_path", exec);
		}
	}

	follow_system_theme = EDITOR_GET("interface/theme/follow_system_theme");
	use_system_accent_color = EDITOR_GET("interface/theme/use_system_accent_color");
}

