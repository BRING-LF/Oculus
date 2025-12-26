/**************************************************************************/
/*  editor_node_resource_management.cpp                                    */
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
#include "core/io/file_access.h"
#include "core/io/resource.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "editor/docks/inspector_dock.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_toaster.h"
#include "editor/settings/editor_settings.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/3d/world_3d.h"
#include "scene/resources/environment.h"

constexpr int LARGE_RESOURCE_WARNING_SIZE_THRESHOLD = 512'000; // 500 KB

void EditorNode::edit_resource(const Ref<Resource> &p_resource) {
	InspectorDock::get_singleton()->edit_resource(p_resource);
}

void EditorNode::save_resource_in_path(const Ref<Resource> &p_resource, const String &p_path) {
	editor_data.apply_changes_in_editors();

	if (saving_resources_in_path.has(p_resource)) {
		return;
	}
	saving_resources_in_path.insert(p_resource);

	int flg = 0;
	if (EDITOR_GET("filesystem/on_save/compress_binary_resources")) {
		flg |= ResourceSaver::FLAG_COMPRESS;
	}

	String path = ProjectSettings::get_singleton()->localize_path(p_path);
	Error err = ResourceSaver::save(p_resource, path, flg | ResourceSaver::FLAG_REPLACE_SUBRESOURCE_PATHS);

	if (err != OK) {
		if (ResourceLoader::is_imported(p_resource->get_path())) {
			show_accept(TTR("Imported resources can't be saved."), TTR("OK"));
		} else {
			show_accept(TTR("Error saving resource!"), TTR("OK"));
		}

		saving_resources_in_path.erase(p_resource);
		return;
	}

	Ref<Resource> prev_resource = ResourceCache::get_ref(p_path);
	if (prev_resource.is_null() || prev_resource != p_resource) {
		p_resource->set_path(path, true);
	}
	if (prev_resource.is_valid() && prev_resource != p_resource) {
		replace_resources_in_scenes({ prev_resource }, { p_resource });
	}
	saving_resources_in_path.erase(p_resource);

	_resource_saved(p_resource, path);
	clear_node_reference(p_resource); // // Check if Resource is saved to disk to potentially remove it from resource_count
	emit_signal(SNAME("resource_saved"), p_resource);
	editor_data.notify_resource_saved(p_resource);

	if (EDITOR_GET("filesystem/on_save/warn_on_saving_large_text_resources")) {
		if (p_path.ends_with(".tres")) {
			const int64_t file_size = FileAccess::get_size(p_path);
			if (file_size >= LARGE_RESOURCE_WARNING_SIZE_THRESHOLD) {
				// File is larger than 500 KiB, likely because it contains binary data serialized as Base64.
				// This is slow to save and load, so warn the user.
				EditorToaster::get_singleton()->popup_str(
						vformat(TTR("The text-based resource at path \"%s\" is large on disk (%s), likely because it has embedded binary data.\nThis slows down resource saving and loading.\nConsider saving its binary subresource(s) to a binary `.res` file or saving the resource as a binary `.res` file.\nThis warning can be disabled in the Editor Settings (FileSystem > On Save > Warn on Saving Large Text Resources)."), p_path, String::humanize_size(file_size)), EditorToaster::SEVERITY_WARNING);
			}
		}
	}
}

void EditorNode::save_resource(const Ref<Resource> &p_resource) {
	// If built-in resource, save the scene instead.
	if (p_resource->is_built_in()) {
		const String scene_path = p_resource->get_path().get_slice("::", 0);
		if (!scene_path.is_empty()) {
			if (ResourceLoader::exists(scene_path) && ResourceLoader::get_resource_type(scene_path) == "PackedScene") {
				save_scene_if_open(scene_path);
			} else {
				// Not a packed scene, so save it as regular resource.
				Ref<Resource> parent_resource = ResourceCache::get_ref(scene_path);
				ERR_FAIL_COND_MSG(parent_resource.is_null(), "Parent resource not loaded, can't save.");
				save_resource(parent_resource);
			}
			return;
		}
	}

	// If the resource has been imported, ask the user to use a different path in order to save it.
	String path = p_resource->get_path();
	if (path.is_resource_file() && !FileAccess::exists(path + ".import")) {
		save_resource_in_path(p_resource, p_resource->get_path());
	} else {
		save_resource_as(p_resource);
	}
}

