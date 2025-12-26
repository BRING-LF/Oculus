/**************************************************************************/
/*  editor_node_scene_save.cpp                                            */
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
#include "core/io/dir_access.h"
#include "core/io/resource.h"
#include "core/io/resource_saver.h"
#include "core/io/resource_loader.h"
#include "core/io/file_access.h"
#include "editor/file_system/editor_paths.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/settings/editor_settings.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_toaster.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/inspector/editor_preview_plugins.h"
#include "editor/run/editor_run_bar.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/scene/editor_scene_tabs.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_feature_profile.h"
#include "scene/animation/animation_mixer.h"
#include "scene/animation/animation_player.h"
#include "scene/animation/animation_tree.h"
#include "scene/main/viewport.h"
#include "scene/resources/packed_scene.h"

static void _reset_animation_mixers(Node *p_node, List<Pair<AnimationMixer *, Ref<AnimatedValuesBackup>>> *r_anim_backups) {
	for (int i = 0; i < p_node->get_child_count(); i++) {
		AnimationMixer *mixer = Object::cast_to<AnimationMixer>(p_node->get_child(i));
		if (mixer && mixer->is_active() && mixer->is_reset_on_save_enabled() && mixer->can_apply_reset()) {
			AnimationTree *tree = Object::cast_to<AnimationTree>(p_node->get_child(i));
			if (tree) {
				AnimationPlayer *player = Object::cast_to<AnimationPlayer>(tree->get_node_or_null(tree->get_animation_player()));
				if (player && player->is_active() && player->is_reset_on_save_enabled() && player->can_apply_reset()) {
					continue; // Avoid to process reset/restore many times.
				}
			}
			Ref<AnimatedValuesBackup> backup = mixer->apply_reset();
			if (backup.is_valid()) {
				Pair<AnimationMixer *, Ref<AnimatedValuesBackup>> pair;
				pair.first = mixer;
				pair.second = backup;
				r_anim_backups->push_back(pair);
			}
		}
		_reset_animation_mixers(p_node->get_child(i), r_anim_backups);
	}
}

bool EditorNode::_find_and_save_resource(Ref<Resource> p_res, HashMap<Ref<Resource>, bool> &processed, int32_t flags) {
	if (p_res.is_null()) {
		return false;
	}

	if (processed.has(p_res)) {
		return processed[p_res];
	}

	bool changed = p_res->is_edited();
	p_res->set_edited(false);

	bool subchanged = _find_and_save_edited_subresources(p_res.ptr(), processed, flags);

	if (p_res->get_path().is_resource_file()) {
		if (changed || subchanged) {
			ResourceSaver::save(p_res, p_res->get_path(), flags);
		}
		processed[p_res] = false; // Because it's a file.
		return false;
	} else {
		processed[p_res] = changed;
		return changed;
	}
}

bool EditorNode::_find_and_save_edited_subresources(Object *obj, HashMap<Ref<Resource>, bool> &processed, int32_t flags) {
	bool ret_changed = false;
	List<PropertyInfo> pi;
	obj->get_property_list(&pi);
	for (const PropertyInfo &E : pi) {
		if (!(E.usage & PROPERTY_USAGE_STORAGE)) {
			continue;
		}

		switch (E.type) {
			case Variant::OBJECT: {
				Ref<Resource> res = obj->get(E.name);

				if (_find_and_save_resource(res, processed, flags)) {
					ret_changed = true;
				}

			} break;
			case Variant::ARRAY: {
				Array varray = obj->get(E.name);
				int len = varray.size();
				for (int i = 0; i < len; i++) {
					const Variant &v = varray.get(i);
					Ref<Resource> res = v;
					if (_find_and_save_resource(res, processed, flags)) {
						ret_changed = true;
					}
				}

			} break;
			case Variant::DICTIONARY: {
				Dictionary d = obj->get(E.name);
				for (const KeyValue<Variant, Variant> &kv : d) {
					Ref<Resource> res = kv.value;
					if (_find_and_save_resource(res, processed, flags)) {
						ret_changed = true;
					}
				}
			} break;
			default: {
			}
		}
	}

	return ret_changed;
}

