/**************************************************************************/
/*  editor_node_init_dock.cpp                                            */
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
#include "scene/gui/box_container.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"

void EditorNode::_init_dock() {
	main_hsplit = memnew(DockSplitContainer);
	main_hsplit->set_name("DockHSplitMain");
	main_hsplit->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_vbox->add_child(main_hsplit);

	left_l_vsplit = memnew(DockSplitContainer);
	left_l_vsplit->set_name("DockVSplitLeftL");
	left_l_vsplit->set_vertical(true);
	main_hsplit->add_child(left_l_vsplit);

	TabContainer *dock_slot[DockConstants::DOCK_SLOT_MAX];
	dock_slot[DockConstants::DOCK_SLOT_LEFT_UL] = memnew(TabContainer);
	dock_slot[DockConstants::DOCK_SLOT_LEFT_UL]->set_name("DockSlotLeftUL");
	left_l_vsplit->add_child(dock_slot[DockConstants::DOCK_SLOT_LEFT_UL]);
	dock_slot[DockConstants::DOCK_SLOT_LEFT_BL] = memnew(TabContainer);
	dock_slot[DockConstants::DOCK_SLOT_LEFT_BL]->set_name("DockSlotLeftBL");
	left_l_vsplit->add_child(dock_slot[DockConstants::DOCK_SLOT_LEFT_BL]);

	left_r_vsplit = memnew(DockSplitContainer);
	left_r_vsplit->set_name("DockVSplitLeftR");
	left_r_vsplit->set_vertical(true);
	main_hsplit->add_child(left_r_vsplit);
	dock_slot[DockConstants::DOCK_SLOT_LEFT_UR] = memnew(TabContainer);
	dock_slot[DockConstants::DOCK_SLOT_LEFT_UR]->set_name("DockSlotLeftUR");
	left_r_vsplit->add_child(dock_slot[DockConstants::DOCK_SLOT_LEFT_UR]);
	dock_slot[DockConstants::DOCK_SLOT_LEFT_BR] = memnew(TabContainer);
	dock_slot[DockConstants::DOCK_SLOT_LEFT_BR]->set_name("DockSlotLeftBR");
	left_r_vsplit->add_child(dock_slot[DockConstants::DOCK_SLOT_LEFT_BR]);

	VBoxContainer *center_vb = memnew(VBoxContainer);
	center_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	main_hsplit->add_child(center_vb);

	center_split = memnew(DockSplitContainer);
	center_split->set_name("DockVSplitCenter");
	center_split->set_vertical(true);
	center_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	center_split->set_collapsed(true);
	center_vb->add_child(center_split);
	center_split->connect("drag_ended", callable_mp(this, &EditorNode::_bottom_panel_resized));

	right_l_vsplit = memnew(DockSplitContainer);
	right_l_vsplit->set_name("DockVSplitRightL");
	right_l_vsplit->set_vertical(true);
	main_hsplit->add_child(right_l_vsplit);
	dock_slot[DockConstants::DOCK_SLOT_RIGHT_UL] = memnew(TabContainer);
	dock_slot[DockConstants::DOCK_SLOT_RIGHT_UL]->set_name("DockSlotRightUL");
	right_l_vsplit->add_child(dock_slot[DockConstants::DOCK_SLOT_RIGHT_UL]);
	dock_slot[DockConstants::DOCK_SLOT_RIGHT_BL] = memnew(TabContainer);
	dock_slot[DockConstants::DOCK_SLOT_RIGHT_BL]->set_name("DockSlotRightBL");
	right_l_vsplit->add_child(dock_slot[DockConstants::DOCK_SLOT_RIGHT_BL]);

	right_r_vsplit = memnew(DockSplitContainer);
	right_r_vsplit->set_name("DockVSplitRightR");
	right_r_vsplit->set_vertical(true);
	main_hsplit->add_child(right_r_vsplit);
	dock_slot[DockConstants::DOCK_SLOT_RIGHT_UR] = memnew(TabContainer);
	dock_slot[DockConstants::DOCK_SLOT_RIGHT_UR]->set_name("DockSlotRightUR");
	right_r_vsplit->add_child(dock_slot[DockConstants::DOCK_SLOT_RIGHT_UR]);
	dock_slot[DockConstants::DOCK_SLOT_RIGHT_BR] = memnew(TabContainer);
	dock_slot[DockConstants::DOCK_SLOT_RIGHT_BR]->set_name("DockSlotRightBR");
	right_r_vsplit->add_child(dock_slot[DockConstants::DOCK_SLOT_RIGHT_BR]);

	editor_dock_manager = memnew(EditorDockManager);

	// Save the splits for easier access.
	editor_dock_manager->add_vsplit(left_l_vsplit);
	editor_dock_manager->add_vsplit(left_r_vsplit);
	editor_dock_manager->add_vsplit(right_l_vsplit);
	editor_dock_manager->add_vsplit(right_r_vsplit);

	editor_dock_manager->set_hsplit(main_hsplit);

	for (int i = 0; i < DockConstants::DOCK_SLOT_BOTTOM; i++) {
		editor_dock_manager->register_dock_slot((DockConstants::DockSlot)i, dock_slot[i], DockConstants::DOCK_LAYOUT_VERTICAL);
	}
}

