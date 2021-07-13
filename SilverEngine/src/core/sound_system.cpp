#include "core/sound_system.h"

namespace sv {

	void AudioSourceComponent_create(AudioSourceComponent* comp, Entity entity)
	{
		audio_source_create(&comp->source);
	}
	
	void AudioSourceComponent_destroy(AudioSourceComponent* comp, Entity entity)
	{
		audio_source_destroy(comp->source);
	}
	
	void AudioSourceComponent_copy(AudioSourceComponent* dst, const AudioSourceComponent* src, Entity entity)
	{
		audio_source_create(&dst->source);

		f32 volume = audio_volume_get(src->source);

		Sound s;
		u32 loop_count;
		audio_sound_get(src->source, s, loop_count);

		audio_sound_set(dst->source, s, loop_count);
		audio_volume_set(dst->source, volume);
	}
	
	void AudioSourceComponent_serialize(AudioSourceComponent* comp, Serializer& s)
	{
		Sound sound;
		u32 loop_count = 0;
		audio_sound_get(comp->source, sound, loop_count);

		serialize_asset(s, sound);
		serialize_u32(s, loop_count);
	}
	
	void AudioSourceComponent_deserialize(AudioSourceComponent* comp, Deserializer& d, u32 version)
	{
		if (version > 0) {
			
			Sound sound;
			u32 loop_count;
			deserialize_asset(d, sound);
			deserialize_u32(d, loop_count);

			audio_sound_set(comp->source, sound, loop_count);
		}
	}

	void AudioSourceComponent::play()
	{
		audio_play(source);
	}
	void AudioSourceComponent::stop()
	{
		audio_stop(source);
	}
	
}
