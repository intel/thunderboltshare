#include "pch.h"
#include "ThunderboltShareSource.h"
#include "SourceProperties.h"
#include "Util.h"

//
// This file implements the entry points for the OBS Studio Plugin DLL
//

using namespace std;

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("thunderbolt-share", "en-US")
MODULE_EXPORT const char* obs_module_description(void)
{
	return "Thunderbolt Share Source";
}

static bool remote_audio_changed(obs_properties_t* props, obs_property_t*, obs_data_t* settings)
{
	bool enabled = obs_data_get_bool(settings, S_AUDIO);
	obs_property_t* audioDelayProp = obs_properties_get(props, S_AUDIO_DELAY);

	if (audioDelayProp != NULL)
	{
		obs_property_set_visible(audioDelayProp, enabled);
	}	

	return true;
}

static obs_properties_t* get_properties(void* data)
{
	CThunderboltShareSource* s = reinterpret_cast<CThunderboltShareSource*>(data);

	string path;

	obs_properties_t* props = obs_properties_create();
	obs_property_t* p;

	p = obs_properties_add_list(props, S_MONITOR, T_MONITOR, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, "0", 0);
	obs_property_list_add_int(p, "1", 1);
	obs_property_list_add_int(p, "2", 2);
	obs_property_list_add_int(p, "3", 3);
	obs_property_list_add_int(p, "4", 4);
	obs_property_list_add_int(p, "5", 5);
	obs_property_list_add_int(p, "6", 6);
	obs_property_list_add_int(p, "7", 7);
	obs_property_list_add_int(p, "8", 8);
	obs_property_list_add_int(p, "9", 9);

	p = obs_properties_add_list(props, S_COLOR_DEPTH, T_COLOR_DEPTH, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, "24-bit", 24);
	obs_property_list_add_int(p, "16-bit", 16);
	obs_property_list_add_int(p, "8-bit", 8);

	obs_properties_add_int(props, S_FPS_LIMIT, T_FPS_LIMIT, 0, 1000, 1);

	p = obs_properties_add_bool(props, S_AUDIO, T_AUDIO);

	obs_property_set_modified_callback(p, remote_audio_changed);

	obs_properties_add_int(props, S_AUDIO_DELAY, T_AUDIO_DELAY, -1000, 1000, 1);
	
	return props;
}

static void get_defaults(obs_data_t* settings)
{
	obs_data_set_default_int(settings, S_MONITOR, 0);
	obs_data_set_default_int(settings, S_COLOR_DEPTH, 24);
	obs_data_set_default_int(settings, S_FPS_LIMIT, 60);
	obs_data_set_default_bool(settings, S_AUDIO, false);
	obs_data_set_default_int(settings, S_AUDIO_DELAY, 10);
};

bool obs_module_load(void)
{
	obs_source_info si = {};
	si.id = "thunderbolt_share";
	si.type = OBS_SOURCE_TYPE_INPUT;
	si.output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE;
	si.get_properties = get_properties;
	si.icon_type = OBS_ICON_TYPE_DESKTOP_CAPTURE;

	si.get_name = [](void*) {
		return obs_module_text("Thunderbolt Share Capture");
		};
	si.create = [](obs_data_t* settings, obs_source_t* source) {
		LogInfo("Create() called");
		return (void*)new CThunderboltShareSource(source, settings);
		};
	si.destroy = [](void* data) {
		LogInfo("Destroy() called");
		delete reinterpret_cast<CThunderboltShareSource*>(data);
		};
	si.get_defaults = [](obs_data_t* settings) {
		get_defaults(settings);
		};
	si.update = [](void* data, obs_data_t* settings) {
		LogInfo("Update() called");
		reinterpret_cast<CThunderboltShareSource*>(data)->Update(settings);
		};
	si.show = [](void* data) {
		LogInfo("Show() called");
		reinterpret_cast<CThunderboltShareSource*>(data)->Show();
		};
	si.hide = [](void* data) {
		LogInfo("Hide() called");
		reinterpret_cast<CThunderboltShareSource*>(data)->Hide();
		};

	obs_register_source(&si);
	return true;
}

void obs_module_unload(void)
{	
}
