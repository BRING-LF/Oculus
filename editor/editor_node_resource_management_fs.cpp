/**************************************************************************/
/*  editor_node_resource_management_fs.cpp                              */
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
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource.h"
#include "core/io/resource_loader.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "editor/animation/animation_player_editor_plugin.h"
#include "editor/export/editor_export.h"
#include "editor/export/export_template_manager.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/scene/editor_scene_tabs.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/tree.h"
#include "scene/main/scene_tree.h"
#include "scene/main/timer.h"
#include "scene/main/window.h"
#include "servers/rendering/rendering_server.h"

void EditorNode::_resources_changed(const Vector<String> &p_resources) {
	List<Ref<Resource>> changed;

	int rc = p_resources.size();
	for (int i = 0; i < rc; i++) {
		Ref<Resource> res = ResourceCache::get_ref(p_resources.get(i));
		if (res.is_null()) {
			continue;
		}

		if (!res->editor_can_reload_from_file()) {
			continue;
		}
		if (!res->get_path().is_resource_file() && !res->get_path().is_absolute_path()) {
			continue;
		}
		if (!FileAccess::exists(res->get_path())) {
			continue;
		}

		if (!res->get_import_path().is_empty()) {
			// This is an imported resource, will be reloaded if reimported via the _resources_reimported() callback.
			continue;
		}

		changed.push_back(res);
	}

	if (changed.size()) {
		for (Ref<Resource> &res : changed) {
			res->reload_from_file();
		}
	}
}

void EditorNode::_fs_changed() {
	for (FileDialog *E : file_dialogs) {
		E->invalidate();
	}

	_mark_unsaved_scenes();

	// FIXME: Move this to a cleaner location, it's hacky to do this in _fs_changed.
	String export_error;
	Error err = OK;
	// It's important to wait for the first scan to finish; otherwise, scripts or resources might not be imported.
	if (!export_defer.preset.is_empty() && !EditorFileSystem::get_singleton()->is_scanning()) {
		String preset_name = export_defer.preset;
		// Ensures export_project does not loop infinitely, because notifications may
		// come during the export.
		export_defer.preset = "";
		Ref<EditorExportPreset> export_preset;
		for (int i = 0; i < EditorExport::get_singleton()->get_export_preset_count(); ++i) {
			export_preset = EditorExport::get_singleton()->get_export_preset(i);
			if (export_preset->get_name() == preset_name) {
				break;
			}
			export_preset.unref();
		}

		if (export_preset.is_null()) {
			Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
			if (da->file_exists("res://export_presets.cfg")) {
				err = FAILED;
				export_error = vformat(
						"Invalid export preset name: %s.\nThe following presets were detected in this project's `export_presets.cfg`:\n\n",
						preset_name);
				for (int i = 0; i < EditorExport::get_singleton()->get_export_preset_count(); ++i) {
					// Write the preset name between double quotes since it needs to be written between quotes on the command line if it contains spaces.
					export_error += vformat("        \"%s\"\n", EditorExport::get_singleton()->get_export_preset(i)->get_name());
				}
			} else {
				err = FAILED;
				export_error = "This project doesn't have an `export_presets.cfg` file at its root.\nCreate an export preset from the \"Project > Export\" dialog and try again.";
			}
		} else {
			Ref<EditorExportPlatform> platform = export_preset->get_platform();
			const String export_path = export_defer.path.is_empty() ? export_preset->get_export_path() : export_defer.path;
			if (export_path.is_empty()) {
				err = FAILED;
				export_error = vformat("Export preset \"%s\" doesn't have a default export path, and none was specified.", preset_name);
			} else if (platform.is_null()) {
				err = FAILED;
				export_error = vformat("Export preset \"%s\" doesn't have a matching platform.", preset_name);
			} else {
				export_preset->update_value_overrides();
				if (export_defer.pack_only) { // Only export .pck or .zip data pack.
					if (export_path.ends_with(".zip")) {
						if (export_defer.patch) {
							err = platform->export_zip_patch(export_preset, export_defer.debug, export_path, export_defer.patches);
						} else {
							err = platform->export_zip(export_preset, export_defer.debug, export_path);
						}
					} else if (export_path.ends_with(".pck")) {
						if (export_defer.patch) {
							err = platform->export_pack_patch(export_preset, export_defer.debug, export_path, export_defer.patches);
						} else {
							err = platform->export_pack(export_preset, export_defer.debug, export_path);
						}
					} else {
						ERR_PRINT(vformat("Export path \"%s\" doesn't end with a supported extension.", export_path));
						err = FAILED;
					}
				} else { // Normal project export.
					String config_error;
					bool missing_templates;
					if (export_defer.android_build_template) {
						export_template_manager->install_android_template(export_preset);
					}
					if (!platform->can_export(export_preset, config_error, missing_templates, export_defer.debug)) {
						ERR_PRINT(vformat("Cannot export project with preset \"%s\" due to configuration errors:\n%s", preset_name, config_error));
						err = missing_templates ? ERR_FILE_NOT_FOUND : ERR_UNCONFIGURED;
					} else {
						platform->clear_messages();
						err = platform->export_project(export_preset, export_defer.debug, export_path);
					}
				}
				if (err != OK) {
					export_error = vformat("Project export for preset \"%s\" failed.", preset_name);
				} else if (platform->get_worst_message_type() >= EditorExportPlatform::EXPORT_MESSAGE_WARNING) {
					export_error = vformat("Project export for preset \"%s\" completed with warnings.", preset_name);
				}
			}
		}

		if (err != OK) {
			ERR_PRINT(export_error);
			_exit_editor(EXIT_FAILURE);
			return;
		}
		if (!export_error.is_empty()) {
			WARN_PRINT(export_error);
		}
		_exit_editor(EXIT_SUCCESS);
	}
}

