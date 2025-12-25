/**************************************************************************/
/*  editor_node_icons.cpp                                                 */
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

#include "core/io/resource_loader.h"
#include "core/object/class_db.h"
#include "core/object/script_language.h"
#include "editor/debugger/editor_debugger_inspector.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/inspector/multi_node_edit.h"
#include "scene/gui/file_dialog.h"
#include "scene/property_utils.h"
#include "scene/resources/image_texture.h"

Ref<Script> EditorNode::get_object_custom_type_base(const Object *p_object) const {
	ERR_FAIL_NULL_V(p_object, nullptr);

	const Node *node = Object::cast_to<const Node>(p_object);
	if (node && node->has_meta(SceneStringName(_custom_type_script))) {
		return PropertyUtils::get_custom_type_script(node);
	}

	Ref<Script> scr = p_object->get_script();

	if (scr.is_valid()) {
		// Uncommenting would break things! Consider adding a parameter if you need it.
		// StringName name = EditorNode::get_editor_data().script_class_get_name(base_script->get_path());
		// if (name != StringName()) {
		// 	return name;
		// }

		// TODO: Should probably be deprecated in 4.x
		StringName base = scr->get_instance_base_type();
		if (base != StringName() && EditorNode::get_editor_data().get_custom_types().has(base)) {
			const Vector<EditorData::CustomType> &types = EditorNode::get_editor_data().get_custom_types()[base];

			Ref<Script> base_scr = scr;
			while (base_scr.is_valid()) {
				for (int i = 0; i < types.size(); ++i) {
					if (types[i].script == base_scr) {
						return types[i].script;
					}
				}
				base_scr = base_scr->get_base_script();
			}
		}
	}

	return nullptr;
}

StringName EditorNode::get_object_custom_type_name(const Object *p_object) const {
	ERR_FAIL_NULL_V(p_object, StringName());

	Ref<Script> scr = p_object->get_script();
	if (scr.is_null() && Object::cast_to<Script>(p_object)) {
		scr = p_object;
	}

	if (scr.is_valid()) {
		Ref<Script> base_scr = scr;
		while (base_scr.is_valid()) {
			StringName name = EditorNode::get_editor_data().script_class_get_name(base_scr->get_path());
			if (name != StringName()) {
				return name;
			}

			// TODO: Should probably be deprecated in 4.x.
			StringName base = base_scr->get_instance_base_type();
			if (base != StringName() && EditorNode::get_editor_data().get_custom_types().has(base)) {
				const Vector<EditorData::CustomType> &types = EditorNode::get_editor_data().get_custom_types()[base];
				for (int i = 0; i < types.size(); ++i) {
					if (types[i].script == base_scr) {
						return types[i].name;
					}
				}
			}
			base_scr = base_scr->get_base_script();
		}
	}

	return StringName();
}

Ref<Texture2D> EditorNode::_get_class_or_script_icon(const String &p_class, const String &p_script_path, const String &p_fallback, bool p_fallback_script_to_theme, bool p_skip_fallback_virtual) {
	ERR_FAIL_COND_V_MSG(p_class.is_empty(), nullptr, "Class name cannot be empty.");
	EditorData &ed = EditorNode::get_editor_data();

	// Check for a script icon first.
	if (!p_script_path.is_empty()) {
		Ref<Texture2D> script_icon = ed.get_script_icon(p_script_path);
		if (script_icon.is_valid()) {
			return script_icon;
		}

		if (p_fallback_script_to_theme) {
			// Look for the native base type in the editor theme. This is relevant for
			// scripts extending other scripts and for built-in classes.
			String base_type;
			if (ScriptServer::is_global_class(p_class)) {
				base_type = ScriptServer::get_global_class_native_base(p_class);
			} else {
				Ref<Script> scr = ResourceLoader::load(p_script_path, "Script");
				if (scr.is_valid()) {
					base_type = scr->get_instance_base_type();
				}
			}
			if (theme.is_valid()) {
				bool instantiable = false;

				// If the class doesn't exist or isn't global, then it's not instantiable
				if (ClassDB::class_exists(p_class) || ScriptServer::is_global_class(p_class)) {
					instantiable = !ClassDB::is_virtual(p_class) && ClassDB::can_instantiate(p_class);
				}

				return _get_class_or_script_icon(base_type, "", "", false, p_skip_fallback_virtual || instantiable);
			}
		}
	}

	// Script was not valid or didn't yield any useful values, try the class name
	// directly.

	// Check if the class name is an extension-defined type.
	Ref<Texture2D> ext_icon = ed.extension_class_get_icon(p_class);
	if (ext_icon.is_valid()) {
		return ext_icon;
	}

	// Check if the class name is a custom type.
	// TODO: Should probably be deprecated in 4.x
	const EditorData::CustomType *ctype = ed.get_custom_type_by_name(p_class);
	if (ctype && ctype->icon.is_valid()) {
		return ctype->icon;
	}

	// Look up the class name or the fallback name in the editor theme.
	// This is only relevant for built-in classes.
	if (theme.is_valid()) {
		if (theme->has_icon(p_class, EditorStringName(EditorIcons))) {
			return theme->get_icon(p_class, EditorStringName(EditorIcons));
		}

		if (!p_fallback.is_empty() && theme->has_icon(p_fallback, EditorStringName(EditorIcons))) {
			return theme->get_icon(p_fallback, EditorStringName(EditorIcons));
		}

		// If the fallback is empty or wasn't found, use the default fallback.
		if (ClassDB::class_exists(p_class)) {
			if (!p_skip_fallback_virtual) {
				bool instantiable = !ClassDB::is_virtual(p_class) && ClassDB::can_instantiate(p_class);
				if (!instantiable) {
					if (ClassDB::is_parent_class(p_class, SNAME("Node"))) {
						return theme->get_icon("NodeDisabled", EditorStringName(EditorIcons));
					} else {
						return theme->get_icon("ObjectDisabled", EditorStringName(EditorIcons));
					}
				}
			}
			StringName parent = ClassDB::get_parent_class_nocheck(p_class);
			if (parent) {
				// Skip virtual class if `p_skip_fallback_virtual` is true or `p_class` is instantiable.
				return _get_class_or_script_icon(parent, "", "", false, true);
			}
		}
	}

	return nullptr;
}

