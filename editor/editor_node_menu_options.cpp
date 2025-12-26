/**************************************************************************/
/*  editor_node_menu_options.cpp                                          */
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
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/io/resource_uid.h"
#include "core/object/object.h"
#include "core/object/script_language.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "core/string/translation_server.h"
#include "core/version.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/editor_data.h"
#include "editor/file_system/dependency_editor.h"
#include "editor/editor_log.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/export/editor_export.h"
#include "editor/export/project_export.h"
#include "editor/export/project_zip_packer.h"
#include "editor/export/export_template_manager.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/editor_about.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_quick_open_dialog.h"
#include "editor/gui/editor_toaster.h"
#include "editor/import/fbx_importer_manager.h"
#include "editor/project_upgrade/project_upgrade_tool.h"
#include "editor/run/editor_run_bar.h"
#include "editor/scene/3d/mesh_library_editor_plugin.h"
#include "editor/scene/editor_scene_tabs.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_build_profile.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_feature_profile.h"
#include "editor/settings/editor_layouts_dialog.h"
#include "editor/settings/editor_settings.h"
#include "editor/settings/editor_settings_dialog.h"
#include "editor/settings/project_settings_editor.h"
#include "core/input/input.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/3d/mesh_library.h"
#include "scene/resources/packed_scene.h"
#include "servers/display/display_server.h"

static const String REMOVE_ANDROID_BUILD_TEMPLATE_MESSAGE = TTRC("The Android build template is already installed in this project and it won't be overwritten.\nRemove the \"%s\" directory manually before attempting this operation again.");

static String _get_unsaved_scene_dialog_text(String p_scene_filename, uint64_t p_started_timestamp) {
	String unsaved_message;

	// Consider editor startup to be a point of saving, so that when you
	// close and reopen the editor, you don't get an excessively long
	// "modified X hours ago".
	const uint64_t last_modified_seconds = Time::get_singleton()->get_unix_time_from_system() - MAX(p_started_timestamp, FileAccess::get_modified_time(p_scene_filename));
	String last_modified_string;
	if (last_modified_seconds < 120) {
		last_modified_string = vformat(TTRN("%d second ago", "%d seconds ago", last_modified_seconds), last_modified_seconds);
	} else if (last_modified_seconds < 7200) {
		last_modified_string = vformat(TTRN("%d minute ago", "%d minutes ago", last_modified_seconds / 60), last_modified_seconds / 60);
	} else {
		last_modified_string = vformat(TTRN("%d hour ago", "%d hours ago", last_modified_seconds / 3600), last_modified_seconds / 3600);
	}
	unsaved_message = vformat(TTR("Scene \"%s\" has unsaved changes.\nLast saved: %s."), p_scene_filename, last_modified_string);

	return unsaved_message;
}

void EditorNode::_menu_option(int p_option) {
	_menu_option_confirm(p_option, false);
}

void EditorNode::_menu_confirm_current() {
	_menu_option_confirm(current_menu_option, true);
}

void EditorNode::trigger_menu_option(int p_option, bool p_confirmed) {
	_menu_option_confirm(p_option, p_confirmed);
}

String EditorNode::adjust_scene_name_casing(const String &p_root_name) {
	switch (GLOBAL_GET("editor/naming/scene_name_casing").operator int()) {
		case SCENE_NAME_CASING_AUTO:
			// Use casing of the root node.
			break;
		case SCENE_NAME_CASING_PASCAL_CASE:
			return p_root_name.to_pascal_case();
		case SCENE_NAME_CASING_SNAKE_CASE:
			return p_root_name.to_snake_case();
		case SCENE_NAME_CASING_KEBAB_CASE:
			return p_root_name.to_kebab_case();
		case SCENE_NAME_CASING_CAMEL_CASE:
			return p_root_name.to_camel_case();
	}
	return p_root_name;
}

String EditorNode::adjust_script_name_casing(const String &p_file_name, ScriptLanguage::ScriptNameCasing p_auto_casing) {
	int editor_casing = GLOBAL_GET("editor/naming/script_name_casing");
	if (editor_casing == ScriptLanguage::SCRIPT_NAME_CASING_AUTO) {
		// Use the script language's preferred casing.
		editor_casing = p_auto_casing;
	}

	switch (editor_casing) {
		case ScriptLanguage::SCRIPT_NAME_CASING_AUTO:
			// Script language has no preference, so do not adjust.
			break;
		case ScriptLanguage::SCRIPT_NAME_CASING_PASCAL_CASE:
			return p_file_name.to_pascal_case();
		case ScriptLanguage::SCRIPT_NAME_CASING_SNAKE_CASE:
			return p_file_name.to_snake_case();
		case ScriptLanguage::SCRIPT_NAME_CASING_KEBAB_CASE:
			return p_file_name.to_kebab_case();
		case ScriptLanguage::SCRIPT_NAME_CASING_CAMEL_CASE:
			return p_file_name.to_camel_case();
	}
	return p_file_name;
}

