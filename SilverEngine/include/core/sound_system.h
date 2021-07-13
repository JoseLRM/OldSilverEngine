#pragma once

#include "platform/audio.h"
#include "core/scene.h"

namespace sv {

	struct SV_API AudioSourceComponent : public Component {

		static constexpr u32 VERSION = 1u;
		
		AudioSource* source;

		void play();
		void stop();
		
	};

	void AudioSourceComponent_create(AudioSourceComponent* comp, Entity entity);
	void AudioSourceComponent_destroy(AudioSourceComponent* comp, Entity entity);
	void AudioSourceComponent_copy(AudioSourceComponent* dst, const AudioSourceComponent* src, Entity entity);
	void AudioSourceComponent_serialize(AudioSourceComponent* comp, Serializer& s);
	void AudioSourceComponent_deserialize(AudioSourceComponent* comp, Deserializer& d, u32 version);

}
