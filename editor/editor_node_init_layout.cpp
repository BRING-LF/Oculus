/**************************************************************************/
/*  editor_node_init_layout.cpp                                           */
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

#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_log.h"
#include "editor/gui/editor_bottom_panel.h"
#include "editor/settings/editor_settings.h"
#include "scene/gui/split_container.h"

static const String EDITOR_NODE_CONFIG_SECTION = "EditorNode";

void EditorNode::_init_layout() {
	// Add some offsets to make LEFT_R and RIGHT_L docks wider than minsize.
	const int dock_hsize = 280;
	// By default there is only 3 visible, so set 2 split offsets for them.
	const int dock_hsize_scaled = dock_hsize * EDSCALE;
	main_hsplit->set_split_offsets({ dock_hsize_scaled, -dock_hsize_scaled });

	// Define corresponding default layout.

	const String docks_section = "docks";
	default_layout.instantiate();
	// Dock numbers are based on DockSlot enum value + 1.
	default_layout->set_value(docks_section, "dock_3", "Scene,Import");
	default_layout->set_value(docks_section, "dock_4", "FileSystem,History");
	default_layout->set_value(docks_section, "dock_5", "Inspector,Signals,Groups");

	int hsplits[] = { 0, dock_hsize, -dock_hsize, 0 };
	for (int i = 0; i < (int)std_size(hsplits); i++) {
		default_layout->set_value(docks_section, "dock_hsplit_" + itos(i + 1), hsplits[i]);
	}
	for (int i = 0; i < editor_dock_manager->get_vsplit_count(); i++) {
		default_layout->set_value(docks_section, "dock_split_" + itos(i + 1), 0);
	}

	{
		Dictionary offsets;
		offsets["Audio"] = -450;
		default_layout->set_value(EDITOR_NODE_CONFIG_SECTION, "bottom_panel_offsets", offsets);
	}

	_update_layouts_menu();

	// Bottom panels.

	bottom_panel = memnew(EditorBottomPanel);
	editor_dock_manager->register_dock_slot(DockConstants::DOCK_SLOT_BOTTOM, bottom_panel, DockConstants::DOCK_LAYOUT_HORIZONTAL);
	bottom_panel->set_theme_type_variation("BottomPanel");
	center_split->add_child(bottom_panel);
	center_split->set_dragger_visibility(SplitContainer::DRAGGER_HIDDEN);

	log = memnew(EditorLog);
	editor_dock_manager->add_dock(log);

	center_split->connect(SceneStringName(resized), callable_mp(this, &EditorNode::_vp_resized));
}

