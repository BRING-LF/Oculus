/**************************************************************************/
/*  editor_node_feature_profile.cpp                                       */
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

#include "core/config/engine.h"
#include "editor/asset_library/asset_library_editor_plugin.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/docks/groups_dock.h"
#include "editor/docks/history_dock.h"
#include "editor/docks/import_dock.h"
#include "editor/docks/signals_dock.h"
#include "editor/editor_main_screen.h"
#include "editor/settings/editor_feature_profile.h"

void EditorNode::_feature_profile_changed() {
	Ref<EditorFeatureProfile> profile = feature_profile_manager->get_current_profile();
	if (profile.is_valid()) {
		editor_dock_manager->set_dock_enabled(SignalsDock::get_singleton(), !profile->is_feature_disabled(EditorFeatureProfile::FEATURE_SIGNALS_DOCK));
		editor_dock_manager->set_dock_enabled(GroupsDock::get_singleton(), !profile->is_feature_disabled(EditorFeatureProfile::FEATURE_GROUPS_DOCK));
		// The Import dock is useless without the FileSystem dock. Ensure the configuration is valid.
		bool fs_dock_disabled = profile->is_feature_disabled(EditorFeatureProfile::FEATURE_FILESYSTEM_DOCK);
		editor_dock_manager->set_dock_enabled(FileSystemDock::get_singleton(), !fs_dock_disabled);
		editor_dock_manager->set_dock_enabled(ImportDock::get_singleton(), !fs_dock_disabled && !profile->is_feature_disabled(EditorFeatureProfile::FEATURE_IMPORT_DOCK));
		editor_dock_manager->set_dock_enabled(history_dock, !profile->is_feature_disabled(EditorFeatureProfile::FEATURE_HISTORY_DOCK));

		editor_main_screen->set_button_enabled(EditorMainScreen::EDITOR_3D, !profile->is_feature_disabled(EditorFeatureProfile::FEATURE_3D));
		editor_main_screen->set_button_enabled(EditorMainScreen::EDITOR_SCRIPT, !profile->is_feature_disabled(EditorFeatureProfile::FEATURE_SCRIPT));
		if (!Engine::get_singleton()->is_recovery_mode_hint()) {
			editor_main_screen->set_button_enabled(EditorMainScreen::EDITOR_GAME, !profile->is_feature_disabled(EditorFeatureProfile::FEATURE_GAME));
		}
		if (AssetLibraryEditorPlugin::is_available()) {
			editor_main_screen->set_button_enabled(EditorMainScreen::EDITOR_ASSETLIB, !profile->is_feature_disabled(EditorFeatureProfile::FEATURE_ASSET_LIB));
		}
	} else {
		editor_dock_manager->set_dock_enabled(ImportDock::get_singleton(), true);
		editor_dock_manager->set_dock_enabled(SignalsDock::get_singleton(), true);
		editor_dock_manager->set_dock_enabled(GroupsDock::get_singleton(), true);
		editor_dock_manager->set_dock_enabled(FileSystemDock::get_singleton(), true);
		editor_dock_manager->set_dock_enabled(history_dock, true);
		editor_main_screen->set_button_enabled(EditorMainScreen::EDITOR_3D, true);
		editor_main_screen->set_button_enabled(EditorMainScreen::EDITOR_SCRIPT, true);
		if (!Engine::get_singleton()->is_recovery_mode_hint()) {
			editor_main_screen->set_button_enabled(EditorMainScreen::EDITOR_GAME, true);
		}
		if (AssetLibraryEditorPlugin::is_available()) {
			editor_main_screen->set_button_enabled(EditorMainScreen::EDITOR_ASSETLIB, true);
		}
	}
}