void EditorNode::save_resource_as(const Ref<Resource> &p_resource, const String &p_at_path) {
	String resource_path = p_resource->get_path();
	bool is_resource = resource_path.is_resource_file();

	{
		// Early exit checks.

		if (is_resource) {
			if (FileAccess::exists(resource_path + ".import")) {
				show_warning(TTR("This resource can't be saved because it was imported from another file. Make it unique first."));
				return;
			}
		} else {
			int separator_pos = resource_path.find("::");
			if (separator_pos != -1) {
				String base = resource_path.substr(0, separator_pos);
				String base_resource_type = ResourceLoader::get_resource_type(base);
				if (base_resource_type == "PackedScene" && (!get_edited_scene() || get_edited_scene()->get_scene_file_path() != base)) {
					show_warning(TTR("This resource can't be saved because it does not belong to the edited scene. Make it unique first."));
					return;
				}
			}
		}
	}

	file->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
	saving_resource = p_resource;

	current_menu_option = RESOURCE_SAVE_AS;
	List<String> extensions;
	ResourceSaver::get_recognized_extensions(p_resource, &extensions);
	file->clear_filters();

	List<String> preferred;
	for (const String &E : extensions) {
		if (p_resource->is_class("Script") && (E == "tres" || E == "res")) {
			// This serves no purpose and confused people.
			continue;
		}
		file->add_filter("*." + E, E.to_upper());
		preferred.push_back(E);
	}
	// Lowest provided extension priority.
	List<String>::Element *res_element = preferred.find("res");
	if (res_element) {
		preferred.move_to_back(res_element);
	}

	String resource_name_snake_case = p_resource->get_class().to_snake_case();
	String new_resource_name_snake_case;
	if (!preferred.is_empty()) {
		new_resource_name_snake_case = "new_" + resource_name_snake_case + "." + preferred.front()->get().to_lower();
	}

	if (!p_at_path.is_empty()) {
		file->set_current_dir(p_at_path);
		if (is_resource) {
			file->set_current_file(resource_path.get_file());
		} else {
			file->set_current_file(new_resource_name_snake_case);
		}
	} else if (!p_resource->get_path().get_base_dir().is_empty()) {
		if (is_resource) {
			if (!extensions.is_empty()) {
				const String ext = resource_path.get_extension().to_lower();
				if (extensions.find(ext) == nullptr) {
					file->set_current_path(resource_path.replacen("." + ext, "." + extensions.front()->get()));
				}
			}
		} else {
			file->set_current_file(new_resource_name_snake_case);
		}
	} else if (!preferred.is_empty()) {
		file->set_current_file(new_resource_name_snake_case);
		file->set_current_path(new_resource_name_snake_case);
	}
	file->set_title(TTR("Save Resource As..."));
	file->popup_file_dialog();
}

