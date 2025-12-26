/**************************************************************************/
/*  editor_node_resource_conversion.cpp                                    */
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

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "editor/plugins/editor_resource_conversion_plugin.h"

void EditorNode::add_resource_conversion_plugin(const Ref<EditorResourceConversionPlugin> &p_plugin) {
	resource_conversion_plugins.push_back(p_plugin);
}

void EditorNode::remove_resource_conversion_plugin(const Ref<EditorResourceConversionPlugin> &p_plugin) {
	resource_conversion_plugins.erase(p_plugin);
}

Vector<Ref<EditorResourceConversionPlugin>> EditorNode::find_resource_conversion_plugin_for_resource(const Ref<Resource> &p_for_resource) {
	if (p_for_resource.is_null()) {
		return Vector<Ref<EditorResourceConversionPlugin>>();
	}

	Vector<Ref<EditorResourceConversionPlugin>> ret;
	for (Ref<EditorResourceConversionPlugin> resource_conversion_plugin : resource_conversion_plugins) {
		if (resource_conversion_plugin.is_valid() && resource_conversion_plugin->handles(p_for_resource)) {
			ret.push_back(resource_conversion_plugin);
		}
	}

	return ret;
}

Vector<Ref<EditorResourceConversionPlugin>> EditorNode::find_resource_conversion_plugin_for_type_name(const String &p_type) {
	Vector<Ref<EditorResourceConversionPlugin>> ret;

	if (ClassDB::class_exists(p_type) && ClassDB::can_instantiate(p_type)) {
		Ref<Resource> temp = Object::cast_to<Resource>(ClassDB::instantiate(p_type));
		if (temp.is_valid()) {
			for (Ref<EditorResourceConversionPlugin> resource_conversion_plugin : resource_conversion_plugins) {
				if (resource_conversion_plugin.is_valid() && resource_conversion_plugin->handles(temp)) {
					ret.push_back(resource_conversion_plugin);
				}
			}
		}
	}

	return ret;
}