void EditorNode::_menu_option_confirm(int p_option, bool p_confirmed) {
	if (!p_confirmed) { // FIXME: this may be a hack.
		current_menu_option = (MenuOptions)p_option;
	}

	switch (p_option) {
		case SCENE_NEW_SCENE: {
			new_scene();

		} break;
		case SCENE_NEW_INHERITED_SCENE:
		case SCENE_OPEN_SCENE: {
			file->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
			List<String> extensions;
			ResourceLoader::get_recognized_extensions_for_type("PackedScene", &extensions);
			file->clear_filters();
			for (const String &extension : extensions) {
				file->add_filter("*." + extension, extension.to_upper());
			}

			Node *scene = editor_data.get_edited_scene_root();
			if (scene) {
				file->set_current_path(scene->get_scene_file_path());
			};
			file->set_title(p_option == SCENE_OPEN_SCENE ? TTR("Open Scene") : TTR("Open Base Scene"));
			file->popup_file_dialog();

		} break;
		case SCENE_QUICK_OPEN: {
			quick_open_dialog->popup_dialog({ "Resource" }, callable_mp(this, &EditorNode::_quick_opened));
		} break;
		case SCENE_QUICK_OPEN_SCENE: {
			quick_open_dialog->popup_dialog({ "PackedScene" }, callable_mp(this, &EditorNode::_quick_opened));
		} break;
		case SCENE_QUICK_OPEN_SCRIPT: {
			quick_open_dialog->popup_dialog({ "Script" }, callable_mp(this, &EditorNode::_quick_opened));
		} break;
		case SCENE_OPEN_PREV: {
			if (!prev_closed_scenes.is_empty()) {
				load_scene(prev_closed_scenes.back()->get());
			}
		} break;
		case EditorSceneTabs::SCENE_CLOSE_OTHERS: {
			tab_closing_menu_option = -1;
			for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
				if (i == editor_data.get_edited_scene()) {
					continue;
				}
				tabs_to_close.push_back(editor_data.get_scene_path(i));
			}
			_proceed_closing_scene_tabs();
		} break;
		case EditorSceneTabs::SCENE_CLOSE_RIGHT: {
			tab_closing_menu_option = -1;
			for (int i = editor_data.get_edited_scene() + 1; i < editor_data.get_edited_scene_count(); i++) {
				tabs_to_close.push_back(editor_data.get_scene_path(i));
			}
			_proceed_closing_scene_tabs();
		} break;
		case SCENE_CLOSE_ALL: {
			tab_closing_menu_option = -1;
			for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
				tabs_to_close.push_back(editor_data.get_scene_path(i));
			}
			_proceed_closing_scene_tabs();
		} break;
		case SCENE_CLOSE: {
			_scene_tab_closed(editor_data.get_edited_scene());
		} break;
		case SCENE_TAB_CLOSE:
		case SCENE_SAVE_SCENE: {
			int scene_idx = (p_option == SCENE_SAVE_SCENE) ? -1 : tab_closing_idx;
			Node *scene = editor_data.get_edited_scene_root(scene_idx);
			if (scene && !scene->get_scene_file_path().is_empty()) {
				if (DirAccess::exists(scene->get_scene_file_path().get_base_dir())) {
					if (scene_idx != editor_data.get_edited_scene()) {
						_save_scene_with_preview(scene->get_scene_file_path(), scene_idx);
					} else {
						_save_scene_with_preview(scene->get_scene_file_path());
					}

					if (scene_idx != -1) {
						_discard_changes();
					}
					save_editor_layout_delayed();
				} else {
					show_save_accept(vformat(TTR("%s no longer exists! Please specify a new save location."), scene->get_scene_file_path().get_base_dir()), TTR("OK"));
				}
				break;
			}
			[[fallthrough]];
		}
		case SCENE_MULTI_SAVE_AS_SCENE:
		case SCENE_SAVE_AS_SCENE: {
			int scene_idx = (p_option == SCENE_SAVE_SCENE || p_option == SCENE_SAVE_AS_SCENE || p_option == SCENE_MULTI_SAVE_AS_SCENE) ? -1 : tab_closing_idx;

			Node *scene = editor_data.get_edited_scene_root(scene_idx);

			if (!scene) {
				if (p_option == SCENE_SAVE_SCENE) {
					// Pressing Ctrl + S saves the current script if a scene is currently open, but it won't if the scene has no root node.
					// Work around this by explicitly saving the script in this case (similar to pressing Ctrl + Alt + S).
					ScriptEditor::get_singleton()->save_current_script();
				}

				const int saved = _save_external_resources(true);
				if (saved > 0) {
					EditorToaster::get_singleton()->popup_str(vformat(TTR("The current scene has no root node, but %d modified external resource(s) and/or plugin data were saved anyway."), saved), EditorToaster::SEVERITY_INFO);
				} else if (p_option == SCENE_SAVE_AS_SCENE) {
					// Don't show this dialog when pressing Ctrl + S to avoid interfering with script saving.
					show_accept(
							TTR("A root node is required to save the scene. You can add a root node using the Scene tree dock."),
							TTR("OK"));
				}

				break;
			}

			file->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);

			List<String> extensions;
			Ref<PackedScene> sd = memnew(PackedScene);
			ResourceSaver::get_recognized_extensions(sd, &extensions);
			file->clear_filters();
			for (const String &extension : extensions) {
				file->add_filter("*." + extension, extension.to_upper());
			}

			if (!scene->get_scene_file_path().is_empty()) {
				String path = scene->get_scene_file_path();
				String root_name = EditorNode::adjust_scene_name_casing(scene->get_name());
				String ext = path.get_extension().to_lower();
				path = path.get_base_dir().path_join(root_name + "." + ext);

				file->set_current_path(path);
				if (extensions.size()) {
					if (extensions.find(ext) == nullptr) {
						file->set_current_path(path.replacen("." + ext, "." + extensions.front()->get()));
					}
				}
			} else if (extensions.size()) {
				String root_name = scene->get_name();
				root_name = EditorNode::adjust_scene_name_casing(root_name);
				file->set_current_path(root_name + "." + extensions.front()->get().to_lower());
			}
			file->set_title(TTR("Save Scene As..."));
			file->popup_file_dialog();

		} break;

		case SCENE_TAB_SET_AS_MAIN_SCENE: {
			const String scene_path = editor_data.get_scene_path(editor_data.get_edited_scene());
			if (scene_path.is_empty()) {
				current_menu_option = SAVE_AND_SET_MAIN_SCENE;
				_menu_option_confirm(SCENE_SAVE_AS_SCENE, true);
				file->set_title(TTR("Save new main scene..."));
			} else {
				ProjectSettings::get_singleton()->set("application/run/main_scene", ResourceUID::path_to_uid(scene_path));
				ProjectSettings::get_singleton()->save();
				FileSystemDock::get_singleton()->update_all();
			}
		} break;

		case SCENE_SAVE_ALL_SCENES: {
			_save_all_scenes();
		} break;

		case EditorSceneTabs::SCENE_RUN: {
			project_run_bar->play_current_scene();
		} break;

		case PROJECT_EXPORT: {
			project_export->popup_export();
		} break;

		case PROJECT_PACK_AS_ZIP: {
			String resource_path = ProjectSettings::get_singleton()->get_resource_path();
			const String base_path = resource_path.substr(0, resource_path.rfind_char('/')) + "/";

			file_pack_zip->set_current_path(base_path);
			file_pack_zip->set_current_file(ProjectZIPPacker::get_project_zip_safe_name());
			file_pack_zip->popup_file_dialog();
		} break;

		case SCENE_UNDO: {
			if ((int)Input::get_singleton()->get_mouse_button_mask() & 0x7) {
				log->add_message(TTR("Can't undo while mouse buttons are pressed."), EditorLog::MSG_TYPE_EDITOR);
			} else {
				EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
				String action = undo_redo->get_current_action_name();
				int id = undo_redo->get_current_action_history_id();
				if (!undo_redo->undo()) {
					log->add_message(TTR("Nothing to undo."), EditorLog::MSG_TYPE_EDITOR);
				} else if (!action.is_empty()) {
					switch (id) {
						case EditorUndoRedoManager::GLOBAL_HISTORY:
							log->add_message(vformat(TTR("Global Undo: %s"), action), EditorLog::MSG_TYPE_EDITOR);
							break;
						case EditorUndoRedoManager::REMOTE_HISTORY:
							log->add_message(vformat(TTR("Remote Undo: %s"), action), EditorLog::MSG_TYPE_EDITOR);
							break;
						default:
							log->add_message(vformat(TTR("Scene Undo: %s"), action), EditorLog::MSG_TYPE_EDITOR);
					}
				}
			}
			_update_unsaved_cache();
		} break;
		case SCENE_REDO: {
			EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
			if ((int)Input::get_singleton()->get_mouse_button_mask() & 0x7) {
				log->add_message(TTR("Can't redo while mouse buttons are pressed."), EditorLog::MSG_TYPE_EDITOR);
			} else {
				if (!undo_redo->redo()) {
					log->add_message(TTR("Nothing to redo."), EditorLog::MSG_TYPE_EDITOR);
				} else {
					String action = undo_redo->get_current_action_name();
					if (action.is_empty()) {
						break;
					}

					switch (undo_redo->get_current_action_history_id()) {
						case EditorUndoRedoManager::GLOBAL_HISTORY:
							log->add_message(vformat(TTR("Global Redo: %s"), action), EditorLog::MSG_TYPE_EDITOR);
							break;
						case EditorUndoRedoManager::REMOTE_HISTORY:
							log->add_message(vformat(TTR("Remote Redo: %s"), action), EditorLog::MSG_TYPE_EDITOR);
							break;
						default:
							log->add_message(vformat(TTR("Scene Redo: %s"), action), EditorLog::MSG_TYPE_EDITOR);
					}
				}
			}
			_update_unsaved_cache();
		} break;

		case SCENE_RELOAD_SAVED_SCENE: {
			Node *scene = get_edited_scene();

			if (!scene) {
				break;
			}

			String scene_filename = scene->get_scene_file_path();
			String unsaved_message;

			if (scene_filename.is_empty()) {
				show_warning(TTR("Can't reload a scene that was never saved."));
				break;
			}

			if (unsaved_cache) {
				if (!p_confirmed) {
					confirmation->set_ok_button_text(TTRC("Save & Reload"));
					unsaved_message = _get_unsaved_scene_dialog_text(scene_filename, started_timestamp);
					confirmation->set_text(unsaved_message + "\n\n" + TTR("Save before reloading the scene?"));
					confirmation->popup_centered();
					confirmation_button->show();
					confirmation_button->grab_focus();
					break;
				} else {
					_save_scene_with_preview(scene_filename);
				}
			}

			_discard_changes();
		} break;

		case EditorSceneTabs::SCENE_SHOW_IN_FILESYSTEM: {
			String path = editor_data.get_scene_path(editor_data.get_edited_scene());
			if (!path.is_empty()) {
				FileSystemDock::get_singleton()->navigate_to_path(path);
			}
		} break;

		case PROJECT_OPEN_SETTINGS: {
			project_settings_editor->popup_project_settings();
		} break;

		case PROJECT_FIND_IN_FILES: {
			ScriptEditor::get_singleton()->open_find_in_files_dialog("");
		} break;

		case PROJECT_INSTALL_ANDROID_SOURCE: {
			if (p_confirmed) {
				if (export_template_manager->is_android_template_installed(android_export_preset)) {
					remove_android_build_template->set_text(vformat(TTR(REMOVE_ANDROID_BUILD_TEMPLATE_MESSAGE), export_template_manager->get_android_build_directory(android_export_preset)));
					remove_android_build_template->popup_centered();
				} else if (!export_template_manager->can_install_android_template(android_export_preset)) {
					gradle_build_manage_templates->popup_centered();
				} else {
					export_template_manager->install_android_template(android_export_preset);
				}
			} else {
				bool has_custom_gradle_build = false;
				choose_android_export_profile->clear();
				for (int i = 0; i < EditorExport::get_singleton()->get_export_preset_count(); i++) {
					Ref<EditorExportPreset> export_preset = EditorExport::get_singleton()->get_export_preset(i);
					if (export_preset->get_platform()->get_class_name() == "EditorExportPlatformAndroid" && (bool)export_preset->get("gradle_build/use_gradle_build")) {
						choose_android_export_profile->add_item(export_preset->get_name(), i);
						String gradle_build_directory = export_preset->get("gradle_build/gradle_build_directory");
						String android_source_template = export_preset->get("gradle_build/android_source_template");
						if (!android_source_template.is_empty() || (gradle_build_directory != "" && gradle_build_directory != "res://android")) {
							has_custom_gradle_build = true;
						}
					}
				}
				_android_export_preset_selected(choose_android_export_profile->get_item_count() >= 1 ? 0 : -1);

				if (choose_android_export_profile->get_item_count() > 1 && has_custom_gradle_build) {
					// If there's multiple options and at least one of them uses a custom gradle build then prompt the user to choose.
					choose_android_export_profile->show();
					install_android_build_template->popup_centered();
				} else {
					choose_android_export_profile->hide();

					if (export_template_manager->is_android_template_installed(android_export_preset)) {
						remove_android_build_template->set_text(vformat(TTR(REMOVE_ANDROID_BUILD_TEMPLATE_MESSAGE), export_template_manager->get_android_build_directory(android_export_preset)));
						remove_android_build_template->popup_centered();
					} else if (export_template_manager->can_install_android_template(android_export_preset)) {
						install_android_build_template->popup_centered();
					} else {
						gradle_build_manage_templates->popup_centered();
					}
				}
			}
		} break;
		case PROJECT_OPEN_USER_DATA_FOLDER: {
			// Ensure_user_data_dir() to prevent the edge case: "Open User Data Folder" won't work after the project was renamed in ProjectSettingsEditor unless the project is saved.
			OS::get_singleton()->ensure_user_data_dir();
			OS::get_singleton()->shell_show_in_file_manager(OS::get_singleton()->get_user_data_dir(), true);
		} break;
		case SCENE_QUIT:
		case PROJECT_QUIT_TO_PROJECT_MANAGER:
		case PROJECT_RELOAD_CURRENT_PROJECT: {
			if (p_confirmed && plugin_to_save) {
				plugin_to_save->save_external_data();
				p_confirmed = false;
			}

			if (p_confirmed && stop_project_confirmation && project_run_bar->is_playing()) {
				project_run_bar->stop_playing();
				stop_project_confirmation = false;
				p_confirmed = false;
			}

			if (!p_confirmed) {
				if (!stop_project_confirmation && project_run_bar->is_playing()) {
					if (p_option == PROJECT_RELOAD_CURRENT_PROJECT) {
						confirmation->set_text(TTR("Stop running project before reloading the current project?"));
						confirmation->set_ok_button_text(TTR("Stop & Reload"));
					} else {
						confirmation->set_text(TTR("Stop running project before exiting the editor?"));
						confirmation->set_ok_button_text(TTR("Stop & Quit"));
					}
					confirmation->reset_size();
					confirmation->popup_centered();
					confirmation_button->hide();
					stop_project_confirmation = true;
					break;
				}

				bool save_each = EDITOR_GET("interface/editor/save_each_scene_on_quit");
				if (_next_unsaved_scene(!save_each) == -1) {
					if (EditorUndoRedoManager::get_singleton()->is_history_unsaved(EditorUndoRedoManager::GLOBAL_HISTORY)) {
						if (p_option == PROJECT_RELOAD_CURRENT_PROJECT) {
							save_confirmation->set_ok_button_text(TTR("Save & Reload"));
							save_confirmation->set_text(TTR("Save modified resources before reloading?"));
						} else {
							save_confirmation->set_ok_button_text(TTR("Save & Quit"));
							save_confirmation->set_text(TTR("Save modified resources before closing?"));
						}
						save_confirmation->reset_size();
						save_confirmation->popup_centered();
						break;
					}

					plugin_to_save = nullptr;
					for (int i = 0; i < editor_data.get_editor_plugin_count(); i++) {
						const String unsaved_status = editor_data.get_editor_plugin(i)->get_unsaved_status();
						if (!unsaved_status.is_empty()) {
							if (p_option == PROJECT_RELOAD_CURRENT_PROJECT) {
								save_confirmation->set_ok_button_text(TTR("Save & Reload"));
								save_confirmation->set_text(unsaved_status);
							} else {
								save_confirmation->set_ok_button_text(TTR("Save & Quit"));
								save_confirmation->set_text(unsaved_status);
							}
							save_confirmation->reset_size();
							save_confirmation->popup_centered();
							plugin_to_save = editor_data.get_editor_plugin(i);
							break;
						}
					}

					if (plugin_to_save) {
						break;
					}

					_discard_changes();
					break;
				}

				if (save_each) {
					tab_closing_menu_option = current_menu_option;
					for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
						tabs_to_close.push_back(editor_data.get_scene_path(i));
					}
					_proceed_closing_scene_tabs();
				} else {
					String unsaved_scenes;
					int i = _next_unsaved_scene(true, 0);
					while (i != -1) {
						unsaved_scenes += "\n            " + editor_data.get_edited_scene_root(i)->get_scene_file_path();
						i = _next_unsaved_scene(true, ++i);
					}
					if (p_option == PROJECT_RELOAD_CURRENT_PROJECT) {
						save_confirmation->set_ok_button_text(TTR("Save & Reload"));
						save_confirmation->set_text(TTR("Save changes to the following scene(s) before reloading?") + unsaved_scenes);
					} else {
						save_confirmation->set_ok_button_text(TTR("Save & Quit"));
						save_confirmation->set_text((p_option == SCENE_QUIT ? TTR("Save changes to the following scene(s) before quitting?") : TTR("Save changes to the following scene(s) before opening Project Manager?")) + unsaved_scenes);
					}
					save_confirmation->reset_size();
					save_confirmation->popup_centered();
				}

				DisplayServer::get_singleton()->window_request_attention();
				break;
			}
			_save_external_resources();
			_discard_changes();
		} break;
		case SPINNER_UPDATE_CONTINUOUSLY: {
			EditorSettings::get_singleton()->set("interface/editor/update_continuously", true);
			_update_update_spinner();
			show_accept(TTR("This option is deprecated. Situations where refresh must be forced are now considered a bug. Please report."), TTR("OK"));
		} break;
		case SPINNER_UPDATE_WHEN_CHANGED: {
			EditorSettings::get_singleton()->set("interface/editor/update_continuously", false);
			_update_update_spinner();
		} break;
		case SPINNER_UPDATE_SPINNER_HIDE: {
			EditorSettings::get_singleton()->set("interface/editor/show_update_spinner", 2); // Disabled
			_update_update_spinner();
		} break;
		case EDITOR_OPEN_SETTINGS: {
			editor_settings_dialog->popup_edit_settings();
		} break;
		case EDITOR_OPEN_DATA_FOLDER: {
			OS::get_singleton()->shell_show_in_file_manager(EditorPaths::get_singleton()->get_data_dir(), true);
		} break;
		case EDITOR_OPEN_CONFIG_FOLDER: {
			OS::get_singleton()->shell_show_in_file_manager(EditorPaths::get_singleton()->get_config_dir(), true);
		} break;
		case EDITOR_MANAGE_EXPORT_TEMPLATES: {
			export_template_manager->popup_manager();
		} break;
		case EDITOR_CONFIGURE_FBX_IMPORTER: {
#if !defined(ANDROID_ENABLED) && !defined(WEB_ENABLED)
			fbx_importer_manager->show_dialog();
#endif
		} break;
		case EDITOR_MANAGE_FEATURE_PROFILES: {
			feature_profile_manager->popup_centered_clamped(Size2(900, 800) * EDSCALE, 0.8);
		} break;
		case EDITOR_TOGGLE_FULLSCREEN: {
			DisplayServer::WindowMode mode = DisplayServer::get_singleton()->window_get_mode();
			if (mode == DisplayServer::WINDOW_MODE_FULLSCREEN || mode == DisplayServer::WINDOW_MODE_EXCLUSIVE_FULLSCREEN) {
				DisplayServer::get_singleton()->window_set_mode(prev_mode);
#ifdef ANDROID_ENABLED
				EditorSettings::get_singleton()->set("_is_editor_fullscreen", false);
				EditorSettings::get_singleton()->save();
#endif
			} else {
				prev_mode = mode;
				DisplayServer::get_singleton()->window_set_mode(DisplayServer::WINDOW_MODE_FULLSCREEN);
#ifdef ANDROID_ENABLED
				EditorSettings::get_singleton()->set("_is_editor_fullscreen", true);
				EditorSettings::get_singleton()->save();
#endif
			}
		} break;
		case EDITOR_TAKE_SCREENSHOT: {
			screenshot_timer->start();
		} break;
		case SETTINGS_PICK_MAIN_SCENE: {
			file->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
			List<String> extensions;
			ResourceLoader::get_recognized_extensions_for_type("PackedScene", &extensions);
			file->clear_filters();
			for (const String &extension : extensions) {
				file->add_filter("*." + extension, extension.to_upper());
			}

			Node *scene = editor_data.get_edited_scene_root();
			if (scene) {
				file->set_current_path(scene->get_scene_file_path());
			}
			file->set_title(TTR("Pick a Main Scene"));
			file->popup_file_dialog();

		} break;
		case HELP_SEARCH: {
			emit_signal(SNAME("request_help_search"), "");
		} break;
		case EDITOR_COMMAND_PALETTE: {
			command_palette->open_popup();
		} break;
		case HELP_DOCS: {
			OS::get_singleton()->shell_open(GODOT_VERSION_DOCS_URL "/");
		} break;
		case HELP_FORUM: {
			OS::get_singleton()->shell_open("https://forum.godotengine.org/");
		} break;
		case HELP_REPORT_A_BUG: {
			OS::get_singleton()->shell_open("https://github.com/godotengine/godot/issues");
		} break;
		case HELP_COPY_SYSTEM_INFO: {
			String info = _get_system_info();
			DisplayServer::get_singleton()->clipboard_set(info);
		} break;
		case HELP_SUGGEST_A_FEATURE: {
			OS::get_singleton()->shell_open("https://github.com/godotengine/godot-proposals#readme");
		} break;
		case HELP_SEND_DOCS_FEEDBACK: {
			OS::get_singleton()->shell_open("https://github.com/godotengine/godot-docs/issues");
		} break;
		case HELP_COMMUNITY: {
			OS::get_singleton()->shell_open("https://godotengine.org/community");
		} break;
		case HELP_ABOUT: {
			about->popup_centered(Size2(780, 500) * EDSCALE);
		} break;
		case HELP_SUPPORT_GODOT_DEVELOPMENT: {
			OS::get_singleton()->shell_open("https://fund.godotengine.org/?ref=help_menu");
		} break;
	}
}

