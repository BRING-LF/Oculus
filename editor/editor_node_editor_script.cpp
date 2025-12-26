/**************************************************************************/
/*  editor_node_editor_script.cpp                                        */
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

#include "core/object/class_db.h"
#include "core/object/script_language.h"
#include "editor/gui/editor_toaster.h"
#include "editor/script/editor_script.h"

void EditorNode::run_editor_script(const Ref<Script> &p_script) {
	Error err = p_script->reload(true); // Always hard reload the script before running.
	if (err != OK || !p_script->is_valid()) {
		EditorToaster::get_singleton()->popup_str(TTR("Cannot run the script because it contains errors, check the output log."), EditorToaster::SEVERITY_WARNING);
		return;
	}

	// Perform additional checks on the script to evaluate if it's runnable.

	bool is_runnable = true;
	if (!ClassDB::is_parent_class(p_script->get_instance_base_type(), "EditorScript")) {
		is_runnable = false;

		EditorToaster::get_singleton()->popup_str(TTR("Cannot run the script because it doesn't extend EditorScript."), EditorToaster::SEVERITY_WARNING);
	}
	if (!p_script->is_tool()) {
		is_runnable = false;

		if (p_script->get_class() == "GDScript") {
			EditorToaster::get_singleton()->popup_str(TTR("Cannot run the script because it's not a tool script (add the @tool annotation at the top)."), EditorToaster::SEVERITY_WARNING);
		} else if (p_script->get_class() == "CSharpScript") {
			EditorToaster::get_singleton()->popup_str(TTR("Cannot run the script because it's not a tool script (add the [Tool] attribute above the class definition)."), EditorToaster::SEVERITY_WARNING);
		} else {
			EditorToaster::get_singleton()->popup_str(TTR("Cannot run the script because it's not a tool script."), EditorToaster::SEVERITY_WARNING);
		}
	}
	if (!is_runnable) {
		return;
	}

	Ref<EditorScript> es = memnew(EditorScript);
	es->set_script(p_script);
	es->run();
}

