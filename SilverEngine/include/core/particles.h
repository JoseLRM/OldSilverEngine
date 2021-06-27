#pragma once

#include "core/scene.h"

namespace sv {

	constexpr u32 PARTICLE_EMITTER_MAX = 10u;

	struct Particle {
		v3_f32 position;
		v3_f32 velocity;
		f32 size;
		
		f32    time_mult;

		// Normalized value, when time_count is 1 the particle should be erased.
		// It's good to interpolate
		f32    time_count;
	};

	enum ParticleShapeType : u32 {
		ParticleShapeType_Sphere,
		ParticleShapeType_Cube,
		ParticleShapeType_Plane,
		ParticleShapeType_Point,
	};

	struct ParticleEmitter {

		~ParticleEmitter();

		u32 max_particles = 1000u;
		u32 _last_max_particles = u32_max;
		Particle* particles = NULL; // TODO: Move on
		u32 particle_count = 0u;
		u32 seed = 6969u; // uwu

		// RENDERING

		TextureAsset texture;
		v4_f32 texcoord = { 0.f, 0.f, 1.f, 1.f };

		// INITIAL VALUES

		v3_f32 min_velocity = { -0.4f, 0.5f, -0.4f };
		v3_f32 max_velocity = { 0.4f, 2.5f, 0.4f };
		f32 min_lifetime = 0.8f;
		f32 max_lifetime = 2.f;
		f32 min_size = 0.1f;
		f32 max_size = 0.3f;

		// GLOBALS
		
		Color init_color = Color::Yellow();
		Color final_color = Color::Red(0);

		f32 gravity_mult = 0.f;

		// SHAPE

		struct {
			
			ParticleShapeType type = ParticleShapeType_Sphere;
			bool fill = true;
			
			struct {
				f32 radius;	
			} sphere;
			
			struct {
				v3_f32 size;
			} cube;

			struct {
				v2_f32 size;
			} plane;
		} shape;

		// EMISSION

		struct {
			f32 rate = 30.f;
			f32 offset_time = 0.f;
			f32 spawn_time = 0.5f;
			f32 count = 0.f;
			f32 spawn_count = 0.f;
		} emission;
		
	};

	struct ParticleSystem : public Component {

		static constexpr u32 VERSION = 0u;

		f32 time_count = 0.f;
		f32 simulation_time = 1.f;
		f32 repeat_time = 1.f;
		bool repeat = true;

		u32 emitter_count = 1u;
		ParticleEmitter emitters[PARTICLE_EMITTER_MAX];

		void serialize(Serializer& s);
		void deserialize(Deserializer& d, u32 version);
		
	};

	void _particle_initialize();
	void _particle_close();
}
