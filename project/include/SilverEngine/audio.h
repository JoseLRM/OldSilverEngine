#pragma once

#include "SilverEngine/scene.h"	

namespace sv {

	SV_DEFINE_HANDLE(AudioDevice);
	SV_DEFINE_HANDLE(AudioSource);
	SV_DEFINE_HANDLE(Sound);

	Result create_audio_device(AudioDevice** pdevice);
	Result destroy_audio_device(AudioDevice* device);

	Result create_audio_source(AudioDevice* device, Sound* sound, AudioSource** psource);
	Result destroy_audio_source(AudioDevice* device, AudioSource* source);

	Result create_sound(const char* filepath, Sound** psound);
	Result destroy_sound(Sound* sound);

	void play_sound(AudioSource* source);

	void set_audio_volume(AudioDevice* device, f32 volume);
	void set_audio_volume(AudioSource* source, f32 volume);

	f32 get_audio_volume(AudioDevice* device);
	f32 get_audio_volume(AudioSource* source);

}