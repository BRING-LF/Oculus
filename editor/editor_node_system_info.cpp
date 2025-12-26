/**************************************************************************/
/*  editor_node_system_info.cpp                                            */
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

#include "core/os/os.h"
#include "core/string/ustring.h"
#include "core/version.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_server.h"

String EditorNode::_get_system_info() const {
	String distribution_name = OS::get_singleton()->get_distribution_name();
	if (distribution_name.is_empty()) {
		distribution_name = OS::get_singleton()->get_name();
	}
	if (distribution_name.is_empty()) {
		distribution_name = "Other";
	}
	const String distribution_version = OS::get_singleton()->get_version_alias();

	String godot_version = "Godot v" + String(GODOT_VERSION_FULL_CONFIG);
	if (String(GODOT_VERSION_BUILD) != "official") {
		String hash = String(GODOT_VERSION_HASH);
		hash = hash.is_empty() ? String("unknown") : vformat("(%s)", hash.left(9));
		godot_version += " " + hash;
	}

	String display_session_type;
#ifdef LINUXBSD_ENABLED
	// `remove_char` is necessary, because `capitalize` introduces a whitespace between "x" and "11".
	display_session_type = OS::get_singleton()->get_environment("XDG_SESSION_TYPE").capitalize().remove_char(' ');
#endif // LINUXBSD_ENABLED
	String driver_name = OS::get_singleton()->get_current_rendering_driver_name().to_lower();
	String rendering_method = OS::get_singleton()->get_current_rendering_method().to_lower();

	const String rendering_device_name = RenderingServer::get_singleton()->get_video_adapter_name();

	RenderingDevice::DeviceType device_type = RenderingServer::get_singleton()->get_video_adapter_type();
	String device_type_string;
	switch (device_type) {
		case RenderingDevice::DeviceType::DEVICE_TYPE_INTEGRATED_GPU:
			device_type_string = "integrated";
			break;
		case RenderingDevice::DeviceType::DEVICE_TYPE_DISCRETE_GPU:
			device_type_string = "dedicated";
			break;
		case RenderingDevice::DeviceType::DEVICE_TYPE_VIRTUAL_GPU:
			device_type_string = "virtual";
			break;
		case RenderingDevice::DeviceType::DEVICE_TYPE_CPU:
			device_type_string = "(software emulation on CPU)";
			break;
		case RenderingDevice::DeviceType::DEVICE_TYPE_OTHER:
		case RenderingDevice::DeviceType::DEVICE_TYPE_MAX:
			break; // Can't happen, but silences warning for DEVICE_TYPE_MAX
	}

	const Vector<String> video_adapter_driver_info = OS::get_singleton()->get_video_adapter_driver_info();

	const String processor_name = OS::get_singleton()->get_processor_name();
	const int processor_count = OS::get_singleton()->get_processor_count();

	// Prettify
	if (rendering_method == "forward_plus") {
		rendering_method = "Forward+";
	} else if (rendering_method == "mobile") {
		rendering_method = "Mobile";
	} else if (rendering_method == "gl_compatibility") {
		rendering_method = "Compatibility";
	}
	if (driver_name == "vulkan") {
		driver_name = "Vulkan";
	} else if (driver_name == "d3d12") {
		driver_name = "Direct3D 12";
	} else if (driver_name == "opengl3_angle") {
		driver_name = "OpenGL ES 3/ANGLE";
	} else if (driver_name == "opengl3_es") {
		driver_name = "OpenGL ES 3";
	} else if (driver_name == "opengl3") {
		if (OS::get_singleton()->get_gles_over_gl()) {
			driver_name = "OpenGL 3";
		} else {
			driver_name = "OpenGL ES 3";
		}
	} else if (driver_name == "metal") {
		driver_name = "Metal";
	}

	// Join info.
	Vector<String> info;
	info.push_back(godot_version);
	String distribution_display_session_type = distribution_name;
	if (!distribution_version.is_empty()) {
		distribution_display_session_type += " " + distribution_version;
	}
	if (!display_session_type.is_empty()) {
		distribution_display_session_type += " on " + display_session_type;
	}
	info.push_back(distribution_display_session_type);

	String display_driver_window_mode;
#ifdef LINUXBSD_ENABLED
	// `remove_char` is necessary, because `capitalize` introduces a whitespace between "x" and "11".
	display_driver_window_mode = DisplayServer::get_singleton()->get_name().capitalize().remove_char(' ') + " display driver";
#endif // LINUXBSD_ENABLED
	if (!display_driver_window_mode.is_empty()) {
		display_driver_window_mode += ", ";
	}
	display_driver_window_mode += get_viewport()->is_embedding_subwindows() ? "Single-window" : "Multi-window";

	if (DisplayServer::get_singleton()->get_screen_count() == 1) {
		display_driver_window_mode += ", " + itos(DisplayServer::get_singleton()->get_screen_count()) + " monitor";
	} else {
		display_driver_window_mode += ", " + itos(DisplayServer::get_singleton()->get_screen_count()) + " monitors";
	}

	info.push_back(display_driver_window_mode);

	info.push_back(vformat("%s (%s)", driver_name, rendering_method));

	String graphics;
	if (!device_type_string.is_empty()) {
		graphics = device_type_string + " ";
	}
	graphics += rendering_device_name;
	if (video_adapter_driver_info.size() == 2) { // This vector is always either of length 0 or 2.
		const String &vad_name = video_adapter_driver_info[0];
		const String &vad_version = video_adapter_driver_info[1]; // Version could be potentially empty on Linux/BSD.
		if (!vad_version.is_empty()) {
			graphics += vformat(" (%s; %s)", vad_name, vad_version);
		} else if (!vad_name.is_empty()) {
			graphics += vformat(" (%s)", vad_name);
		}
	}
	info.push_back(graphics);

	info.push_back(vformat("%s (%d threads)", processor_name, processor_count));

	const int64_t system_ram = OS::get_singleton()->get_memory_info()["physical"];
	if (system_ram > 0) {
		// If the memory info is available, display it.
		info.push_back(vformat("%s memory", String::humanize_size(system_ram)));
	}

	return String(" - ").join(info);
}

