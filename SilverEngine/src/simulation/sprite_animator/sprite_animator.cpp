#include "core.h"

#include "sprite_animator_internal.h"
#include "utils/allocator.h"
#include "utils/io.h"

#define PARSE_ANIMATED_SPRITE() sv::AnimatedSprite_internal& spr = *reinterpret_cast<sv::AnimatedSprite_internal*>(pInternal);
#define ASSERT_PTR() SV_ASSERT(pInternal)

namespace sv {

	IterableInstanceAllocator<AnimatedSprite_internal>	g_AnimatedSprites;
	std::mutex											g_AnimatedSpritesMutex;

	// Main Functions

	Result sprite_animator_initialize()
	{
		// Register SpriteAnimationAsset
		const char* extensions[] = {
			"anim"
		};

		AssetRegisterTypeDesc desc;
		desc.name = "Sprite Animation";
		desc.pExtensions = extensions;
		desc.extensionsCount = 1u;
		desc.createFn = sprite_animation_create;
		desc.destroyFn = sprite_animation_destroy;
		desc.recreateFn = sprite_animation_recreate;
		desc.isUnusedFn = nullptr;
		desc.assetSize = sizeof(SpriteAnimation);
		desc.unusedLifeTime = 5.f;

		svCheck(asset_register_type(&desc, nullptr));

		return Result_Success;
	}

	inline void sprite_animator_update_animation(AnimatedSprite_internal& spr, float dt)
	{
		if (spr.running) {
			SV_ASSERT(spr.animation.hasReference());

			while (dt > 0.f) {

				spr.time += dt;
				if (spr.time >= spr.duration) {

					dt = spr.time - spr.duration;

					// Next sprite
					spr.time = 0.f;

					SpriteAnimation& anim = *spr.animation.get();

					if (++spr.index >= anim.sprites.size()) {

						spr.index = 0u;

						if (spr.repeatCount > 0 && ++spr.repeatIndex >= spr.repeatCount) {
							// Idle the animtion
							spr.repeatIndex = 0u;
							spr.running = false;
						}
					}
				}
				else dt = 0.f;

			}
		}
	}

	void sprite_animator_update(float deltaTime)
	{
		std::scoped_lock lock(g_AnimatedSpritesMutex);

		float dt;

		for (auto& pool : g_AnimatedSprites) {
			for (AnimatedSprite_internal& spr : pool) {
				sprite_animator_update_animation(spr, deltaTime);
			}
		}
	}

	Result sprite_animator_close()
	{
		std::scoped_lock lock(g_AnimatedSpritesMutex);
		g_AnimatedSprites.clear();
		return Result_Success;
	}

	// Animated Sprite

	AnimatedSprite::~AnimatedSprite()
	{
		destroy();
	}

	AnimatedSprite::AnimatedSprite(const AnimatedSprite& other)
	{
		if (other.pInternal == nullptr) return;
		create();
		AnimatedSprite_internal& dst = *reinterpret_cast<AnimatedSprite_internal*>(pInternal);
		const AnimatedSprite_internal& src = *reinterpret_cast<const AnimatedSprite_internal*>(other.pInternal);
		
		dst = src;
	}

	AnimatedSprite::AnimatedSprite(AnimatedSprite&& other) noexcept
	{
		pInternal = other.pInternal;
		other.pInternal = nullptr;
	}

	AnimatedSprite& AnimatedSprite::operator=(const AnimatedSprite& other)
	{
		destroy();
		if (other.pInternal == nullptr) return *this;
		create();
		AnimatedSprite_internal& dst = *reinterpret_cast<AnimatedSprite_internal*>(pInternal);
		const AnimatedSprite_internal& src = *reinterpret_cast<const AnimatedSprite_internal*>(other.pInternal);

		dst = src;
		return *this;
	}

	AnimatedSprite& AnimatedSprite::operator=(AnimatedSprite&& other) noexcept
	{
		destroy();
		pInternal = other.pInternal;
		other.pInternal = nullptr;
		return *this;
	}

	void AnimatedSprite::setAnimation(SpriteAnimationAsset& animationAsset)
	{
		reset();
		PARSE_ANIMATED_SPRITE();
		spr.animation = animationAsset;

		if (animationAsset.hasReference()) {
			// Adjust index
			spr.index = std::min(spr.index, ui32(animationAsset->sprites.size() - 1u));
		}
		// Make idle
		else spr.running = false;
	}

	void AnimatedSprite::setIndex(ui32 index)
	{
		create();
		ASSERT_PTR();
		PARSE_ANIMATED_SPRITE();
		spr.index = index;

		if (spr.animation.hasReference()) {
			// Adjust index
			spr.index = std::min(spr.index, ui32(spr.animation->sprites.size() - 1u));
		}
	}

	void AnimatedSprite::setRepeatCount(i32 repeatCount)
	{
		create();
		ASSERT_PTR();
		PARSE_ANIMATED_SPRITE();
		spr.repeatCount = repeatCount;

		if (spr.repeatCount >= spr.repeatIndex) spr.running = false;
	}

	void AnimatedSprite::setSpriteDuration(float duration)
	{
		create();
		ASSERT_PTR();
		PARSE_ANIMATED_SPRITE();
		spr.duration = std::max(duration, 0.01f);
	}

	void AnimatedSprite::setSpriteTime(float time)
	{
		create();
		ASSERT_PTR();
		PARSE_ANIMATED_SPRITE();
		time = spr.time;
	}

