/**************************************************************************/
/*  editor_node_init_inspector.cpp                                       */
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

#include "editor/inspector/editor_inspector.h"
#include "editor/inspector/editor_properties.h"
#include "editor/scene/3d/root_motion_editor_plugin.h"
#include "editor/scene/particle_process_material_editor_plugin.h"
#include "editor/shader/visual_shader_editor_plugin.h"

void EditorNode::_init_inspector_plugins() {
	Ref<EditorInspectorDefaultPlugin> eidp;
	eidp.instantiate();
	EditorInspector::add_inspector_plugin(eidp);

	Ref<EditorInspectorRootMotionPlugin> rmp;
	rmp.instantiate();
	EditorInspector::add_inspector_plugin(rmp);

	Ref<EditorInspectorVisualShaderModePlugin> smp;
	smp.instantiate();
	EditorInspector::add_inspector_plugin(smp);

	Ref<EditorInspectorParticleProcessMaterialPlugin> ppm;
	ppm.instantiate();
	EditorInspector::add_inspector_plugin(ppm);
}