void EditorNode::_save_edited_subresources(Node *scene, HashMap<Ref<Resource>, bool> &processed, int32_t flags) {
	_find_and_save_edited_subresources(scene, processed, flags);

	for (int i = 0; i < scene->get_child_count(); i++) {
		Node *n = scene->get_child(i);
		if (n->get_owner() != editor_data.get_edited_scene_root()) {
			continue;
		}
		_save_edited_subresources(n, processed, flags);
	}
}

void EditorNode::_find_node_types(Node *p_node, int &count_2d, int &count_3d) {
	if (p_node->is_class("Viewport") || (p_node != editor_data.get_edited_scene_root() && p_node->get_owner() != editor_data.get_edited_scene_root())) {
		return;
	}

	if (p_node->is_class("CanvasItem")) {
		count_2d++;
	} else if (p_node->is_class("Node3D")) {
		count_3d++;
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		_find_node_types(p_node->get_child(i), count_2d, count_3d);
	}
}

bool EditorNode::_validate_scene_recursive(const String &p_filename, Node *p_node) {
	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *child = p_node->get_child(i);
		if (child->get_scene_file_path() == p_filename) {
			return true;
		}

		if (_validate_scene_recursive(p_filename, child)) {
			return true;
		}
	}

	return false;
}

void EditorNode::_close_save_scene_progress() {
	memdelete_notnull(save_scene_progress);
	save_scene_progress = nullptr;
}

int EditorNode::_save_external_resources(bool p_also_save_external_data) {
	// Save external resources and its subresources if any was modified.

	int flg = 0;
	if (EDITOR_GET("filesystem/on_save/compress_binary_resources")) {
		flg |= ResourceSaver::FLAG_COMPRESS;
	}
	flg |= ResourceSaver::FLAG_REPLACE_SUBRESOURCE_PATHS;

	HashSet<String> edited_resources;
	int saved = 0;
	List<Ref<Resource>> cached;
	ResourceCache::get_cached_resources(&cached);

	for (Ref<Resource> res : cached) {
		if (!res->is_edited()) {
			continue;
		}

		String path = res->get_path();
		if (path.begins_with("res://")) {
			int subres_pos = path.find("::");
			if (subres_pos == -1) {
				// Actual resource.
				edited_resources.insert(path);
			} else {
				edited_resources.insert(path.substr(0, subres_pos));
			}
		}

		res->set_edited(false);
	}

	bool script_was_saved = false;
	for (const String &E : edited_resources) {
		Ref<Resource> res = ResourceCache::get_ref(E);
		if (res.is_null()) {
			continue; // Maybe it was erased in a thread, who knows.
		}
		Ref<PackedScene> ps = res;
		if (ps.is_valid()) {
			continue; // Do not save PackedScenes, this will mess up the editor.
		}
		if (!script_was_saved) {
			Ref<Script> scr = res;
			script_was_saved = scr.is_valid();
		}
		ResourceSaver::save(res, res->get_path(), flg);
		saved++;
	}

	if (script_was_saved) {
		ScriptEditor::get_singleton()->update_script_times();
	}

	if (p_also_save_external_data) {
		for (int i = 0; i < editor_data.get_editor_plugin_count(); i++) {
			EditorPlugin *plugin = editor_data.get_editor_plugin(i);
			if (!plugin->get_unsaved_status().is_empty()) {
				plugin->save_external_data();
				saved++;
			}
		}
	}

	EditorSettings::get_singleton()->save_project_metadata();
	EditorUndoRedoManager::get_singleton()->set_history_as_saved(EditorUndoRedoManager::GLOBAL_HISTORY);
	_update_unsaved_cache();

	return saved;
}

