/**************************************************************************/
/*  editor_node_init_importers.cpp                                       */
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

#include "core/io/resource_importer.h"
#include "editor/import/3d/editor_import_collada.h"
#include "editor/import/3d/resource_importer_obj.h"
#include "editor/import/3d/resource_importer_scene.h"
#include "editor/import/resource_importer_bitmask.h"
#include "editor/import/resource_importer_bmfont.h"
#include "editor/import/resource_importer_csv_translation.h"
#include "editor/import/resource_importer_dynamic_font.h"
#include "editor/import/resource_importer_image.h"
#include "editor/import/resource_importer_imagefont.h"
#include "editor/import/resource_importer_layered_texture.h"
#include "editor/import/resource_importer_shader_file.h"
#include "editor/import/resource_importer_svg.h"
#include "editor/import/resource_importer_texture.h"
#include "editor/import/resource_importer_texture_atlas.h"
#include "editor/import/resource_importer_wav.h"

void EditorNode::_init_importers() {
	// Register importers at the beginning, so dialogs are created with the right extensions.
	Ref<ResourceImporterTexture> import_texture = memnew(ResourceImporterTexture(true));
	ResourceFormatImporter::get_singleton()->add_importer(import_texture);

	Ref<ResourceImporterLayeredTexture> import_cubemap;
	import_cubemap.instantiate();
	import_cubemap->set_mode(ResourceImporterLayeredTexture::MODE_CUBEMAP);
	ResourceFormatImporter::get_singleton()->add_importer(import_cubemap);

	Ref<ResourceImporterLayeredTexture> import_array;
	import_array.instantiate();
	import_array->set_mode(ResourceImporterLayeredTexture::MODE_2D_ARRAY);
	ResourceFormatImporter::get_singleton()->add_importer(import_array);

	Ref<ResourceImporterLayeredTexture> import_cubemap_array;
	import_cubemap_array.instantiate();
	import_cubemap_array->set_mode(ResourceImporterLayeredTexture::MODE_CUBEMAP_ARRAY);
	ResourceFormatImporter::get_singleton()->add_importer(import_cubemap_array);

	Ref<ResourceImporterLayeredTexture> import_3d = memnew(ResourceImporterLayeredTexture(true));
	import_3d->set_mode(ResourceImporterLayeredTexture::MODE_3D);
	ResourceFormatImporter::get_singleton()->add_importer(import_3d);

	Ref<ResourceImporterImage> import_image;
	import_image.instantiate();
	ResourceFormatImporter::get_singleton()->add_importer(import_image);

	Ref<ResourceImporterSVG> import_svg;
	import_svg.instantiate();
	ResourceFormatImporter::get_singleton()->add_importer(import_svg);

	Ref<ResourceImporterTextureAtlas> import_texture_atlas;
	import_texture_atlas.instantiate();
	ResourceFormatImporter::get_singleton()->add_importer(import_texture_atlas);

	Ref<ResourceImporterDynamicFont> import_font_data_dynamic;
	import_font_data_dynamic.instantiate();
	ResourceFormatImporter::get_singleton()->add_importer(import_font_data_dynamic);

	Ref<ResourceImporterBMFont> import_font_data_bmfont;
	import_font_data_bmfont.instantiate();
	ResourceFormatImporter::get_singleton()->add_importer(import_font_data_bmfont);

	Ref<ResourceImporterImageFont> import_font_data_image;
	import_font_data_image.instantiate();
	ResourceFormatImporter::get_singleton()->add_importer(import_font_data_image);

	Ref<ResourceImporterCSVTranslation> import_csv_translation;
	import_csv_translation.instantiate();
	ResourceFormatImporter::get_singleton()->add_importer(import_csv_translation);

	Ref<ResourceImporterWAV> import_wav;
	import_wav.instantiate();
	ResourceFormatImporter::get_singleton()->add_importer(import_wav);

	Ref<ResourceImporterOBJ> import_obj;
	import_obj.instantiate();
	ResourceFormatImporter::get_singleton()->add_importer(import_obj);

	Ref<ResourceImporterShaderFile> import_shader_file;
	import_shader_file.instantiate();
	ResourceFormatImporter::get_singleton()->add_importer(import_shader_file);

	Ref<ResourceImporterScene> import_model_as_scene;
	import_model_as_scene.instantiate("PackedScene");
	ResourceFormatImporter::get_singleton()->add_importer(import_model_as_scene);

	Ref<ResourceImporterScene> import_model_as_animation;
	import_model_as_animation.instantiate("AnimationLibrary");
	ResourceFormatImporter::get_singleton()->add_importer(import_model_as_animation);

	{
		Ref<EditorSceneFormatImporterCollada> import_collada;
		import_collada.instantiate();
		ResourceImporterScene::add_scene_importer(import_collada);

		Ref<EditorOBJImporter> import_obj2;
		import_obj2.instantiate();
		ResourceImporterScene::add_scene_importer(import_obj2);

		Ref<EditorSceneFormatImporterESCN> import_escn;
		import_escn.instantiate();
		ResourceImporterScene::add_scene_importer(import_escn);
	}

	Ref<ResourceImporterBitMap> import_bitmap;
	import_bitmap.instantiate();
	ResourceFormatImporter::get_singleton()->add_importer(import_bitmap);
}

