/**************************************************************************/
/*  editor_node_multi_window.cpp                                          */
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

#include "core/string/translation_server.h"
#include "editor/settings/editor_settings.h"
#include "scene/main/scene_tree.h"
#include "servers/display/display_server.h"

bool EditorNode::is_multi_window_enabled() const {
	return !SceneTree::get_singleton()->get_root()->is_embedding_subwindows() && !EDITOR_GET("interface/editor/single_window_mode") && EDITOR_GET("interface/multi_window/enable");
}

String EditorNode::get_multiwindow_support_tooltip_text() const {
	if (SceneTree::get_singleton()->get_root()->is_embedding_subwindows()) {
		if (DisplayServer::get_singleton()->has_feature(DisplayServer::FEATURE_SUBWINDOWS)) {
			return TTR("Multi-window support is not available because the `--single-window` command line argument was used to start the editor.");
		} else {
			return TTR("Multi-window support is not available because the current platform doesn't support multiple windows.");
		}
	} else if (EDITOR_GET("interface/editor/single_window_mode")) {
		return TTR("Multi-window support is not available because Interface > Editor > Single Window Mode is enabled in the editor settings.");
	}

	return TTR("Multi-window support is not available because Interface > Multi Window > Enable is disabled in the editor settings.");
}

