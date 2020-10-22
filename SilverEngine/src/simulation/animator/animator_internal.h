#pragma once

#include "simulation/animator.h"

namespace sv {

	constexpr ui32 ANIMATION_POOL_SIZE = 100u;

	// Main functions

	Result animator_initialize();
	Result animator_close();
	Result animator_update(float dt);

	// ANIMATION STRUCTS

	template<typename T>
	struct AnimationData {
		ui32 currentIteration = 0u;
		ui32 iterations = ui32_max;
		ui32 currentIndex = 0u;
		std::vector<KeyFrame<T>> keyFrames;
	};

	template<typename T>
	struct KeyFrame_internal {

		AnimationEffect effect;
		T target;
		float time;
		float duration;

		union {
			struct {
				T velocity; // Per second
			} linear;
		};

	};

	template<typename T>
	struct AnimatedInstance {

		std::unique_ptr<AnimationData<T>> data;
		T value;
		KeyFrame_internal<T> kf;

	};

	typedef AnimatedInstance<float> AnimatedFloat_internal;

	// ALLOCATOR STRUCTS

	template<typename T>
	struct PoolInstance {
		AnimatedInstance<T> animation;
		ui32 nextCount = 0u;
	};

	template<typename T>
	struct AnimationPool {
		PoolInstance<T>* instances = nullptr;
		PoolInstance<T>* freeList = nullptr;
		ui32 size = 0u;
		ui32 beginCount = 0u;
	};

	template<typename T>
	struct AnimationAllocator {
		std::vector<AnimationPool<T>> pools;
		std::mutex mutex;
	};

	// Effects

	template<typename T>
	void animator_effect_init(T actualValue, KeyFrame_internal<T>& kf, const KeyFrame<T>& keyFrame)
	{
		kf.time = 0.f;
		kf.target = keyFrame.value;
		kf.duration = keyFrame.duration;
		kf.effect = keyFrame.effect;

		switch (kf.effect)
		{
		case AnimationEffect_Linear:
		{
			T distance = kf.target - actualValue;
			kf.linear.velocity = distance / kf.duration;

			if (keyFrame.minVelocity != float_max && kf.linear.velocity < keyFrame.minVelocity) {
				kf.linear.velocity = keyFrame.minVelocity;
			}
			else if (keyFrame.maxVelocity != float_max && kf.linear.velocity > keyFrame.maxVelocity) {
				kf.linear.velocity = keyFrame.maxVelocity;
			}
			else break;

			kf.duration = distance / kf.linear.velocity;
		}
			break;

		case AnimationEffect_None:
			kf.target = actualValue;
			break;
		}
	}

	template<typename T>
	inline void animator_effect_update(AnimationAllocator<T>& allocator, AnimatedInstance<T>& animation, float deltaTime)
	{
		KeyFrame_internal<T>& kf = animation.kf;
		float dt;

		while (deltaTime) {

			dt = deltaTime;

			// Adjust deltatime
			if (kf.time + dt > kf.duration) dt = kf.duration - kf.time;

			// Aply effect
			switch (kf.effect)
			{
			case AnimationEffect_Linear:
				animator_effect_linear_update(animation, dt);
				break;
			}

			// Next keyframe ?
			deltaTime -= dt;
			kf.time += dt;
			if (kf.time >= kf.duration) {

				// Set exact target
				animation.value = kf.target;

				// TODO: different configurations
				auto& keyframes = animation.data->keyFrames;
				if (keyframes.empty()) {
					animator_idle(allocator, animation);
					deltaTime = 0.f;
				}
				else {				

					animation.data->currentIndex = (size_t(animation.data->currentIndex) + 1u) % keyframes.size();

					KeyFrame<T>& nextKF = keyframes[animation.data->currentIndex];
					animator_effect_init(animation.value, animation.kf, nextKF);

					// check iterations and next keyframe
					if (animation.data->currentIndex == 0u && animation.data->iterations != ui32_max) {
						if (++animation.data->currentIteration == animation.data->iterations) {
							animator_idle(allocator, animation);
							deltaTime = 0.f;
						}
					}
				}
			}
		}
	}

	template<typename T>
	inline void animator_effect_linear_update(AnimatedInstance<T>& animation, float dt)
	{
		animation.value += animation.kf.linear.velocity * dt;
	}