void EditorNode::_save_scene_silently() {
	// Save scene without displaying progress dialog. Used to work around
	// errors about parent node being busy setting up children
	// when Save on Focus Loss kicks in.
	Node *scene = editor_data.get_edited_scene_root();
	if (scene && !scene->get_scene_file_path().is_empty() && DirAccess::exists(scene->get_scene_file_path().get_base_dir())) {
		_save_scene(scene->get_scene_file_path());
		save_editor_layout_delayed();
	}
}

void EditorNode::_save_scene_with_preview(String p_file, int p_idx) {
	save_scene_progress = memnew(EditorProgress("save", TTR("Saving Scene"), 4));

	if (editor_data.get_edited_scene_root() != nullptr) {
		save_scene_progress->step(TTR("Analyzing"), 0);

		int c2d = 0;
		int c3d = 0;

		_find_node_types(editor_data.get_edited_scene_root(), c2d, c3d);

		save_scene_progress->step(TTR("Creating Thumbnail"), 1);
		// Current view?

		Ref<Image> img;
		// If neither 3D or 2D nodes are present, make a 1x1 black texture.
		// We cannot fallback on the 2D editor, because it may not have been used yet,
		// which would result in an invalid texture.
		if (c3d == 0 && c2d == 0) {
			img.instantiate();
			img->initialize_data(1, 1, false, Image::FORMAT_RGB8);
		} else if (c3d < c2d) {
			Ref<ViewportTexture> viewport_texture = scene_root->get_texture();
			if (viewport_texture->get_width() > 0 && viewport_texture->get_height() > 0) {
				img = viewport_texture->get_image();
			}
		} else {
			// The 3D editor may be disabled as a feature, but scenes can still be opened.
			// This check prevents the preview from regenerating in case those scenes are then saved.
			// The preview will be generated if no feature profile is set (as the 3D editor is enabled by default).
			Ref<EditorFeatureProfile> profile = feature_profile_manager->get_current_profile();
			if (profile.is_null() || !profile->is_feature_disabled(EditorFeatureProfile::FEATURE_3D)) {
				img = Node3DEditor::get_singleton()->get_editor_viewport(0)->get_viewport_node()->get_texture()->get_image();
			}
		}

		if (img.is_valid() && img->get_width() > 0 && img->get_height() > 0) {
			img = img->duplicate();

			save_scene_progress->step(TTR("Creating Thumbnail"), 3);

			int preview_size = EDITOR_GET("filesystem/file_dialog/thumbnail_size");
			preview_size *= EDSCALE;

			// Consider a square region.
			int vp_size = MIN(img->get_width(), img->get_height());
			int x = (img->get_width() - vp_size) / 2;
			int y = (img->get_height() - vp_size) / 2;

			if (vp_size < preview_size) {
				// Just square it.
				img->crop_from_point(x, y, vp_size, vp_size);
			} else {
				int ratio = vp_size / preview_size;
				int size = preview_size * MAX(1, ratio / 2);

				x = (img->get_width() - size) / 2;
				y = (img->get_height() - size) / 2;

				img->crop_from_point(x, y, size, size);
				img->resize(preview_size, preview_size, Image::INTERPOLATE_LANCZOS);
			}
			img->convert(Image::FORMAT_RGB8);

			// Save thumbnail directly, as thumbnailer may not update due to actual scene not changing md5.
			String temp_path = EditorPaths::get_singleton()->get_cache_dir();
			String cache_base = ProjectSettings::get_singleton()->globalize_path(p_file).md5_text();
			cache_base = temp_path.path_join("resthumb-" + cache_base);

			// Does not have it, try to load a cached thumbnail.
			post_process_preview(img);
			img->save_png(cache_base + ".png");
		}
	}

	save_scene_progress->step(TTR("Saving Scene"), 4);
	_save_scene(p_file, p_idx);

	if (!singleton->cmdline_mode) {
		EditorResourcePreview::get_singleton()->check_for_invalidation(p_file);
	}

	_close_save_scene_progress();
}

