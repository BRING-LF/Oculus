/**************************************************************************/
/*  editor_node_plugin_management.cpp                                     */
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

#include "core/config/project_settings.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/resource_loader.h"
#include "core/object/class_db.h"
#include "editor/editor_main_screen.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/plugins/editor_plugin.h"
#include "editor/plugins/editor_plugin_list.h"
#include "editor/settings/project_settings_editor.h"

void EditorNode::init_plugins() {
	_initializing_plugins = true;
	Vector<String> addons;
	if (ProjectSettings::get_singleton()->has_setting("editor_plugins/enabled")) {
		addons = GLOBAL_GET("editor_plugins/enabled");
	}

	for (const String &addon : addons) {
		set_addon_plugin_enabled(addon, true);
	}
	_initializing_plugins = false;

	if (!pending_addons.is_empty()) {
		EditorFileSystem::get_singleton()->connect("script_classes_updated", callable_mp(this, &EditorNode::_enable_pending_addons), CONNECT_ONE_SHOT);
	}
}

void EditorNode::_on_plugin_ready(Object *p_script, const String &p_activate_name) {
	Ref<Script> scr = Object::cast_to<Script>(p_script);
	if (scr.is_null()) {
		return;
	}
	project_settings_editor->update_plugins();
	project_settings_editor->hide();
	push_item(scr.operator->());
	if (p_activate_name.length()) {
		set_addon_plugin_enabled(p_activate_name, true);
	}
}

void EditorNode::_remove_plugin_from_enabled(const String &p_name) {
	ProjectSettings *ps = ProjectSettings::get_singleton();
	PackedStringArray enabled_plugins = ps->get("editor_plugins/enabled");
	for (int i = 0; i < enabled_plugins.size(); ++i) {
		if (enabled_plugins.get(i) == p_name) {
			enabled_plugins.remove_at(i);
			break;
		}
	}
	ps->set("editor_plugins/enabled", enabled_plugins);
}

void EditorNode::_plugin_over_edit(EditorPlugin *p_plugin, Object *p_object) {
	if (p_object) {
		editor_plugins_over->add_plugin(p_plugin);
		p_plugin->edit(p_object);
		p_plugin->make_visible(true);
	} else {
		editor_plugins_over->remove_plugin(p_plugin);
		p_plugin->edit(nullptr);
		p_plugin->make_visible(false);
	}
}

void EditorNode::_plugin_over_self_own(EditorPlugin *p_plugin) {
	active_plugins[p_plugin->get_instance_id()].insert(p_plugin);
}

void EditorNode::add_editor_plugin(EditorPlugin *p_editor, bool p_config_changed) {
	if (p_editor->has_main_screen()) {
		singleton->editor_main_screen->add_main_plugin(p_editor);
	}
	singleton->editor_data.add_editor_plugin(p_editor);
	singleton->add_child(p_editor);
	if (p_config_changed) {
		p_editor->enable_plugin();
	}
}

void EditorNode::remove_editor_plugin(EditorPlugin *p_editor, bool p_config_changed) {
	if (p_editor->has_main_screen()) {
		singleton->editor_main_screen->remove_main_plugin(p_editor);
	}
	p_editor->make_visible(false);
	p_editor->clear();
	if (p_config_changed) {
		p_editor->disable_plugin();
	}
	singleton->editor_plugins_over->remove_plugin(p_editor);
	singleton->editor_plugins_force_over->remove_plugin(p_editor);
	singleton->editor_plugins_force_input_forwarding->remove_plugin(p_editor);
	singleton->remove_child(p_editor);
	singleton->editor_data.remove_editor_plugin(p_editor);

	for (KeyValue<ObjectID, HashSet<EditorPlugin *>> &kv : singleton->active_plugins) {
		kv.value.erase(p_editor);
	}
}

void EditorNode::add_extension_editor_plugin(const StringName &p_class_name) {
	ERR_FAIL_COND_MSG(!ClassDB::class_exists(p_class_name), vformat("No such editor plugin registered: %s", p_class_name));
	ERR_FAIL_COND_MSG(!ClassDB::is_parent_class(p_class_name, SNAME("EditorPlugin")), vformat("Class is not an editor plugin: %s", p_class_name));
	ERR_FAIL_COND_MSG(singleton->editor_data.has_extension_editor_plugin(p_class_name), vformat("Editor plugin already added for class: %s", p_class_name));

	EditorPlugin *plugin = Object::cast_to<EditorPlugin>(ClassDB::_instantiate_allow_unexposed(p_class_name));
	singleton->editor_data.add_extension_editor_plugin(p_class_name, plugin);
	add_editor_plugin(plugin);
}

void EditorNode::remove_extension_editor_plugin(const StringName &p_class_name) {
	// If we're exiting, the editor plugins will get cleaned up anyway, so don't do anything.
	if (!singleton || singleton->exiting) {
		return;
	}

	ERR_FAIL_COND_MSG(!singleton->editor_data.has_extension_editor_plugin(p_class_name), vformat("No editor plugin added for class: %s", p_class_name));

	EditorPlugin *plugin = singleton->editor_data.get_extension_editor_plugin(p_class_name);
	remove_editor_plugin(plugin);
	memdelete(plugin);
	singleton->editor_data.remove_extension_editor_plugin(p_class_name);
}

