#pragma once

#include "defines.h"

namespace sv {

	struct Sound;
	struct AudioSource;

	SV_API bool audio_source_create(AudioSource** source, Sound* sound);
	SV_API void audio_source_destroy(AudioSource* source);

	SV_API bool audio_sound_load(Sound** sound, const char* filepath);
	SV_API void audio_sound_destroy(Sound* sound);

	SV_API void audio_play(AudioSource* source);

	SV_API void audio_global_volume_set(f32 volume);
	SV_API void audio_volume_set(AudioSource* source, f32 volume);

	SV_API f32 audio_global_volume_get();
	SV_API f32 audio_volume_get(AudioSource* source);

	bool _audio_initialize();
	void _audio_close();

}
