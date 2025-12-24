/**************************************************************************/
/*  editor_node_translations.cpp                                          */
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
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"

void EditorNode::_update_translations() {
	Ref<TranslationDomain> main = TranslationServer::get_singleton()->get_main_domain();

	TranslationServer::get_singleton()->load_project_translations(main);

	if (main->is_enabled()) {
		// Check for the exact locale.
		if (main->has_translation_for_locale(main->get_locale_override(), true)) {
			// The set of translation resources for the current locale changed.
			const HashSet<Ref<Translation>> translations = main->find_translations(main->get_locale_override(), false);
			if (translations != tracked_translations) {
				_translation_resources_changed();
			}
		} else {
			// Translations for the current preview locale is removed.
			main->set_enabled(false);
			main->set_locale_override(String());
			_translation_resources_changed();
		}
	}
}

void EditorNode::_translation_resources_changed() {
	for (const Ref<Translation> &E : tracked_translations) {
		E->disconnect_changed(callable_mp(this, &EditorNode::_queue_translation_notification));
	}
	tracked_translations.clear();

	const Ref<TranslationDomain> main = TranslationServer::get_singleton()->get_main_domain();
	if (main->is_enabled()) {
		const HashSet<Ref<Translation>> translations = main->find_translations(main->get_locale_override(), false);
		tracked_translations.reserve(translations.size());
		for (const Ref<Translation> &translation : translations) {
			translation->connect_changed(callable_mp(this, &EditorNode::_queue_translation_notification));
			tracked_translations.insert(translation);
		}
	}

	_queue_translation_notification();
	emit_signal(SNAME("preview_locale_changed"));
}

void EditorNode::_queue_translation_notification() {
	if (pending_translation_notification) {
		return;
	}
	pending_translation_notification = true;
	callable_mp(this, &EditorNode::_propagate_translation_notification).call_deferred();
}

void EditorNode::_propagate_translation_notification() {
	pending_translation_notification = false;
	scene_root->propagate_notification(NOTIFICATION_TRANSLATION_CHANGED);
}