void EditorNode::_update_addon_config() {
	if (_initializing_plugins) {
		return;
	}

	Vector<String> enabled_addons;

	for (const KeyValue<String, EditorPlugin *> &E : addon_name_to_plugin) {
		enabled_addons.push_back(E.key);
	}

	if (enabled_addons.is_empty()) {
		ProjectSettings::get_singleton()->set("editor_plugins/enabled", Variant());
	} else {
		enabled_addons.sort();
		ProjectSettings::get_singleton()->set("editor_plugins/enabled", enabled_addons);
	}

	project_settings_editor->queue_save();
}

void EditorNode::set_addon_plugin_enabled(const String &p_addon, bool p_enabled, bool p_config_changed) {
	String addon_path = p_addon;

	if (!addon_path.begins_with("res://")) {
		addon_path = "res://addons/" + p_addon + "/plugin.cfg";
	}

	ERR_FAIL_COND(p_enabled && addon_name_to_plugin.has(addon_path));
	ERR_FAIL_COND(!p_enabled && !addon_name_to_plugin.has(addon_path));

	if (!p_enabled) {
		EditorPlugin *addon = addon_name_to_plugin[addon_path];
		remove_editor_plugin(addon, p_config_changed);
		memdelete(addon);
		addon_name_to_plugin.erase(addon_path);
		_update_addon_config();
		return;
	}

	Ref<ConfigFile> cf;
	cf.instantiate();
	if (!DirAccess::exists(addon_path.get_base_dir())) {
		_remove_plugin_from_enabled(addon_path);
		WARN_PRINT("Addon '" + addon_path + "' failed to load. No directory found. Removing from enabled plugins.");
		return;
	}
	Error err = cf->load(addon_path);
	if (err != OK) {
		show_warning(vformat(TTR("Unable to enable addon plugin at: '%s' parsing of config failed."), addon_path));
		return;
	}

	String plugin_version;
	if (cf->has_section_key("plugin", "version")) {
		plugin_version = cf->get_value("plugin", "version");
	}

	if (!cf->has_section_key("plugin", "script")) {
		show_warning(vformat(TTR("Unable to find script field for addon plugin at: '%s'."), addon_path));
		return;
	}

	String script_path = cf->get_value("plugin", "script");
	Ref<Script> scr; // We need to save it for creating "ep" below.

	// Only try to load the script if it has a name. Else, the plugin has no init script.
	EditorPlugin *ep = nullptr;
	if (script_path.length() > 0) {
		script_path = addon_path.get_base_dir().path_join(script_path);
		// We should not use the cached version on startup to prevent a script reload
		// if it is already loaded and potentially running from autoloads. See GH-100750.
		scr = ResourceLoader::load(script_path, "Script", EditorFileSystem::get_singleton()->doing_first_scan() ? ResourceFormatLoader::CACHE_MODE_REUSE : ResourceFormatLoader::CACHE_MODE_IGNORE);

		if (scr.is_null()) {
			show_warning(vformat(TTR("Unable to load addon script from path: '%s'."), script_path));
			return;
		}

		// Errors in the script cause the base_type to be an empty StringName.
		if (scr->get_instance_base_type() == StringName()) {
			if (_initializing_plugins) {
				// However, if it happens during initialization, waiting for file scan might help.
				pending_addons.push_back(p_addon);
				return;
			}

			show_warning(vformat(TTR("Unable to load addon script from path: '%s'. This might be due to a code error in that script.\nDisabling the addon at '%s' to prevent further errors."), script_path, addon_path));
			_remove_plugin_from_enabled(addon_path);
			return;
		}

		// Plugin init scripts must inherit from EditorPlugin and be tools.
		if (!ClassDB::is_parent_class(scr->get_instance_base_type(), "EditorPlugin")) {
			show_warning(vformat(TTR("Unable to load addon script from path: '%s'. Base type is not 'EditorPlugin'."), script_path));
			return;
		}

		if (!scr->is_tool()) {
			show_warning(vformat(TTR("Unable to load addon script from path: '%s'. Script is not in tool mode."), script_path));
			return;
		}

		Object *obj = ClassDB::instantiate(scr->get_instance_base_type());
		ep = Object::cast_to<EditorPlugin>(obj);
		ERR_FAIL_NULL(ep);
		ep->set_script(scr);
	} else {
		ep = memnew(EditorPlugin);
	}

	ep->set_plugin_version(plugin_version);
	addon_name_to_plugin[addon_path] = ep;
	add_editor_plugin(ep, p_config_changed);

	_update_addon_config();
}

bool EditorNode::is_addon_plugin_enabled(const String &p_addon) const {
	if (p_addon.begins_with("res://")) {
		return addon_name_to_plugin.has(p_addon);
	}

	return addon_name_to_plugin.has("res://addons/" + p_addon + "/plugin.cfg");
}

void EditorNode::_enable_pending_addons() {
	for (uint32_t i = 0; i < pending_addons.size(); i++) {
		set_addon_plugin_enabled(pending_addons[i], true);
	}
	pending_addons.clear();
}

