/**************************************************************************/
/*  editor_node_project.cpp                                              */
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
#include "core/io/file_access.h"
#include "core/io/resource_uid.h"
#include "editor/file_system/editor_paths.h"

bool EditorNode::_is_project_data_missing() {
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	const String project_data_dir = EditorPaths::get_singleton()->get_project_data_dir();
	if (!da->dir_exists(project_data_dir)) {
		return true;
	}

	String project_data_gdignore_file_path = project_data_dir.path_join(".gdignore");
	if (!FileAccess::exists(project_data_gdignore_file_path)) {
		Ref<FileAccess> f = FileAccess::open(project_data_gdignore_file_path, FileAccess::WRITE);
		if (f.is_valid()) {
			f->store_line("");
		} else {
			ERR_PRINT("Failed to create file " + project_data_gdignore_file_path.quote() + ".");
		}
	}

	String uid_cache = ResourceUID::get_singleton()->get_cache_file();
	if (!da->file_exists(uid_cache)) {
		Error err = ResourceUID::get_singleton()->save_to_cache();
		if (err != OK) {
			ERR_PRINT("Failed to create file " + uid_cache.quote() + ".");
		}
	}

	const String dirs[] = {
		EditorPaths::get_singleton()->get_project_settings_dir(),
		ProjectSettings::get_singleton()->get_imported_files_path()
	};
	for (const String &dir : dirs) {
		if (!da->dir_exists(dir)) {
			return true;
		}
	}
	return false;
}