void EditorNode::_dialog_action(String p_file) {
	switch (current_menu_option) {
		case SCENE_NEW_INHERITED_SCENE: {
			Node *scene = editor_data.get_edited_scene_root();
			// If the previous scene is rootless, just close it in favor of the new one.
			if (!scene) {
				_menu_option_confirm(SCENE_CLOSE, true);
			}

			load_scene(p_file, false, true);
		} break;
		case SCENE_OPEN_SCENE: {
			load_scene(p_file);
		} break;
		case SETTINGS_PICK_MAIN_SCENE: {
			ProjectSettings::get_singleton()->set("application/run/main_scene", ResourceUID::path_to_uid(p_file));
			ProjectSettings::get_singleton()->save();
			// TODO: Would be nice to show the project manager opened with the highlighted field.

			project_run_bar->play_main_scene((bool)pick_main_scene->get_meta("from_native", false));
		} break;
		case SCENE_CLOSE:
		case SCENE_TAB_CLOSE:
		case SCENE_SAVE_SCENE:
		case SCENE_MULTI_SAVE_AS_SCENE:
		case SCENE_SAVE_AS_SCENE: {
			int scene_idx = (current_menu_option == SCENE_SAVE_SCENE || current_menu_option == SCENE_SAVE_AS_SCENE || current_menu_option == SCENE_MULTI_SAVE_AS_SCENE) ? -1 : tab_closing_idx;

			if (file->get_file_mode() == EditorFileDialog::FILE_MODE_SAVE_FILE) {
				bool same_open_scene = false;
				for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
					if (editor_data.get_scene_path(i) == p_file && i != scene_idx) {
						same_open_scene = true;
					}
				}

				if (same_open_scene) {
					show_warning(TTR("Can't overwrite scene that is still open!"));
					return;
				}

				save_default_environment();
				_save_scene_with_preview(p_file, scene_idx);
				_add_to_recent_scenes(p_file);
				save_editor_layout_delayed();

				if (scene_idx != -1) {
					_discard_changes();
				} else {
					// Update the path of the edited scene to ensure later do/undo action history matches.
					editor_data.set_scene_path(editor_data.get_edited_scene(), p_file);
				}
			}

			if (current_menu_option == SCENE_MULTI_SAVE_AS_SCENE) {
				_proceed_save_asing_scene_tabs();
			}

		} break;

		case SAVE_AND_RUN: {
			if (file->get_file_mode() == EditorFileDialog::FILE_MODE_SAVE_FILE) {
				save_default_environment();
				_save_scene_with_preview(p_file);
				project_run_bar->play_custom_scene(p_file);
			}
		} break;

		case SAVE_AND_RUN_MAIN_SCENE: {
			ProjectSettings::get_singleton()->set("application/run/main_scene", ResourceUID::path_to_uid(p_file));
			ProjectSettings::get_singleton()->save();

			if (file->get_file_mode() == EditorFileDialog::FILE_MODE_SAVE_FILE) {
				save_default_environment();
				_save_scene_with_preview(p_file);
				project_run_bar->play_main_scene((bool)pick_main_scene->get_meta("from_native", false));
			}
		} break;

		case SAVE_AND_SET_MAIN_SCENE: {
			_save_scene(p_file);
			_menu_option_confirm(SCENE_TAB_SET_AS_MAIN_SCENE, true);
		} break;

		case FILE_EXPORT_MESH_LIBRARY: {
			const Dictionary &fd_options = file_export_lib->get_selected_options();
			bool merge_with_existing_library = fd_options.get(TTR("Merge With Existing"), true);
			bool apply_mesh_instance_transforms = fd_options.get(TTR("Apply MeshInstance Transforms"), false);

			Ref<MeshLibrary> ml;
			if (merge_with_existing_library && FileAccess::exists(p_file)) {
				ml = ResourceLoader::load(p_file, "MeshLibrary");

				if (ml.is_null()) {
					show_accept(TTR("Can't load MeshLibrary for merging!"), TTR("OK"));
					return;
				}
			}

			if (ml.is_null()) {
				ml.instantiate();
			}

			MeshLibraryEditor::update_library_file(editor_data.get_edited_scene_root(), ml, merge_with_existing_library, apply_mesh_instance_transforms);

			Error err = ResourceSaver::save(ml, p_file);
			if (err) {
				show_accept(TTR("Error saving MeshLibrary!"), TTR("OK"));
				return;
			} else if (ResourceCache::has(p_file)) {
				// Make sure MeshLibrary is updated in the editor.
				ResourceLoader::load(p_file)->reload_from_file();
			}

		} break;

		case PROJECT_PACK_AS_ZIP: {
			ProjectZIPPacker::pack_project_zip(p_file);
			{
				Ref<FileAccess> f = FileAccess::open(p_file, FileAccess::READ);
				ERR_FAIL_COND_MSG(f.is_null(), vformat("Unable to create ZIP file at: %s. Check for write permissions and whether you have enough disk space left.", p_file));
			}

		} break;

		case RESOURCE_SAVE:
		case RESOURCE_SAVE_AS: {
			ERR_FAIL_COND(saving_resource.is_null());
			save_resource_in_path(saving_resource, p_file);

			saving_resource = Ref<Resource>();
			ObjectID current_id = editor_history.get_current();
			Object *current_obj = current_id.is_valid() ? ObjectDB::get_instance(current_id) : nullptr;
			ERR_FAIL_NULL(current_obj);
			current_obj->notify_property_list_changed();
		} break;
		case LAYOUT_SAVE: {
			if (p_file.is_empty()) {
				return;
			}

			Ref<ConfigFile> config;
			config.instantiate();
			Error err = config->load(EditorSettings::get_singleton()->get_editor_layouts_config());

			if (err == ERR_FILE_CANT_OPEN || err == ERR_FILE_NOT_FOUND) {
				config.instantiate();
			} else if (err != OK) {
				show_warning(TTR("An error occurred while trying to save the editor layout.\nMake sure the editor's user data path is writable."));
				return;
			}

			editor_dock_manager->save_docks_to_config(config, p_file);

			config->save(EditorSettings::get_singleton()->get_editor_layouts_config());

			layout_dialog->hide();
			_update_layouts_menu();

			if (p_file == "Default") {
				show_warning(TTR("Default editor layout overridden.\nTo restore the Default layout to its base settings, use the Delete Layout option and delete the Default layout."));
			}

		} break;
		case LAYOUT_DELETE: {
			Ref<ConfigFile> config;
			config.instantiate();
			Error err = config->load(EditorSettings::get_singleton()->get_editor_layouts_config());

			if (err != OK || !config->has_section(p_file)) {
				show_warning(TTR("Layout name not found!"));
				return;
			}

			// Erase key values.
			Vector<String> keys = config->get_section_keys(p_file);
			for (const String &key : keys) {
				config->set_value(p_file, key, Variant());
			}

			config->save(EditorSettings::get_singleton()->get_editor_layouts_config());

			layout_dialog->hide();
			_update_layouts_menu();

			if (p_file == "Default") {
				show_warning(TTR("Restored the Default layout to its base settings."));
			}

		} break;
		default: {
			// Save scene?
			if (file->get_file_mode() == EditorFileDialog::FILE_MODE_SAVE_FILE) {
				_save_scene_with_preview(p_file);
			}

		} break;
	}
}

