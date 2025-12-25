/**************************************************************************/
/*  editor_node_resource_counting.cpp                                     */
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

#include "core/object/object.h"
#include "core/variant/variant.h"
#include "scene/main/node.h"
#include "core/io/resource.h"

bool EditorNode::is_resource_internal_to_scene(Ref<Resource> p_resource) {
	bool inside_scene = !get_edited_scene() || get_edited_scene()->get_scene_file_path() == p_resource->get_path().get_slice("::", 0);
	return inside_scene || p_resource->get_path().is_empty();
}

void EditorNode::gather_resources(const Variant &p_variant, List<Ref<Resource>> &r_list, bool p_subresources, bool p_allow_external) {
	Variant::Type type = p_variant.get_type();
	if (type == Variant::OBJECT && p_variant.get_validated_object() == nullptr) {
		return;
	}

	if (type != Variant::OBJECT && type != Variant::ARRAY && type != Variant::DICTIONARY) {
		return;
	}

	if (type == Variant::ARRAY) {
		Array arr = p_variant;
		for (const Variant &v : arr) {
			Ref<Resource> res = v;
			if (res.is_valid()) {
				if (p_allow_external) {
					r_list.push_back(res);
				} else if (is_resource_internal_to_scene(res)) {
					r_list.push_back(res);
				}
			}
			if (Object::cast_to<Node>(v) == nullptr) {
				gather_resources(v, r_list, p_subresources, p_allow_external);
			}
		}
		return;
	}

	if (type == Variant::DICTIONARY) {
		Dictionary dict = p_variant;
		for (const KeyValue<Variant, Variant> &kv : dict) {
			Ref<Resource> res_key = kv.key;
			Ref<Resource> res_value = kv.value;
			if (res_key.is_valid()) {
				if (p_allow_external) {
					r_list.push_back(res_key);
				} else if (is_resource_internal_to_scene(res_key)) {
					r_list.push_back(res_key);
				}
			}
			if (res_value.is_valid()) {
				if (p_allow_external) {
					r_list.push_back(res_value);
				} else if (is_resource_internal_to_scene(res_value)) {
					r_list.push_back(res_value);
				}
			}
			if (Object::cast_to<Node>(kv.key) == nullptr) {
				gather_resources(kv.key, r_list, p_subresources, p_allow_external);
			}
			if (Object::cast_to<Node>(kv.value) == nullptr) {
				gather_resources(kv.value, r_list, p_subresources, p_allow_external);
			}
		}
		return;
	}

	List<PropertyInfo> pinfo;
	p_variant.get_property_list(&pinfo);

	for (const PropertyInfo &E : pinfo) {
		if (!(E.usage & PROPERTY_USAGE_EDITOR) || E.name == "script") {
			continue;
		}

		Variant property_value = p_variant.get(E.name);
		Variant::Type property_type = property_value.get_type();
		if (property_type == Variant::ARRAY || property_type == Variant::DICTIONARY) {
			gather_resources(property_value, r_list, p_subresources, p_allow_external);
			continue;
		}
		Ref<Resource> res = property_value;
		if (res.is_null()) {
			continue;
		}
		if (!p_allow_external) {
			if (!res->is_built_in() || res->get_path().get_slice("::", 0) != get_edited_scene()->get_scene_file_path()) {
				if (!res->get_path().is_empty()) {
					continue;
				}
			}
		}
		r_list.push_back(res);
		if (p_subresources) {
			gather_resources(res, r_list, p_subresources, p_allow_external);
		}
	}
}

void EditorNode::update_resource_count(Node *p_node, bool p_remove) {
	if (!get_edited_scene()) {
		return;
	}

	List<Ref<Resource>> res_list;
	gather_resources(p_node, res_list, true);

	for (Ref<Resource> &R : res_list) {
		List<Node *>::Element *E = resource_count[R].find(p_node);
		if (E) {
			if (p_remove) {
				resource_count[R].erase(E);
			}
		} else {
			resource_count[R].push_back(p_node);
		}
	}

	emit_signal(SNAME("resource_counter_changed"));
}

int EditorNode::get_resource_count(Ref<Resource> p_res) {
	List<Node *> *L = resource_count.getptr(p_res);
	return L ? L->size() : 0;
}

List<Node *> EditorNode::get_resource_node_list(Ref<Resource> p_res) {
	List<Node *> *L = resource_count.getptr(p_res);
	return L == nullptr ? List<Node *>() : *L;
}

void EditorNode::update_node_reference(const Variant &p_value, Node *p_node, bool p_remove) {
	List<Ref<Resource>> list;
	Ref<Resource> res = p_value;
	gather_resources(p_value, list, true); //Gather all Resources and their SubResources to remove p_node from their lists.

	if (res.is_valid() && is_resource_internal_to_scene(res)) {
		// Avoid external Resources from being added in.
		list.push_back(res);
	}

	for (Ref<Resource> &R : list) {
		if (!p_remove) {
			resource_count[R].push_back(p_node);
		} else {
			List<Node *>::Element *E = resource_count[R].find(p_node);
			if (E) {
				resource_count[R].erase(E);
			}
		}
	}
	emit_signal(SNAME("resource_counter_changed"));
}

void EditorNode::clear_node_reference(Ref<Resource> p_res) {
	if (is_resource_internal_to_scene(p_res)) {
		return;
	}
	List<Node *> *node_list = resource_count.getptr(p_res);
	if (node_list != nullptr) {
		node_list->clear();
	}
}