void EditorNode::replace_resources_in_object(Object *p_object, const Vector<Ref<Resource>> &p_source_resources, const Vector<Ref<Resource>> &p_target_resource) {
	List<PropertyInfo> pi;
	p_object->get_property_list(&pi);

	for (const PropertyInfo &E : pi) {
		if (!(E.usage & PROPERTY_USAGE_STORAGE)) {
			continue;
		}

		switch (E.type) {
			case Variant::OBJECT: {
				if (E.hint == PROPERTY_HINT_RESOURCE_TYPE) {
					const Variant &v = p_object->get(E.name);
					Ref<Resource> res = v;

					if (res.is_valid()) {
						int res_idx = p_source_resources.find(res);
						if (res_idx != -1) {
							p_object->set(E.name, p_target_resource.get(res_idx));
						} else {
							replace_resources_in_object(v, p_source_resources, p_target_resource);
						}
					}
				}
			} break;
			case Variant::ARRAY: {
				Array varray = p_object->get(E.name);
				int len = varray.size();
				bool array_requires_updating = false;
				for (int i = 0; i < len; i++) {
					const Variant &v = varray.get(i);
					Ref<Resource> res = v;

					if (res.is_valid()) {
						int res_idx = p_source_resources.find(res);
						if (res_idx != -1) {
							varray.set(i, p_target_resource.get(res_idx));
							array_requires_updating = true;
						} else {
							replace_resources_in_object(v, p_source_resources, p_target_resource);
						}
					}
				}
				if (array_requires_updating) {
					p_object->set(E.name, varray);
				}
			} break;
			case Variant::DICTIONARY: {
				Dictionary d = p_object->get(E.name);
				bool dictionary_requires_updating = false;
				for (const Variant &F : d.get_key_list()) {
					Variant v = d[F];
					Ref<Resource> res = v;

					if (res.is_valid()) {
						int res_idx = p_source_resources.find(res);
						if (res_idx != -1) {
							d[F] = p_target_resource.get(res_idx);
							dictionary_requires_updating = true;
						} else {
							replace_resources_in_object(v, p_source_resources, p_target_resource);
						}
					}
				}
				if (dictionary_requires_updating) {
					p_object->set(E.name, d);
				}
			} break;
			default: {
			}
		}
	}

	Node *n = Object::cast_to<Node>(p_object);
	if (n) {
		for (int i = 0; i < n->get_child_count(); i++) {
			replace_resources_in_object(n->get_child(i), p_source_resources, p_target_resource);
		}
	}
}

void EditorNode::replace_resources_in_scenes(const Vector<Ref<Resource>> &p_source_resources, const Vector<Ref<Resource>> &p_target_resource) {
	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		Node *edited_scene_root = editor_data.get_edited_scene_root(i);
		if (edited_scene_root) {
			replace_resources_in_object(edited_scene_root, p_source_resources, p_target_resource);
		}
	}
}

void EditorNode::edit_foreign_resource(Ref<Resource> p_resource) {
	load_scene(p_resource->get_path().get_slice("::", 0));
	callable_mp(InspectorDock::get_singleton(), &InspectorDock::edit_resource).call_deferred(p_resource);
}

bool EditorNode::is_resource_read_only(Ref<Resource> p_resource, bool p_foreign_resources_are_writable) {
	ERR_FAIL_COND_V(p_resource.is_null(), false);

	String path = p_resource->get_path();
	if (!path.is_resource_file()) {
		// If the resource name contains '::', that means it is a subresource embedded in another resource.
		int srpos = path.find("::");
		if (srpos != -1) {
			String base = path.substr(0, srpos);
			// If the base resource is a packed scene, we treat it as read-only if it is not the currently edited scene.
			if (ResourceLoader::get_resource_type(base) == "PackedScene") {
				if (!get_tree()->get_edited_scene_root() || get_tree()->get_edited_scene_root()->get_scene_file_path() != base) {
					// If we have not flagged foreign resources as writable or the base scene the resource is
					// part was imported, it can be considered read-only.
					if (!p_foreign_resources_are_writable || FileAccess::exists(base + ".import")) {
						return true;
					}
				}
			} else {
				// If a corresponding .import file exists for the base file, we assume it to be imported and should therefore treated as read-only.
				if (FileAccess::exists(base + ".import")) {
					return true;
				}
			}
		}
	} else if (FileAccess::exists(path + ".import")) {
		// The resource is not a subresource, but if it has an .import file, it's imported so treat it as read only.
		return true;
	}

	return false;
}

void EditorNode::save_default_environment() {
	Ref<Environment> fallback = get_tree()->get_root()->get_world_3d()->get_fallback_environment();

	if (fallback.is_valid() && fallback->get_path().is_resource_file()) {
		HashMap<Ref<Resource>, bool> processed;
		_find_and_save_edited_subresources(fallback.ptr(), processed, 0);
		save_resource_in_path(fallback, fallback->get_path());
	}
}