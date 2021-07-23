#pragma once

#include "core/scene.h"

namespace sv {

	constexpr u32 PARTICLE_EMITTER_MAX = 10u;

	struct Particle {
		v3_f32 position;
		v3_f32 rotation;
		v3_f32 angular_velocity;
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

	struct ParticleEmitterModel {

		u32 max_particles = 1000u;
		u32 seed = 6969u; // uwu
		u32 sort_value = 0u;
		bool show = true;

		// RENDERING

		TextureAsset texture;
		v4_f32 texcoord = { 0.f, 0.f, 1.f, 1.f };

		// INITIAL VALUES

		v3_f32 min_rotation = { 0.f };
		v3_f32 max_rotation = { PI * 2.f };
		v3_f32 min_angular_velocity = { -0.4f, 0.5f, -0.4f };
		v3_f32 max_angular_velocity = { 0.4f, 2.5f, 0.4f };
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
		bool relative_space = false;

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
		} emission;
		
	};

	struct ParticleSystemModel : public Component {

		static constexpr u32 VERSION = 4u;
		
		f32 simulation_time = 1.f;
		f32 repeat_time = 1.f;
		bool repeat = true;

		u32 emitter_count = 1u;
		ParticleEmitterModel emitters[PARTICLE_EMITTER_MAX];

		void serialize(Serializer& s);
		void deserialize(Deserializer& d, u32 version);
		
	};

	struct ParticleEmitter {

		~ParticleEmitter();

		u32 _last_max_particles = u32_max;
		Particle* particles = NULL; // TODO: Move on
		u32 particle_count = 0u;

		struct {
			f32 count = 0.f;
			f32 spawn_count = 0.f;
		} emission;
		
	};

	enum ParticleSystemState : u32 {
		ParticleSystemState_None,
		ParticleSystemState_Running,
		ParticleSystemState_Paused,
	};

	struct ParticleSystem : public Component {

		static constexpr u32 VERSION = 0u;

		ParticleSystemState state = ParticleSystemState_None;
		u32 layer = RENDER_LAYER_COUNT / 2u;

		u64 last_update_frame = u32_max;
		f32 time_count = 0.f;

		ParticleEmitter emitters[PARTICLE_EMITTER_MAX];

		void serialize(Serializer& s);
		void deserialize(Deserializer& d, u32 version);

		inline void reset() {

			state = ParticleSystemState_None;
			time_count = 0.f;

			foreach(i, PARTICLE_EMITTER_MAX) {
				emitters[i].emission.count = 0.f;
				emitters[i].emission.spawn_count = 0.f;
			}
		}

		inline void run() {
			state = ParticleSystemState_Running;
		}

		inline void pause() {

			if (state == ParticleSystemState_Running) {
				state = ParticleSystemState_Paused;
			}
		}

		inline bool is_running() {
			return state == ParticleSystemState_Running;
		}

	};

	SV_API void draw_particles(ParticleSystem& particles, ParticleSystemModel& model, v3_f32 position, const XMMATRIX& ivm, const XMMATRIX& vm, const XMMATRIX& pm, CommandList cmd);

	inline Entity create_particles(const char* filepath, u32 layer = RENDER_LAYER_COUNT / 2)
	{
		Entity p = load_prefab(filepath);

		if (p) {

			Entity entity = create_entity(0, NULL, p);

			if (entity) {

				// TODO
				CompID comp_id = get_component_id("Particle System");
				ParticleSystem* ps = (ParticleSystem*)add_entity_component(entity, comp_id);
				if (ps) {
					ps->run();
					ps->layer = layer;
				}
				else return 0;
			}

			return entity;
		}

		return 0;
	}

	void _particle_initialize();
	void _particle_close();
}
