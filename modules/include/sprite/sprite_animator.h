#pragma once

#include "core/rendering/material_system.h"

namespace sv {

	struct Sprite {
		TextureAsset texture;
		v4_f32 texCoord = { 0.f, 0.f, 1.f, 1.f };
	};

	struct SpriteAnimation {
		std::vector<Sprite> sprites;
	};

	class SpriteAnimationAsset : public Asset {
	public:
		inline SpriteAnimation* get() const noexcept { return reinterpret_cast<SpriteAnimation*>(m_Ref.get()); }
		inline SpriteAnimation* operator->() const noexcept { return get(); }

		Result createFile(const char* filePath, const SpriteAnimation& animation);
	};

	class AnimatedSprite {
	public:
		struct State {
			// TODO: use the save/load methods form asset system
			size_t animationHashCode;
			float spriteDuration;
			u32 repeatCount;
			u32 index;
			u32 repeatIndex;
			float time;
			bool running;
		};

	public:
		AnimatedSprite() = default;
		~AnimatedSprite();

		AnimatedSprite(const AnimatedSprite& other);
		AnimatedSprite(AnimatedSprite&& other) noexcept;
		
		AnimatedSprite& operator=(const AnimatedSprite& other);
		AnimatedSprite& operator=(AnimatedSprite&& other) noexcept;

		// setters

		void setAnimation(SpriteAnimationAsset& animationAsset);
		void setIndex(u32 index);
		void setRepeatCount(u32 repeatCount); // 0 to infinite
		void setRepeatIndex(u32 repeatIndex);
		void setSpriteDuration(float duration);
		void setSpriteTime(float time);
		Result setState(const State& state);

		// getters

		SpriteAnimationAsset	getAnimation();
		u32					getIndex();
		u32					getRepeatCount();
		u32					getRepeatIndex();
		float					getSpriteDuration();
		float					getSpriteTime();
		Sprite					getSprite();
		bool					isRunning();
		State					getState();

		// state functions

		void start();
		void pause();
		void reset();

		void step(float dt);

	private:
		void create();
		void destroy();

		void* pInternal = nullptr;

	};

}