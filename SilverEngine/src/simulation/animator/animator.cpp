#include "core.h"

#include "animator_internal.h"

namespace sv {

#define ASSERT_PTR() SV_ASSERT(pInternal != nullptr)
#define PARSE_ANIMATED(type) sv::AnimatedInstance<type>& anim = *reinterpret_cast<sv::AnimatedInstance<type>*>(pInternal);

	static AnimationAllocator<float> g_FloatAllocator;

	Result animator_initialize()
	{
		animator_allocator_initialize(g_FloatAllocator);

		return Result_Success;
	}

	Result animator_close()
	{
		animator_allocator_close(g_FloatAllocator);

		return Result_Success;
	}

	Result animator_update(float dt)
	{
		// Update floats
		{
			std::vector<AnimationPool<float>>& pools = g_FloatAllocator.pools;

			for (auto& pool : pools) {
				PoolInstance<float>* it = pool.instances + pool.beginCount;
				PoolInstance<float>* end = pool.instances + pool.size;

				while (it != end) {
					ui32 next = it->nextCount;
					animator_effect_update(g_FloatAllocator, it->animation, dt);
					it += next;
				}
			}
		}

		return Result_Success;
	}

	template<typename T>
	void animator_keyframes_set(AnimatedInstance<T>& anim, const KeyFrame<T>* kfs, ui32 count)
	{
		SV_ASSERT(anim.data.get());

		auto& keyFrameList = anim.data->keyFrames;
		keyFrameList.clear();

		for (ui32 i = 0; i < count; ++i) {
			keyFrameList.emplace_back(*kfs);
			++kfs;
		}

		animator_reset(g_FloatAllocator, anim);
	}

	template<typename T>
	inline void animator_internal_check(AnimationAllocator<T>& allocator, void*& pInternal)
	{
		if (pInternal == nullptr) {
			AnimatedInstance<float>* anim = animator_allocator_create(allocator);
			anim->data = std::make_unique<AnimationData<T>>();
			pInternal = anim;
		}
	}

	// FLOAT ANIMATOR	

	AnimatedFloat::~AnimatedFloat()
	{
		if (pInternal) {
			PARSE_ANIMATED(float);
			animator_allocator_destroy(g_FloatAllocator, &anim);
			pInternal = nullptr;
		}
	}

	AnimatedFloat::AnimatedFloat(const AnimatedFloat& other_)
	{
		AnimatedFloat_internal* anim = animator_allocator_create(g_FloatAllocator);
		this->operator=(other_);
		pInternal = anim;
	}

	AnimatedFloat::AnimatedFloat(AnimatedFloat&& other) noexcept
	{
		pInternal = other.pInternal;
		other.pInternal = nullptr;
	}

	AnimatedFloat& AnimatedFloat::operator=(const AnimatedFloat& other_)
	{
		ASSERT_PTR();
		PARSE_ANIMATED(float);
		SV_ASSERT(other_.pInternal);
		AnimatedFloat_internal* other = reinterpret_cast<AnimatedFloat_internal*>(other_.pInternal);
		anim.kf = other->kf;
		anim.value = other->value;
		anim.data = std::make_unique<AnimationData<float>>(*other->data.get());
		return *this;
	}

	AnimatedFloat& AnimatedFloat::operator=(AnimatedFloat&& other) noexcept
	{
		pInternal = other.pInternal;
		other.pInternal = nullptr;
		return *this;
	}

	void AnimatedFloat::set(float value) noexcept
	{
		animator_internal_check(g_FloatAllocator, pInternal);

		ASSERT_PTR();
		PARSE_ANIMATED(float);
		anim.value = value;

		// if has keyframes, recalculate effect and save the current time
		if (anim.data->keyFrames.size()) {
			float currentTime = anim.kf.time;
			animator_effect_init(anim.value, anim.kf, anim.data->keyFrames[anim.data->currentIndex]);
			anim.kf.time = currentTime;
		}
	}

	float AnimatedFloat::get() const noexcept
	{
		if (pInternal) {
			PARSE_ANIMATED(float);
			return anim.value;
		}
		else {
			return 0.f;
		}
	}

	void AnimatedFloat::setKeyFrames(const KeyFrame<float>* kfs, ui32 count) noexcept
	{
		animator_internal_check(g_FloatAllocator, pInternal);

		ASSERT_PTR();
		PARSE_ANIMATED(float);
		animator_keyframes_set(anim, kfs, count);
	}

	void AnimatedFloat::start()
	{
		animator_internal_check(g_FloatAllocator, pInternal);

		ASSERT_PTR();
		PARSE_ANIMATED(float);
		if (animator_is_idle(anim))
			animator_wake(g_FloatAllocator, anim);
	}

	void AnimatedFloat::restart()
	{
		animator_internal_check(g_FloatAllocator, pInternal);

		ASSERT_PTR();
		PARSE_ANIMATED(float);
		if (animator_is_idle(anim)) {
			animator_wake(g_FloatAllocator, anim);
		}
		animator_reset(g_FloatAllocator, anim);
	}

	void AnimatedFloat::repeatFor(i32 count)
	{
		SV_ASSERT(count >= -1);

		if (count == 0) {
			stop();
			return;
		}

		animator_internal_check(g_FloatAllocator, pInternal);

		ASSERT_PTR();
		PARSE_ANIMATED(float);
		anim.data->iterations = count;

		if (animator_is_idle(anim)) {
			animator_wake(g_FloatAllocator, anim);
		}
		animator_reset(g_FloatAllocator, anim);
	}

	void AnimatedFloat::pause()
	{
		animator_internal_check(g_FloatAllocator, pInternal);

		ASSERT_PTR();
		PARSE_ANIMATED(float);
		if (!animator_is_idle(anim))
			animator_idle(g_FloatAllocator, anim);
	}

	void AnimatedFloat::stop()
	{
		animator_internal_check(g_FloatAllocator, pInternal);

		ASSERT_PTR();
		PARSE_ANIMATED(float);
		if (!animator_is_idle(anim)) {
			animator_idle(g_FloatAllocator, anim);
		}
		animator_reset(g_FloatAllocator, anim);
	}

}