	// State

	template<typename T>
	bool animator_is_idle(AnimatedInstance<T>& animation)
	{
		PoolInstance<T>* instance = reinterpret_cast<PoolInstance<T>*>(&animation);
		return instance->nextCount == 0u;
	}

	template<typename T>
	void animator_wake(AnimationAllocator<T>& allocator, AnimatedInstance<T>& animation)
	{
		PoolInstance<T>* instance = reinterpret_cast<PoolInstance<T>*>(&animation);
		AnimationPool<T>* pool = animator_allocator_pool_find(allocator, instance);
		SV_ASSERT(pool);
		std::scoped_lock<std::mutex> lock(allocator.mutex);
		animator_allocator_wake(allocator, *pool, instance);
	}

	template<typename T>
	void animator_idle(AnimationAllocator<T>& allocator, AnimatedInstance<T>& animation)
	{
		PoolInstance<T>* instance = reinterpret_cast<PoolInstance<T>*>(&animation);
		AnimationPool<T>* pool = animator_allocator_pool_find(allocator, instance);
		SV_ASSERT(pool);
		std::scoped_lock<std::mutex> lock(allocator.mutex);
		animator_allocator_idle(allocator, *pool, instance);
	}

	template<typename T>
	void animator_reset(AnimationAllocator<T>& allocator, AnimatedInstance<T>& anim)
	{
		if (anim.data->keyFrames.empty()) {
			if (!animator_is_idle(anim)) animator_idle(allocator, anim);
			return;
		}

		anim.data->currentIteration = 0u;
		anim.data->currentIndex = 0u;
		animator_effect_init(anim.value, anim.kf, anim.data->keyFrames[anim.data->currentIndex]);
	}

	// Allocator

	template<typename T>
	void animator_allocator_initialize(AnimationAllocator<T>& allocator)
	{
		allocator.pools.emplace_back();
	}

	template<typename T>
	void animator_allocator_close(AnimationAllocator<T>& allocator)
	{
		for (auto& pool : allocator.pools) {
			if (pool.instances == nullptr) continue;
			
			PoolInstance<T>* it = pool.instances;
			PoolInstance<T>* end = pool.instances + pool.size;

			while (it != end) {
				if (it->nextCount != ui32_max) {
					it->animation.~AnimatedInstance<T>();
				}
				++it;
			}

			operator delete[](pool.instances);
		}
		allocator.pools.clear();
	}

	template<typename T>
	AnimatedInstance<T>* animator_allocator_create(AnimationAllocator<T>& allocator)
	{
		std::scoped_lock<std::mutex> lock(allocator.mutex);

		auto& pools = allocator.pools;
		SV_ASSERT(pools.size());

		if (pools.back().freeList == nullptr && pools.back().size == ANIMATION_POOL_SIZE) {

			ui32 nextPoolIndex = ui32_max;

			// Find pool with available freelist
			for (ui32 i = 0; i < pools.size(); ++i) {
				if (pools[i].freeList) {
					nextPoolIndex = i;
					goto swap;
				}
			}

			// Find pool with available size
			for (ui32 i = 0; i < pools.size(); ++i) {
				if (pools[i].size < ANIMATION_POOL_SIZE) {
					nextPoolIndex = i;
					goto swap;
				}
			}

		swap:
			// if not found create new one
			if (nextPoolIndex == ui32_max) {
				pools.emplace_back();
			}
			// else swap pools
			else {
				AnimationPool<T> aux = pools.back();
				pools.back() = pools[nextPoolIndex];
				pools[nextPoolIndex] = aux;
			}
		}

		AnimationPool<T>& pool = pools.back();
		PoolInstance<T>* res = nullptr;
		bool resized = false;

		if (pool.freeList) {
			res = pool.freeList;
			pool.freeList = *reinterpret_cast<PoolInstance<T>**>(res);
		}
		else if (pool.instances == nullptr) {
			pool.instances = (PoolInstance<T>*)operator new(ANIMATION_POOL_SIZE * sizeof(PoolInstance<T>));
			pool.size = 0u;
			pool.beginCount = 0u;
			pool.freeList = nullptr;
		}

		if (res == nullptr) {
			resized = true;
			res = pool.instances + pool.size++;
		}

		new(res) PoolInstance<T>();
		
		// Change next count
		if (resized) {
			if (res == pool.instances) {
				++pool.beginCount;
			}
			else {

				PoolInstance<T>* it = res - 1u;
				PoolInstance<T>* begin = pool.instances - 1u;

				while (it != begin) {

					if (it->nextCount != ui32_max && it->nextCount != 0u) {
						break;
					}

					--it;
				}

				if (it == begin) ++pool.beginCount;
				else ++it->nextCount;

			}
		}
		
		return &res->animation;
	}

