#pragma once

#include "core/asset_system.h"

namespace sv {

	SV_DEFINE_ASSET_PTR(Sound, void*);
	struct AudioSource;

	SV_API void audio_source_create(AudioSource** source);
	SV_API void audio_source_destroy(AudioSource* source);

	SV_API void audio_source_set(AudioSource* source, Sound& sound);

	SV_API void audio_play(AudioSource* source);
	SV_API void audio_pause(AudioSource* source);
	SV_API void audio_stop(AudioSource* source);

	SV_API void audio_global_volume_set(f32 volume);
	SV_API void audio_volume_set(AudioSource* source, f32 volume);

	SV_API f32 audio_global_volume_get();
	SV_API f32 audio_volume_get(AudioSource* source);

	bool _audio_initialize();
	void _audio_close();

}