void EditorNode::_save_scene(String p_file, int idx) {
	ERR_FAIL_COND_MSG(!saving_scene.is_empty() && saving_scene == p_file, "Scene saved while already being saved!");

	Node *scene = editor_data.get_edited_scene_root(idx);

	if (!scene) {
		show_accept(TTR("This operation can't be done without a tree root."), TTR("OK"));
		return;
	}

	if (!scene->get_scene_file_path().is_empty() && _validate_scene_recursive(scene->get_scene_file_path(), scene)) {
		show_accept(TTR("This scene can't be saved because there is a cyclic instance inclusion.\nPlease resolve it and then attempt to save again."), TTR("OK"));
		return;
	}

	scene->propagate_notification(NOTIFICATION_EDITOR_PRE_SAVE);

	editor_data.apply_changes_in_editors();
	save_default_environment();
	List<Pair<AnimationMixer *, Ref<AnimatedValuesBackup>>> anim_backups;
	_reset_animation_mixers(scene, &anim_backups);
	_save_editor_states(p_file, idx);

	Ref<PackedScene> sdata;

	if (ResourceCache::has(p_file)) {
		// Something may be referencing this resource and we are good with that.
		// We must update it, but also let the previous scene state go, as
		// old version still work for referencing changes in instantiated or inherited scenes.

		sdata = ResourceCache::get_ref(p_file);
		if (sdata.is_valid()) {
			sdata->recreate_state();
		} else {
			sdata.instantiate();
		}
	} else {
		sdata.instantiate();
	}
	Error err = sdata->pack(scene);

	if (err != OK) {
		show_accept(TTR("Couldn't save scene. Likely dependencies (instances or inheritance) couldn't be satisfied."), TTR("OK"));
		return;
	}

	int flg = 0;
	if (EDITOR_GET("filesystem/on_save/compress_binary_resources")) {
		flg |= ResourceSaver::FLAG_COMPRESS;
	}
	flg |= ResourceSaver::FLAG_REPLACE_SUBRESOURCE_PATHS;

	err = ResourceSaver::save(sdata, p_file, flg);

	// This needs to be emitted before saving external resources.
	emit_signal(SNAME("scene_saved"), p_file);
	editor_data.notify_scene_saved(p_file);

	_save_external_resources();
	saving_scene = p_file; // Some editors may save scenes of built-in resources as external data, so avoid saving this scene again.
	editor_data.save_editor_external_data();
	saving_scene = "";

	for (Pair<AnimationMixer *, Ref<AnimatedValuesBackup>> &E : anim_backups) {
		E.first->restore(E.second);
	}

	if (err == OK) {
		scene->set_scene_file_path(ProjectSettings::get_singleton()->localize_path(p_file));
		editor_data.set_scene_as_saved(idx);
		editor_data.set_scene_modified_time(idx, FileAccess::get_modified_time(p_file));

		if (EDITOR_GET("filesystem/on_save/warn_on_saving_large_text_resources")) {
			if (p_file.ends_with(".tscn") || p_file.ends_with(".tres")) {
				const int64_t file_size = FileAccess::get_size(p_file);
				if (file_size >= LARGE_RESOURCE_WARNING_SIZE_THRESHOLD) {
					// File is larger than 500 KiB, likely because it contains binary data serialized as Base64.
					// This is slow to save and load, so warn the user.
					EditorToaster::get_singleton()->popup_str(
							vformat(TTR("The text-based scene at path \"%s\" is large on disk (%s), likely because it has embedded binary data.\nThis slows down scene saving and loading.\nConsider saving its binary subresource(s) to a binary `.res` file or saving the scene as a binary `.scn` file.\nThis warning can be disabled in the Editor Settings (FileSystem > On Save > Warn on Saving Large Text Resources)."), p_file, String::humanize_size(file_size)), EditorToaster::SEVERITY_WARNING);
				}
			}
		}

		editor_folding.save_scene_folding(scene, p_file);

		_update_title();
		scene_tabs->update_scene_tabs();
	} else {
		_dialog_display_save_error(p_file, err);
	}

	scene->propagate_notification(NOTIFICATION_EDITOR_POST_SAVE);
	_update_unsaved_cache();
}

