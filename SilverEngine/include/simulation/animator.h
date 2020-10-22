#pragma once

#include "core.h"

namespace sv {

	enum AnimationEffect : ui32 {
		AnimationEffect_None,
		AnimationEffect_Linear,
	};

	template<typename T>
	struct KeyFrame {
		T				value;
		float			duration;
		AnimationEffect effect;

		float minVelocity = float_max;
		float maxVelocity = float_max;

		// effect parameters
		union {
		};
	};

	class AnimatedFloat {
	public:
		AnimatedFloat() = default;
		~AnimatedFloat();

		AnimatedFloat(const AnimatedFloat& other);
		AnimatedFloat(AnimatedFloat&& other) noexcept;

		AnimatedFloat& operator=(const AnimatedFloat& other);
		AnimatedFloat& operator=(AnimatedFloat&& other) noexcept;

		void set(float value) noexcept;
		float get() const noexcept;

		void setKeyFrames(const KeyFrame<float>* kfs, ui32 count) noexcept;

		// animation control

		void start();
		void restart();
		void repeatFor(i32 count); // -1 to infinite loop
		void pause();
		void stop();

		// float overriding
		inline void operator=(float value) noexcept { set(value); }
		inline operator float() noexcept { return get(); }

	private:
		void* pInternal = nullptr;

	};

}