	Result AnimatedSprite::setState(const State& state)
	{
		create();
		ASSERT_PTR();
		PARSE_ANIMATED_SPRITE();

		Result res = Result_Success;
		if (state.animationHashCode) res = spr.animation.load(state.animationHashCode);
		
		spr.duration = state.spriteDuration;
		spr.repeatCount = state.repeatCount;
		spr.index = state.index;
		spr.repeatIndex = state.repeatIndex;
		spr.running = state.running;

		if (!spr.animation.hasReference())
			spr.running = false;

		return res;
	}

	SpriteAnimationAsset AnimatedSprite::getAnimation()
	{
		create();
		ASSERT_PTR();
		PARSE_ANIMATED_SPRITE();
		return spr.animation;
	}

	ui32 AnimatedSprite::getIndex()
	{
		create();
		ASSERT_PTR();
		PARSE_ANIMATED_SPRITE();
		return spr.index;
	}

	i32 AnimatedSprite::getRepeatCount()
	{
		create();
		ASSERT_PTR();
		PARSE_ANIMATED_SPRITE();
		return spr.repeatCount;
	}

	float AnimatedSprite::getSpriteDuration()
	{
		create();
		ASSERT_PTR();
		PARSE_ANIMATED_SPRITE();
		return spr.duration;
	}

	float AnimatedSprite::getSpriteTime()
	{
		create();
		ASSERT_PTR();
		PARSE_ANIMATED_SPRITE();
		return spr.time;
	}

	Sprite AnimatedSprite::getSprite()
	{
		create();
		ASSERT_PTR();
		PARSE_ANIMATED_SPRITE();
		if (spr.animation.hasReference()) {
			return spr.animation->sprites[spr.index];
		}
		return Sprite();
	}

	bool AnimatedSprite::isRunning()
	{
		create();
		ASSERT_PTR();
		PARSE_ANIMATED_SPRITE();
		return spr.running;
	}

	AnimatedSprite::State AnimatedSprite::getState()
	{
		create();
		ASSERT_PTR();
		PARSE_ANIMATED_SPRITE();
		return { spr.animation.getHashCode(), spr.duration, spr.repeatCount, 
			spr.index, spr.repeatIndex, spr.time };
	}

	void AnimatedSprite::start()
	{
		create();
		ASSERT_PTR();
		PARSE_ANIMATED_SPRITE();
		if (spr.animation.hasReference()) {
			spr.running = true;
		}
	}

	void AnimatedSprite::pause()
	{
		create();
		ASSERT_PTR();
		PARSE_ANIMATED_SPRITE();
		spr.running = false;
	}

	void AnimatedSprite::reset()
	{
		create();
		ASSERT_PTR();
		PARSE_ANIMATED_SPRITE();

		spr.index = 0u;
		spr.repeatIndex = 0u;
		spr.time = 0.f;
		spr.running = spr.animation.hasReference() ? spr.running : false;
	}

	void AnimatedSprite::step(float dt)
	{
		create();
		ASSERT_PTR();
		PARSE_ANIMATED_SPRITE();

		sprite_animator_update_animation(spr, dt);
	}

	void AnimatedSprite::create()
	{
		if (pInternal == nullptr) {
			std::scoped_lock lock(g_AnimatedSpritesMutex);
			pInternal = g_AnimatedSprites.create();
		}
	}

	void AnimatedSprite::destroy()
	{
		if (pInternal) {
			std::scoped_lock lock(g_AnimatedSpritesMutex);
			g_AnimatedSprites.destroy(reinterpret_cast<AnimatedSprite_internal*>(pInternal));
			pInternal = nullptr;
		}
	}

	// Sprite Animation Asset

	Result sprite_animation_create(const char* filePath, void* pObject)
	{
		// Create SpriteAnimation
		SpriteAnimation& anim = *new(pObject) SpriteAnimation();

		// Deserialize
		ArchiveI file;
		svCheck(file.open_file(filePath));

		{
			ui32 count;
			file >> count;
			anim.sprites.resize(count);
		}

		for (Sprite& spr : anim.sprites) {
			size_t hashCode;
			file >> hashCode >> spr.texCoord;
			if (hashCode) {
				Result res = spr.texture.load(hashCode);
				if (result_fail(res)) {
					SV_LOG_ERROR("Can't find a texture deserializing SpriteAnimation: '%s'", filePath);
				}
			}
		}

		return Result_Success;
	}

	Result sprite_animation_destroy(void* pObject)
	{
		SpriteAnimation* anim = reinterpret_cast<SpriteAnimation*>(pObject);
		anim->~SpriteAnimation();
		return Result_Success;
	}

	Result sprite_animation_recreate(const char* filePath, void* pObject)
	{
		svCheck(sprite_animation_destroy(pObject));
		return sprite_animation_create(filePath, pObject);
	}

	Result SpriteAnimationAsset::createFile(const char* filePath, const SpriteAnimation& animation)
	{
		ArchiveO file;

		file << ui32(animation.sprites.size());
		for (const Sprite& spr : animation.sprites) {
			file << spr.texture.getHashCode() << spr.texCoord;
		}

		std::string absFilePath = asset_folderpath_get() + filePath;
		return file.save_file(absFilePath.c_str());
	}

	Result SpriteAnimationAsset::serialize()
	{
		if (!hasReference()) return Result_InvalidUsage;

		SpriteAnimation& anim = *get();

		ArchiveO file;
		file << ui32(anim.sprites.size());

		for (Sprite& spr : anim.sprites) {
			file << spr.texture.getHashCode() << spr.texCoord;
		}

		return file.save_file((asset_folderpath_get() + getFilePath()).c_str());
	}

}