/**************************************************************************/
/*  editor_node_node_modification.cpp                                     */
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

#include "core/core_string_names.h"
#include "core/error/error_macros.h"
#include "core/object/object.h"
#include "core/string/string_name.h"
#include "core/variant/variant.h"
#include "editor/inspector/editor_inspector.h"
#include "scene/main/node.h"
#include "scene/property_utils.h"

HashMap<StringName, Variant> EditorNode::get_modified_properties_for_node(Node *p_node, bool p_node_references_only) {
	HashMap<StringName, Variant> modified_property_map;

	List<PropertyInfo> pinfo;
	p_node->get_property_list(&pinfo);
	for (const PropertyInfo &E : pinfo) {
		if (E.usage & PROPERTY_USAGE_STORAGE) {
			bool node_reference = (E.type == Variant::OBJECT && E.hint == PROPERTY_HINT_NODE_TYPE);
			if (p_node_references_only && !node_reference) {
				continue;
			}
			bool is_valid_revert = false;
			Variant revert_value = EditorPropertyRevert::get_property_revert_value(p_node, E.name, &is_valid_revert);
			Variant current_value = p_node->get(E.name);
			if (is_valid_revert) {
				if (PropertyUtils::is_property_value_different(p_node, current_value, revert_value)) {
					// If this property is a direct node reference, save a NodePath instead to prevent corrupted references.
					if (node_reference) {
						Node *target_node = Object::cast_to<Node>(current_value);
						if (target_node) {
							modified_property_map[E.name] = p_node->get_path_to(target_node);
						}
					} else {
						modified_property_map[E.name] = current_value;
					}
				}
			}
		}
	}

	return modified_property_map;
}

HashMap<StringName, Variant> EditorNode::get_modified_properties_reference_to_nodes(Node *p_node, List<Node *> &p_nodes_referenced_by) {
	HashMap<StringName, Variant> modified_property_map;

	List<PropertyInfo> pinfo;
	p_node->get_property_list(&pinfo);
	for (const PropertyInfo &E : pinfo) {
		if (E.usage & PROPERTY_USAGE_STORAGE) {
			if (E.type != Variant::OBJECT || E.hint != PROPERTY_HINT_NODE_TYPE) {
				continue;
			}
			Variant current_value = p_node->get(E.name);
			Node *target_node = Object::cast_to<Node>(current_value);
			if (target_node && p_nodes_referenced_by.find(target_node)) {
				modified_property_map[E.name] = p_node->get_path_to(target_node);
			}
		}
	}

	return modified_property_map;
}

void EditorNode::update_node_from_node_modification_entry(Node *p_node, ModificationNodeEntry &p_node_modification) {
	if (p_node) {
		// First, attempt to restore the script property since it may affect the get_property_list method.
		Variant *script_property_table_entry = p_node_modification.property_table.getptr(CoreStringName(script));
		if (script_property_table_entry) {
			p_node->set_script(*script_property_table_entry);
		}

		// Get properties for this node.
		List<PropertyInfo> pinfo;
		p_node->get_property_list(&pinfo);

		// Get names of all valid property names.
		HashMap<StringName, bool> property_node_reference_table;
		for (const PropertyInfo &E : pinfo) {
			if (E.usage & PROPERTY_USAGE_STORAGE) {
				if (E.type == Variant::OBJECT && E.hint == PROPERTY_HINT_NODE_TYPE) {
					property_node_reference_table[E.name] = true;
				} else {
					property_node_reference_table[E.name] = false;
				}
			}
		}

		// Restore the modified properties for this node.
		for (const KeyValue<StringName, Variant> &E : p_node_modification.property_table) {
			bool *property_node_reference_table_entry = property_node_reference_table.getptr(E.key);
			if (property_node_reference_table_entry) {
				// If the property is a node reference, attempt to restore from the node path instead.
				bool is_node_reference = *property_node_reference_table_entry;
				if (is_node_reference) {
					if (E.value.get_type() == Variant::NODE_PATH) {
						p_node->set(E.key, p_node->get_node_or_null(E.value));
					}
				} else {
					p_node->set(E.key, E.value);
				}
			}
		}

		// Restore the connections to other nodes.
		for (const ConnectionWithNodePath &E : p_node_modification.connections_to) {
			Connection conn = E.connection;

			// Get the node the callable is targeting.
			Node *target_node = Object::cast_to<Node>(conn.callable.get_object());

			// If the callable object no longer exists or is marked for deletion,
			// attempt to reaccquire the closest match by using the node path
			// we saved earlier.
			if (!target_node || !target_node->is_queued_for_deletion()) {
				target_node = p_node->get_node_or_null(E.node_path);
			}

			if (target_node) {
				// Reconstruct the callable.
				Callable new_callable = Callable(target_node, conn.callable.get_method());

				if (!p_node->is_connected(conn.signal.get_name(), new_callable)) {
					ERR_FAIL_COND(p_node->connect(conn.signal.get_name(), new_callable, conn.flags) != OK);
				}
			}
		}

		// Restore the connections from other nodes.
		for (const Connection &E : p_node_modification.connections_from) {
			Connection conn = E;

			bool valid = p_node->has_method(conn.callable.get_method()) || Ref<Script>(p_node->get_script()).is_null() || Ref<Script>(p_node->get_script())->has_method(conn.callable.get_method());
			ERR_CONTINUE_MSG(!valid, vformat("Attempt to connect signal '%s.%s' to nonexistent method '%s.%s'.", conn.signal.get_object()->get_class(), conn.signal.get_name(), conn.callable.get_object()->get_class(), conn.callable.get_method()));

			// Get the object which the signal is connected from.
			Object *source_object = conn.signal.get_object();

			if (source_object) {
				ERR_FAIL_COND(source_object->connect(conn.signal.get_name(), Callable(p_node, conn.callable.get_method()), conn.flags) != OK);
			}
		}

		// Re-add the groups.
		for (const Node::GroupInfo &E : p_node_modification.groups) {
			p_node->add_to_group(E.name, E.persistent);
		}
	}
}

bool EditorNode::is_additional_node_in_scene(Node *p_edited_scene, Node *p_reimported_root, Node *p_node) {
	if (p_node == p_reimported_root) {
		return false;
	}

	bool node_part_of_subscene = p_node != p_edited_scene &&
			p_edited_scene->get_scene_inherited_state().is_valid() &&
			p_edited_scene->get_scene_inherited_state()->find_node_by_path(p_edited_scene->get_path_to(p_node)) >= 0 &&
			// It's important to process added nodes from the base scene in the inherited scene as
			// additional nodes to ensure they do not disappear on reload.
			// When p_reimported_root == p_edited_scene that means the edited scene
			// is the reimported scene, in that case the node is in the root base scene,
			// so it's not an addition, otherwise, the node would be added twice on reload.
			(p_node->get_owner() != p_edited_scene || p_reimported_root == p_edited_scene);

	if (node_part_of_subscene) {
		return false;
	}

	// Loop through the owners until either we reach the root node or nullptr
	Node *valid_node_owner = p_node->get_owner();
	while (valid_node_owner) {
		if (valid_node_owner == p_reimported_root) {
			break;
		}
		valid_node_owner = valid_node_owner->get_owner();
	}

	// When the owner is the imported scene and the owner is also the edited scene,
	// that means the node was added in the current edited scene.
	// We can be sure here because if the node that the node does not come from
	// the base scene because we checked just over with 'get_scene_inherited_state()->find_node_by_path'.
	if (valid_node_owner == p_reimported_root && p_reimported_root != p_edited_scene) {
		return false;
	}

	return true;
}