void EditorNode::save_all_scenes() {
	project_run_bar->stop_playing();
	_save_all_scenes();
}

void EditorNode::save_scene_if_open(const String &p_scene_path) {
	int idx = editor_data.get_edited_scene_from_path(p_scene_path);
	if (idx >= 0) {
		_save_scene(p_scene_path, idx);
	}
}

void EditorNode::save_scene_list(const HashSet<String> &p_scene_paths) {
	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		Node *scene = editor_data.get_edited_scene_root(i);

		if (scene && p_scene_paths.has(scene->get_scene_file_path())) {
			_save_scene(scene->get_scene_file_path(), i);
		}
	}
}

void EditorNode::_save_all_scenes() {
	scenes_to_save_as.clear(); // In case saving was canceled before.
	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		if (!_is_scene_unsaved(i)) {
			continue;
		}

		const Node *scene = editor_data.get_edited_scene_root(i);
		ERR_FAIL_NULL(scene);

		const String &scene_path = scene->get_scene_file_path();
		if (scene_path.is_empty() || !DirAccess::exists(scene_path.get_base_dir())) {
			scenes_to_save_as.push_back(i);
			continue;
		}

		if (i == editor_data.get_edited_scene()) {
			_save_scene_with_preview(scene_path);
		} else {
			_save_scene(scene_path, i);
		}
	}
	save_default_environment();

	if (!scenes_to_save_as.is_empty()) {
		_proceed_save_asing_scene_tabs();
	}
}

void EditorNode::_mark_unsaved_scenes() {
	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		Node *node = editor_data.get_edited_scene_root(i);
		if (!node) {
			continue;
		}

		String path = node->get_scene_file_path();
		if (!path.is_empty() && !FileAccess::exists(path)) {
			// Mark scene tab as unsaved if the file is gone.
			EditorUndoRedoManager::get_singleton()->set_history_as_unsaved(editor_data.get_scene_history_id(i));
		}
	}

	_update_title();
	scene_tabs->update_scene_tabs();
}

void EditorNode::save_before_run() {
	current_menu_option = SAVE_AND_RUN;
	_menu_option_confirm(SCENE_SAVE_AS_SCENE, true);
	file->set_title(TTR("Save scene before running..."));
}

void EditorNode::try_autosave() {
	if (!bool(EDITOR_GET("run/auto_save/save_before_running"))) {
		return;
	}

	if (unsaved_cache) {
		Node *scene = editor_data.get_edited_scene_root();

		if (scene && !scene->get_scene_file_path().is_empty()) { // Only autosave if there is a scene and if it has a path.
			_save_scene_with_preview(scene->get_scene_file_path());
		}
	}
	_menu_option(SCENE_SAVE_ALL_SCENES);
	editor_data.save_editor_external_data();
}

bool EditorNode::_is_scene_unsaved(int p_idx) {
	const Node *scene = editor_data.get_edited_scene_root(p_idx);
	if (!scene) {
		return false;
	}

	if (EditorUndoRedoManager::get_singleton()->is_history_unsaved(editor_data.get_scene_history_id(p_idx))) {
		return true;
	}

	const String &scene_path = scene->get_scene_file_path();
	if (!scene_path.is_empty()) {
		// Check if scene has unsaved changes in built-in resources.
		for (int j = 0; j < editor_data.get_editor_plugin_count(); j++) {
			if (!editor_data.get_editor_plugin(j)->get_unsaved_status(scene_path).is_empty()) {
				return true;
			}
		}
	}
	return false;
}