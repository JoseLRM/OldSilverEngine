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

		friend class AnimatedUInt;
	private:
		void* pInternal = nullptr;

	};

	class AnimatedUInt {
	public:
		AnimatedUInt() = default;
		~AnimatedUInt() = default;

		AnimatedUInt(const AnimatedUInt& other) : m_Float(other.m_Float) {}
		AnimatedUInt(AnimatedUInt&& other) noexcept : m_Float(other.m_Float) {}

		AnimatedUInt& operator=(const AnimatedUInt& other) { m_Float = other.m_Float; }
		AnimatedUInt& operator=(AnimatedUInt&& other) noexcept { m_Float = std::move(other.m_Float); }

		inline void set(ui32 value) noexcept { m_Float.set(float(value)); }
		inline ui32 get() const noexcept { return ui32(std::round(m_Float.get())); }

		void setKeyFrames(const KeyFrame<ui32>* kfs, ui32 count) noexcept;
		inline void setKeyFrames(const KeyFrame<float>* kfs, ui32 count) noexcept { m_Float.setKeyFrames(kfs, count); }

		// animation control

		inline void start() { m_Float.start(); }
		inline void restart() { m_Float.restart(); }
		inline void repeatFor(i32 count) { m_Float.repeatFor(count); } // -1 to infinite loop
		inline void pause() { m_Float.pause(); }
		inline void stop() { m_Float.stop(); }

		// ui32 overriding
		inline void operator=(ui32 value) noexcept { set(value); }
		inline operator ui32() noexcept { return get(); }

	private:
		AnimatedFloat m_Float;

	};

}