void EditorNode::_tool_menu_option(int p_idx) {
	switch (tool_menu->get_item_id(p_idx)) {
		case TOOLS_ORPHAN_RESOURCES: {
			orphan_resources->show();
		} break;
		case TOOLS_BUILD_PROFILE_MANAGER: {
			build_profile_manager->popup_centered_clamped(Size2(700, 800) * EDSCALE, 0.8);
		} break;
		case TOOLS_PROJECT_UPGRADE: {
			project_upgrade_tool->popup_dialog();
		} break;
		case TOOLS_CUSTOM: {
			if (tool_menu->get_item_submenu(p_idx) == "") {
				Callable callback = tool_menu->get_item_metadata(p_idx);
				Callable::CallError ce;
				Variant result;
				callback.callp(nullptr, 0, result, ce);

				if (ce.error != Callable::CallError::CALL_OK) {
					String err = Variant::get_callable_error_text(callback, nullptr, 0, ce);
					ERR_PRINT("Error calling function from tool menu: " + err);
				}
			} // Else it's a submenu so don't do anything.
		} break;
	}
}

void EditorNode::_export_as_menu_option(int p_idx) {
	if (p_idx == 0) { // MeshLibrary
		current_menu_option = FILE_EXPORT_MESH_LIBRARY;

		if (!editor_data.get_edited_scene_root()) {
			show_accept(TTR("This operation can't be done without a scene."), TTR("OK"));
			return;
		}

		List<String> extensions;
		Ref<MeshLibrary> ml(memnew(MeshLibrary));
		ResourceSaver::get_recognized_extensions(ml, &extensions);
		file_export_lib->clear_filters();
		for (const String &E : extensions) {
			file_export_lib->add_filter("*." + E);
		}

		file_export_lib->set_title(TTR("Export Mesh Library"));
		file_export_lib->popup_file_dialog();
	} else { // Custom menu options added by plugins
		if (export_as_menu->get_item_submenu(p_idx).is_empty()) { // If not a submenu
			Callable callback = export_as_menu->get_item_metadata(p_idx);
			Callable::CallError ce;
			Variant result;
			callback.callp(nullptr, 0, result, ce);

			if (ce.error != Callable::CallError::CALL_OK) {
				String err = Variant::get_callable_error_text(callback, nullptr, 0, ce);
				ERR_PRINT("Error calling function from export_as menu: " + err);
			}
		}
	}
}