void EditorNode::_resources_reimporting(const Vector<String> &p_resources) {
	// This will copy all the modified properties of the nodes into 'scenes_modification_table'
	// before they are actually reimported. It's important to do this before the reimportation
	// because if a mesh is present in an inherited scene, the resource will be modified in
	// the inherited scene. Then, get_modified_properties_for_node will return the mesh property,
	// which will trigger a recopy of the previous mesh, preventing the reload.
	scenes_modification_table.clear();
	scenes_reimported.clear();
	resources_reimported.clear();
	EditorFileSystem *editor_file_system = EditorFileSystem::get_singleton();
	for (const String &res_path : p_resources) {
		// It's faster to use EditorFileSystem::get_file_type than fetching the resource type from disk.
		// This makes a big difference when reimporting many resources.
		String file_type = editor_file_system->get_file_type(res_path);
		if (file_type.is_empty()) {
			file_type = ResourceLoader::get_resource_type(res_path);
		}
		if (file_type == "PackedScene") {
			scenes_reimported.push_back(res_path);
		} else {
			resources_reimported.push_back(res_path);
		}
	}

	if (scenes_reimported.size() > 0) {
		preload_reimporting_with_path_in_edited_scenes(scenes_reimported);
	}
}

void EditorNode::_resources_reimported(const Vector<String> &p_resources) {
	int current_tab = scene_tabs->get_current_tab();

	for (const String &res_path : resources_reimported) {
		if (!ResourceCache::has(res_path)) {
			// Not loaded, no need to reload.
			continue;
		}
		// Reload normally.
		Ref<Resource> resource = ResourceCache::get_ref(res_path);
		if (resource.is_valid()) {
			resource->reload_from_file();
		}
	}

	// Editor may crash when related animation is playing while re-importing GLTF scene, stop it in advance.
	AnimationPlayer *ap = AnimationPlayerEditor::get_singleton()->get_player();
	if (ap && scenes_reimported.size() > 0) {
		ap->stop(true);
	}

	// Only refresh the current scene tab if it's been reimported.
	// Otherwise the scene tab will try to grab focus unnecessarily.
	bool should_refresh_current_scene_tab = false;
	const String current_scene_tab = editor_data.get_scene_path(current_tab);
	for (const String &E : scenes_reimported) {
		if (!should_refresh_current_scene_tab && E == current_scene_tab) {
			should_refresh_current_scene_tab = true;
		}
		reload_scene(E);
	}

	reload_instances_with_path_in_edited_scenes();

	scenes_modification_table.clear();
	scenes_reimported.clear();
	resources_reimported.clear();

	if (should_refresh_current_scene_tab) {
		_set_current_scene_nocheck(current_tab);
	}
}

void EditorNode::_sources_changed(bool p_exist) {
	if (waiting_for_first_scan) {
		waiting_for_first_scan = false;

		OS::get_singleton()->benchmark_end_measure("Editor", "First Scan");

		// Reload the global shader variables, but this time
		// loading textures, as they are now properly imported.
		RenderingServer::get_singleton()->global_shader_parameters_load_settings(true);

		_load_editor_layout();

		if (!defer_load_scene.is_empty()) {
			OS::get_singleton()->benchmark_begin_measure("Editor", "Load Scene");

			load_scene(defer_load_scene);
			defer_load_scene = "";

			OS::get_singleton()->benchmark_end_measure("Editor", "Load Scene");
			OS::get_singleton()->benchmark_dump();
		}

		// Start preview thread now that it's safe.
		if (!singleton->cmdline_mode) {
			EditorResourcePreview::get_singleton()->start();
		}
		get_tree()->create_timer(1.0f)->connect("timeout", callable_mp(this, &EditorNode::_remove_lock_file));
	}
}

void EditorNode::_remove_lock_file() {
	OS::get_singleton()->remove_lock_file();
}

void EditorNode::_scan_external_changes() {
	disk_changed_list->clear();
	TreeItem *r = disk_changed_list->create_item();
	disk_changed_list->set_hide_root(true);
	bool need_reload = false;

	disk_changed_scenes.clear();
	disk_changed_project = false;

	// Check if any edited scene has changed.
	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);

		const String scene_path = editor_data.get_scene_path(i);

		if (scene_path == "" || !da->file_exists(scene_path)) {
			continue;
		}

		uint64_t last_date = editor_data.get_scene_modified_time(i);
		uint64_t date = FileAccess::get_modified_time(scene_path);

		if (date > last_date) {
			TreeItem *ti = disk_changed_list->create_item(r);
			ti->set_text(0, scene_path.get_file());
			need_reload = true;
			disk_changed_scenes.push_back(scene_path);
		}
	}

	String project_settings_path = ProjectSettings::get_singleton()->get_resource_path().path_join("project.godot");
	if (FileAccess::get_modified_time(project_settings_path) > ProjectSettings::get_singleton()->get_last_saved_time()) {
		TreeItem *ti = disk_changed_list->create_item(r);
		ti->set_text(0, "project.godot");
		need_reload = true;
		disk_changed_project = true;
	}

	if (need_reload) {
		callable_mp((Window *)disk_changed, &Window::popup_centered_ratio).call_deferred(0.3);
	}
}

void EditorNode::_reload_project_settings() {
	ProjectSettings::get_singleton()->setup(ProjectSettings::get_singleton()->get_resource_path(), String(), true, true);
}

