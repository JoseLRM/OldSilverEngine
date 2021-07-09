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
	}
	
	void AudioSourceComponent_serialize(AudioSourceComponent* comp, Serializer& s)
	{
	}
	
	void AudioSourceComponent_deserialize(AudioSourceComponent* comp, Deserializer& d, u32 version)
	{
	}
	
}
