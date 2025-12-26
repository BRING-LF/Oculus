/**************************************************************************/
/*  editor_node_destructor.cpp                                            */
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

#include "core/extension/gdextension.h"
#include "editor/doc/editor_help.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/gui/progress_dialog.h"
#include "editor/inspector/editor_context_menu_plugin.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/import/3d/resource_importer_scene.h"
#include "editor/plugins/editor_plugin_list.h"
#include "editor/project_upgrade/project_upgrade_tool.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_theme_manager.h"
#include "editor/translations/editor_translation_parser.h"
#include "scene/gui/file_dialog.h"

#if defined(MODULE_GDSCRIPT_ENABLED) || defined(MODULE_MONO_ENABLED)
#include "editor/doc/editor_help_highlighter.h"
#endif

EditorNode::~EditorNode() {
	EditorInspector::cleanup_plugins();
	EditorTranslationParser::get_singleton()->clean_parsers();
	ResourceImporterScene::clean_up_importer_plugins();
	EditorContextMenuPluginManager::cleanup();

	remove_print_handler(&print_handler);
	EditorHelp::cleanup_doc();
#if defined(MODULE_GDSCRIPT_ENABLED) || defined(MODULE_MONO_ENABLED)
	EditorHelpHighlighter::free_singleton();
#endif
	memdelete(editor_selection);
	memdelete(editor_plugins_over);
	memdelete(editor_plugins_force_over);
	memdelete(editor_plugins_force_input_forwarding);
	memdelete(progress_hb);
	memdelete(project_upgrade_tool);
	memdelete(editor_dock_manager);

	EditorSettings::destroy();
	EditorThemeManager::finalize();

	GDExtensionEditorPlugins::editor_node_add_plugin = nullptr;
	GDExtensionEditorPlugins::editor_node_remove_plugin = nullptr;

	FileDialog::register_func = nullptr;
	FileDialog::unregister_func = nullptr;

	file_dialogs.clear();

	singleton = nullptr;
}