	template<typename T>
	void animator_allocator_destroy(AnimationAllocator<T>& allocator, AnimatedInstance<T>* animation)
	{
		if (allocator.pools.empty()) return;

		animation->~AnimatedInstance<T>();
		PoolInstance<T>* ptr = reinterpret_cast<PoolInstance<T>*>(animation);

		std::scoped_lock<std::mutex> lock(allocator.mutex);

		AnimationPool<T>* pPool = animator_allocator_pool_find(allocator, ptr);

		SV_ASSERT(pPool);

		// Change the next count
		if (ptr->nextCount != 0u)
			animator_allocator_idle(allocator, *pPool, ptr);
		ptr->nextCount = ui32_max; // That means it is invalid

		// Remove from pool
		memcpy(ptr, &pPool->freeList, sizeof(void*));
		pPool->freeList = ptr;

		// Destroy empty pool
		if (pPool->beginCount == pPool->size) {

			// free memory
			SV_ASSERT(pPool->instances);
			operator delete[](pPool->instances);

			if (allocator.pools.size() == 1u) {
				pPool->beginCount = 0u;
				pPool->freeList = nullptr;
				pPool->instances = nullptr;
				pPool->size = 0u;
			}
			else {
				// find the position
				for (auto it = allocator.pools.begin(); it != allocator.pools.end(); ++it) {
					if (&(*it) == pPool) {
						allocator.pools.erase(it);
						break;
					}
				}
			}
		}
	}

	template<typename T>
	AnimationPool<T>* animator_allocator_pool_find(AnimationAllocator<T>& allocator, PoolInstance<T>* ptr)
	{
		AnimationPool<T>* pPool = nullptr;

		for (auto& pool : allocator.pools) {
			if (pool.instances <= ptr && pool.instances + pool.size > ptr) {
				pPool = &pool;
				break;
			}
		}

		return pPool;
	}

	template<typename T>
	void animator_allocator_wake(AnimationAllocator<T>& allocator, AnimationPool<T>& pool, PoolInstance<T>* ptr)
	{
		SV_ASSERT(ptr->nextCount == 0u);
		
		if (pool.instances == ptr) {
			SV_ASSERT(pool.beginCount != 0u);
			ptr->nextCount = pool.beginCount;
			pool.beginCount = 0u;
		}
		else {

			PoolInstance<T>* begin = pool.instances - 1u;
			PoolInstance<T>* it = ptr - 1u;

			// Find active animation
			while (it != begin) {

				if (it->nextCount != ui32_max && it->nextCount != 0u) {
					break;
				}

				--it;
			}

			ui32& otherCount = (begin == it) ? pool.beginCount : it->nextCount;
			ui32 distance = ptr - it;
			SV_ASSERT(otherCount >= distance);

			ptr->nextCount = otherCount - distance;
			otherCount = distance;
		}
	}

	template<typename T>
	void animator_allocator_idle(AnimationAllocator<T>& allocator, AnimationPool<T>& pool, PoolInstance<T>* ptr)
	{
		SV_ASSERT(ptr->nextCount != ui32_max && ptr->nextCount != 0u);

		if (pool.instances == ptr) {
			pool.beginCount = ptr->nextCount;
		}
		else {

			PoolInstance<T>* begin = pool.instances - 1u;
			PoolInstance<T>* it = ptr - 1u;

			// Find active animation
			while (it != begin) {

				if (it->nextCount != 0u && it->nextCount != ui32_max) {
					break;
				}

				--it;
			}

			if (begin == it) {
				pool.beginCount += ptr->nextCount;
			}
			else it->nextCount += ptr->nextCount;
		}

		ptr->nextCount = 0u;
	}

}