/**************************************************************************/
/*  editor_node_settings.cpp                                             */
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
#include "core/io/resource_loader.h"
#include "core/string/translation_server.h"
#include "editor/import/resource_importer_texture.h"
#include "scene/gui/control.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "servers/navigation_2d/navigation_server_2d.h"
#include "servers/navigation_3d/navigation_server_3d.h"
#include "servers/rendering/rendering_server.h"

void EditorNode::_update_from_settings() {
	if (!is_inside_tree()) {
		return;
	}
	_update_title();

	int current_filter = GLOBAL_GET("rendering/textures/canvas_textures/default_texture_filter");
	if (current_filter != scene_root->get_default_canvas_item_texture_filter()) {
		Viewport::DefaultCanvasItemTextureFilter tf = (Viewport::DefaultCanvasItemTextureFilter)current_filter;
		scene_root->set_default_canvas_item_texture_filter(tf);
	}
	int current_repeat = GLOBAL_GET("rendering/textures/canvas_textures/default_texture_repeat");
	if (current_repeat != scene_root->get_default_canvas_item_texture_repeat()) {
		Viewport::DefaultCanvasItemTextureRepeat tr = (Viewport::DefaultCanvasItemTextureRepeat)current_repeat;
		scene_root->set_default_canvas_item_texture_repeat(tr);
	}
	String current_fallback_locale = GLOBAL_GET("internationalization/locale/fallback");
	if (current_fallback_locale != TranslationServer::get_singleton()->get_fallback_locale()) {
		TranslationServer::get_singleton()->set_fallback_locale(current_fallback_locale);
		Ref<TranslationDomain> domain = TranslationServer::get_singleton()->get_main_domain();
		if (!domain->is_enabled()) {
			domain->set_locale_override(current_fallback_locale);
		}
		scene_root->propagate_notification(Control::NOTIFICATION_LAYOUT_DIRECTION_CHANGED);
	}

	RS::DOFBokehShape dof_shape = RS::DOFBokehShape(int(GLOBAL_GET("rendering/camera/depth_of_field/depth_of_field_bokeh_shape")));
	RS::get_singleton()->camera_attributes_set_dof_blur_bokeh_shape(dof_shape);
	RS::DOFBlurQuality dof_quality = RS::DOFBlurQuality(int(GLOBAL_GET("rendering/camera/depth_of_field/depth_of_field_bokeh_quality")));
	bool dof_jitter = GLOBAL_GET("rendering/camera/depth_of_field/depth_of_field_use_jitter");
	RS::get_singleton()->camera_attributes_set_dof_blur_quality(dof_quality, dof_jitter);
	RS::get_singleton()->environment_set_ssao_quality(RS::EnvironmentSSAOQuality(int(GLOBAL_GET("rendering/environment/ssao/quality"))), GLOBAL_GET("rendering/environment/ssao/half_size"), GLOBAL_GET("rendering/environment/ssao/adaptive_target"), GLOBAL_GET("rendering/environment/ssao/blur_passes"), GLOBAL_GET("rendering/environment/ssao/fadeout_from"), GLOBAL_GET("rendering/environment/ssao/fadeout_to"));
	RS::get_singleton()->screen_space_roughness_limiter_set_active(GLOBAL_GET("rendering/anti_aliasing/screen_space_roughness_limiter/enabled"), GLOBAL_GET("rendering/anti_aliasing/screen_space_roughness_limiter/amount"), GLOBAL_GET("rendering/anti_aliasing/screen_space_roughness_limiter/limit"));
	bool glow_bicubic = int(GLOBAL_GET("rendering/environment/glow/upscale_mode")) > 0;
	RS::get_singleton()->environment_set_ssil_quality(RS::EnvironmentSSILQuality(int(GLOBAL_GET("rendering/environment/ssil/quality"))), GLOBAL_GET("rendering/environment/ssil/half_size"), GLOBAL_GET("rendering/environment/ssil/adaptive_target"), GLOBAL_GET("rendering/environment/ssil/blur_passes"), GLOBAL_GET("rendering/environment/ssil/fadeout_from"), GLOBAL_GET("rendering/environment/ssil/fadeout_to"));
	RS::get_singleton()->environment_glow_set_use_bicubic_upscale(glow_bicubic);
	RS::get_singleton()->environment_set_ssr_half_size(GLOBAL_GET("rendering/environment/screen_space_reflection/half_size"));
	RS::SubSurfaceScatteringQuality sss_quality = RS::SubSurfaceScatteringQuality(int(GLOBAL_GET("rendering/environment/subsurface_scattering/subsurface_scattering_quality")));
	RS::get_singleton()->sub_surface_scattering_set_quality(sss_quality);
	float sss_scale = GLOBAL_GET("rendering/environment/subsurface_scattering/subsurface_scattering_scale");
	float sss_depth_scale = GLOBAL_GET("rendering/environment/subsurface_scattering/subsurface_scattering_depth_scale");
	RS::get_singleton()->sub_surface_scattering_set_scale(sss_scale, sss_depth_scale);

	uint32_t directional_shadow_size = GLOBAL_GET("rendering/lights_and_shadows/directional_shadow/size");
	uint32_t directional_shadow_16_bits = GLOBAL_GET("rendering/lights_and_shadows/directional_shadow/16_bits");
	RS::get_singleton()->directional_shadow_atlas_set_size(directional_shadow_size, directional_shadow_16_bits);

	RS::ShadowQuality shadows_quality = RS::ShadowQuality(int(GLOBAL_GET("rendering/lights_and_shadows/positional_shadow/soft_shadow_filter_quality")));
	RS::get_singleton()->positional_soft_shadow_filter_set_quality(shadows_quality);
	RS::ShadowQuality directional_shadow_quality = RS::ShadowQuality(int(GLOBAL_GET("rendering/lights_and_shadows/directional_shadow/soft_shadow_filter_quality")));
	RS::get_singleton()->directional_soft_shadow_filter_set_quality(directional_shadow_quality);
	float probe_update_speed = GLOBAL_GET("rendering/lightmapping/probe_capture/update_speed");
	RS::get_singleton()->lightmap_set_probe_capture_update_speed(probe_update_speed);
	RS::EnvironmentSDFGIFramesToConverge frames_to_converge = RS::EnvironmentSDFGIFramesToConverge(int(GLOBAL_GET("rendering/global_illumination/sdfgi/frames_to_converge")));
	RS::get_singleton()->environment_set_sdfgi_frames_to_converge(frames_to_converge);
	RS::EnvironmentSDFGIRayCount ray_count = RS::EnvironmentSDFGIRayCount(int(GLOBAL_GET("rendering/global_illumination/sdfgi/probe_ray_count")));
	RS::get_singleton()->environment_set_sdfgi_ray_count(ray_count);
	RS::VoxelGIQuality voxel_gi_quality = RS::VoxelGIQuality(int(GLOBAL_GET("rendering/global_illumination/voxel_gi/quality")));
	RS::get_singleton()->voxel_gi_set_quality(voxel_gi_quality);
	RS::get_singleton()->environment_set_volumetric_fog_volume_size(GLOBAL_GET("rendering/environment/volumetric_fog/volume_size"), GLOBAL_GET("rendering/environment/volumetric_fog/volume_depth"));
	RS::get_singleton()->environment_set_volumetric_fog_filter_active(bool(GLOBAL_GET("rendering/environment/volumetric_fog/use_filter")));
	RS::get_singleton()->canvas_set_shadow_texture_size(GLOBAL_GET("rendering/2d/shadow_atlas/size"));

	bool use_half_res_gi = GLOBAL_GET("rendering/global_illumination/gi/use_half_resolution");
	RS::get_singleton()->gi_set_use_half_resolution(use_half_res_gi);

	bool snap_2d_transforms = GLOBAL_GET("rendering/2d/snap/snap_2d_transforms_to_pixel");
	scene_root->set_snap_2d_transforms_to_pixel(snap_2d_transforms);
	bool snap_2d_vertices = GLOBAL_GET("rendering/2d/snap/snap_2d_vertices_to_pixel");
	scene_root->set_snap_2d_vertices_to_pixel(snap_2d_vertices);

	Viewport::SDFOversize sdf_oversize = Viewport::SDFOversize(int(GLOBAL_GET("rendering/2d/sdf/oversize")));
	scene_root->set_sdf_oversize(sdf_oversize);
	Viewport::SDFScale sdf_scale = Viewport::SDFScale(int(GLOBAL_GET("rendering/2d/sdf/scale")));
	scene_root->set_sdf_scale(sdf_scale);

	Viewport::MSAA msaa = Viewport::MSAA(int(GLOBAL_GET("rendering/anti_aliasing/quality/msaa_2d")));
	scene_root->set_msaa_2d(msaa);

	// 2D doesn't use a dedicated SubViewport like 3D does, so we apply it on the root viewport instead.
	bool use_debanding = GLOBAL_GET("rendering/anti_aliasing/quality/use_debanding");
	scene_root->set_use_debanding(use_debanding);
	get_viewport()->set_use_debanding(use_debanding);

	bool use_hdr_2d = GLOBAL_GET("rendering/viewport/hdr_2d");
	scene_root->set_use_hdr_2d(use_hdr_2d);
	get_viewport()->set_use_hdr_2d(use_hdr_2d);

	float mesh_lod_threshold = GLOBAL_GET("rendering/mesh_lod/lod_change/threshold_pixels");
	scene_root->set_mesh_lod_threshold(mesh_lod_threshold);

	RS::get_singleton()->decals_set_filter(RS::DecalFilter(int(GLOBAL_GET("rendering/textures/decals/filter"))));
	RS::get_singleton()->light_projectors_set_filter(RS::LightProjectorFilter(int(GLOBAL_GET("rendering/textures/light_projectors/filter"))));
	RS::get_singleton()->lightmaps_set_bicubic_filter(GLOBAL_GET("rendering/lightmapping/lightmap_gi/use_bicubic_filter"));
	RS::get_singleton()->material_set_use_debanding(GLOBAL_GET("rendering/anti_aliasing/quality/use_debanding"));

	SceneTree *tree = get_tree();
	tree->set_debug_collisions_color(GLOBAL_GET("debug/shapes/collision/shape_color"));
	tree->set_debug_collision_contact_color(GLOBAL_GET("debug/shapes/collision/contact_color"));

	ResourceImporterTexture::get_singleton()->update_imports();

	_update_translations();

#ifdef DEBUG_ENABLED
	NavigationServer2D::get_singleton()->set_debug_navigation_edge_connection_color(GLOBAL_GET("debug/shapes/navigation/2d/edge_connection_color"));
	NavigationServer2D::get_singleton()->set_debug_navigation_geometry_edge_color(GLOBAL_GET("debug/shapes/navigation/2d/geometry_edge_color"));
	NavigationServer2D::get_singleton()->set_debug_navigation_geometry_face_color(GLOBAL_GET("debug/shapes/navigation/2d/geometry_face_color"));
	NavigationServer2D::get_singleton()->set_debug_navigation_geometry_edge_disabled_color(GLOBAL_GET("debug/shapes/navigation/2d/geometry_edge_disabled_color"));
	NavigationServer2D::get_singleton()->set_debug_navigation_geometry_face_disabled_color(GLOBAL_GET("debug/shapes/navigation/2d/geometry_face_disabled_color"));
	NavigationServer2D::get_singleton()->set_debug_navigation_enable_edge_connections(GLOBAL_GET("debug/shapes/navigation/2d/enable_edge_connections"));
	NavigationServer2D::get_singleton()->set_debug_navigation_enable_edge_lines(GLOBAL_GET("debug/shapes/navigation/2d/enable_edge_lines"));
	NavigationServer2D::get_singleton()->set_debug_navigation_enable_geometry_face_random_color(GLOBAL_GET("debug/shapes/navigation/2d/enable_geometry_face_random_color"));

	NavigationServer3D::get_singleton()->set_debug_navigation_edge_connection_color(GLOBAL_GET("debug/shapes/navigation/3d/edge_connection_color"));
	NavigationServer3D::get_singleton()->set_debug_navigation_geometry_edge_color(GLOBAL_GET("debug/shapes/navigation/3d/geometry_edge_color"));
	NavigationServer3D::get_singleton()->set_debug_navigation_geometry_face_color(GLOBAL_GET("debug/shapes/navigation/3d/geometry_face_color"));
	NavigationServer3D::get_singleton()->set_debug_navigation_geometry_edge_disabled_color(GLOBAL_GET("debug/shapes/navigation/3d/geometry_edge_disabled_color"));
	NavigationServer3D::get_singleton()->set_debug_navigation_geometry_face_disabled_color(GLOBAL_GET("debug/shapes/navigation/3d/geometry_face_disabled_color"));
	NavigationServer3D::get_singleton()->set_debug_navigation_enable_edge_connections(GLOBAL_GET("debug/shapes/navigation/3d/enable_edge_connections"));
	NavigationServer3D::get_singleton()->set_debug_navigation_enable_edge_connections_xray(GLOBAL_GET("debug/shapes/navigation/3d/enable_edge_connections_xray"));
	NavigationServer3D::get_singleton()->set_debug_navigation_enable_edge_lines(GLOBAL_GET("debug/shapes/navigation/3d/enable_edge_lines"));
	NavigationServer3D::get_singleton()->set_debug_navigation_enable_edge_lines_xray(GLOBAL_GET("debug/shapes/navigation/3d/enable_edge_lines_xray"));
	NavigationServer3D::get_singleton()->set_debug_navigation_enable_geometry_face_random_color(GLOBAL_GET("debug/shapes/navigation/3d/enable_geometry_face_random_color"));
#endif // DEBUG_ENABLED
}