Ref<Texture2D> EditorNode::get_object_icon(const Object *p_object, const String &p_fallback) {
	ERR_FAIL_NULL_V_MSG(p_object, nullptr, "Object cannot be null.");

	Ref<Script> scr = p_object->get_script();

	const EditorDebuggerRemoteObjects *robjs = Object::cast_to<EditorDebuggerRemoteObjects>(p_object);
	if (robjs) {
		String class_name;
		if (scr.is_valid()) {
			class_name = scr->get_global_name();

			if (class_name.is_empty()) {
				// If there is no class_name in this script we just take the script path.
				class_name = scr->get_path();
			}
		}

		if (class_name.is_empty()) {
			return get_class_icon(robjs->type_name, p_fallback);
		}

		return get_class_icon(class_name, p_fallback);
	}

	if (scr.is_null() && p_object->is_class("Script")) {
		scr = p_object;
	}

	if (Object::cast_to<MultiNodeEdit>(p_object)) {
		return get_class_icon(Object::cast_to<MultiNodeEdit>(p_object)->get_edited_class_name(), p_fallback);
	} else {
		return _get_class_or_script_icon(p_object->get_class(), scr.is_valid() ? scr->get_path() : String(), p_fallback);
	}
}

Ref<Texture2D> EditorNode::get_class_icon(const String &p_class, const String &p_fallback) {
	ERR_FAIL_COND_V_MSG(p_class.is_empty(), nullptr, "Class name cannot be empty.");
	const Pair<String, String> key(p_class, p_fallback);

	// Take from the local cache, if available.
	{
		Ref<Texture2D> *icon = class_icon_cache.getptr(key);
		if (icon) {
			return *icon;
		}
	}

	String script_path;
	if (ScriptServer::is_global_class(p_class)) {
		script_path = ScriptServer::get_global_class_path(p_class);
	} else if (!p_class.get_extension().is_empty() && ResourceLoader::exists(p_class)) { // If the script is not a class_name we check if the script resource exists.
		script_path = p_class;
	}

	Ref<Texture2D> icon = _get_class_or_script_icon(p_class, script_path, p_fallback, true);
	class_icon_cache[key] = icon;
	return icon;
}

bool EditorNode::is_object_of_custom_type(const Object *p_object, const StringName &p_class) {
	ERR_FAIL_NULL_V(p_object, false);

	Ref<Script> scr = p_object->get_script();
	if (scr.is_null() && Object::cast_to<Script>(p_object)) {
		scr = p_object;
	}

	if (scr.is_valid()) {
		Ref<Script> base_script = scr;
		while (base_script.is_valid()) {
			StringName name = EditorNode::get_editor_data().script_class_get_name(base_script->get_path());
			if (name == p_class) {
				return true;
			}
			base_script = base_script->get_base_script();
		}
	}
	return false;
}

Ref<Texture2D> EditorNode::_file_dialog_get_icon(const String &p_path) {
	EditorFileSystemDirectory *efsd = EditorFileSystem::get_singleton()->get_filesystem_path(p_path.get_base_dir());
	if (efsd) {
		String file = p_path.get_file();
		for (int i = 0; i < efsd->get_file_count(); i++) {
			if (efsd->get_file(i) == file) {
				String type = efsd->get_file_type(i);

				if (singleton->icon_type_cache.has(type)) {
					return singleton->icon_type_cache[type];
				} else {
					return singleton->icon_type_cache["Object"];
				}
			}
		}
	}

	return singleton->icon_type_cache["Object"];
}

Ref<Texture2D> EditorNode::_file_dialog_get_thumbnail(const String &p_path) {
	Ref<ImageTexture> texture = singleton->default_thumbnail->duplicate();
	EditorResourcePreview::get_singleton()->queue_resource_preview(p_path, callable_mp_static(EditorNode::_file_dialog_thumbnail_callback).bind(texture));
	return texture;
}

void EditorNode::_file_dialog_thumbnail_callback(const String &p_path, const Ref<Texture2D> &p_preview, const Ref<Texture2D> &p_small_preview, Ref<ImageTexture> p_texture) {
	ERR_FAIL_COND(p_texture.is_null());
	if (p_preview.is_valid()) {
		p_texture->set_image(p_preview->get_image());
	}
}

void EditorNode::_build_icon_type_cache() {
	List<StringName> tl;
	theme->get_icon_list(EditorStringName(EditorIcons), &tl);
	for (const StringName &E : tl) {
		if (!ClassDB::class_exists(E)) {
			continue;
		}
		icon_type_cache[E] = theme->get_icon(E, EditorStringName(EditorIcons));
	}
}

void EditorNode::_file_dialog_register(FileDialog *p_dialog) {
	singleton->file_dialogs.insert(p_dialog);
}

void EditorNode::_file_dialog_unregister(FileDialog *p_dialog) {
	singleton->file_dialogs.erase(p_dialog);
}

