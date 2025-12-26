/**************************************************************************/
/*  editor_node_init_servers.cpp                                           */
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

#include "core/input/input.h"
#include "core/object/script_language.h"
#include "core/os/os.h"
#include "servers/audio/audio_server.h"
#include "servers/display/display_server.h"
#include "servers/navigation_3d/navigation_server_3d.h"
#include "servers/rendering/rendering_server.h"
#include "servers/physics_2d/physics_server_2d.h"
#include "servers/physics_3d/physics_server_3d.h"
#include "scene/resources/portable_compressed_texture.h"

void EditorNode::_init_servers() {
	PortableCompressedTexture2D::set_keep_all_compressed_buffers(true);
	RenderingServer::get_singleton()->set_debug_generate_wireframes(true);

	AudioServer::get_singleton()->set_enable_tagging_used_audio_streams(true);

	// No navigation by default if in editor.
	if (NavigationServer3D::get_singleton()->get_debug_enabled()) {
		NavigationServer3D::get_singleton()->set_active(true);
	} else {
		NavigationServer3D::get_singleton()->set_active(false);
	}

	// No physics by default if in editor.
#ifndef PHYSICS_3D_DISABLED
	PhysicsServer3D::get_singleton()->set_active(false);
#endif // PHYSICS_3D_DISABLED
#ifndef PHYSICS_2D_DISABLED
	PhysicsServer2D::get_singleton()->set_active(false);
#endif // PHYSICS_2D_DISABLED

	// No scripting by default if in editor (except for tool).
	ScriptServer::set_scripting_enabled(false);

	if (!DisplayServer::get_singleton()->is_touchscreen_available()) {
		// Only if no touchscreen ui hint, disable emulation just in case.
		Input::get_singleton()->set_emulate_touch_from_mouse(false);
	}
	if (DisplayServer::get_singleton()->has_feature(DisplayServer::FEATURE_CUSTOM_CURSOR_SHAPE)) {
		DisplayServer::get_singleton()->cursor_set_custom_image(Ref<Resource>());
	}
}

