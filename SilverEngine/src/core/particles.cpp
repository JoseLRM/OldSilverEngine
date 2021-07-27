#include "core/particles.h"
#include "core/renderer.h"

namespace sv {

	void ParticleSystem::serialize(Serializer& s)
	{
		
	}
	
	void ParticleSystem::deserialize(Deserializer& d, u32 version)
	{
		
	}

	void ParticleSystemModel::serialize(Serializer& s)
	{
		serialize_f32(s, simulation_time);
		serialize_f32(s, repeat_time);
		serialize_bool(s, repeat);
		serialize_u32(s, emitter_count);

		foreach(i, emitter_count) {

			ParticleEmitterModel& e = emitters[i];
			serialize_u32(s, e.max_particles);
			serialize_u32(s, e.seed);
			serialize_u32(s, e.sort_value);
			serialize_bool(s, e.show);
			
			serialize_asset(s, e.texture);
			serialize_v4_f32(s, e.texcoord);

			serialize_v3_f32(s, e.min_rotation);
			serialize_v3_f32(s, e.max_rotation);
			serialize_v3_f32(s, e.min_angular_velocity);
			serialize_v3_f32(s, e.max_angular_velocity);
			serialize_v3_f32(s, e.min_velocity);
			serialize_v3_f32(s, e.max_velocity);
			serialize_f32(s, e.min_lifetime);
			serialize_f32(s, e.max_lifetime);
			serialize_f32(s, e.min_size);
			serialize_f32(s, e.max_size);

			serialize_color(s, e.init_color);
			serialize_color(s, e.final_color);

			serialize_f32(s, e.gravity_mult);
			serialize_bool(s, e.relative_space);

			serialize_u32(s, e.shape.type);
			serialize_bool(s, e.shape.fill);

			switch (e.shape.type) {

			case ParticleShapeType_Sphere:
				serialize_f32(s, e.shape.sphere.radius);
				break;

			case ParticleShapeType_Cube:
				serialize_v3_f32(s, e.shape.cube.size);
				break;

			case ParticleShapeType_Plane:
				serialize_v2_f32(s, e.shape.plane.size);
				break;
				
			}

			serialize_f32(s, e.emission.rate);
			serialize_f32(s, e.emission.offset_time);
			serialize_f32(s, e.emission.spawn_time);
		}
	}
	
	void ParticleSystemModel::deserialize(Deserializer& s, u32 version)
	{
		if (version > 0) {

			deserialize_f32(s, simulation_time);
			deserialize_f32(s, repeat_time);
			deserialize_bool(s, repeat);
			deserialize_u32(s, emitter_count);

			foreach(i, emitter_count) {

				ParticleEmitterModel& e = emitters[i];
				deserialize_u32(s, e.max_particles);
				deserialize_u32(s, e.seed);
				if (version >= 3)
					deserialize_u32(s, e.sort_value);
				if (version >= 4)
					deserialize_bool(s, e.show);
			
				deserialize_asset(s, e.texture);
				deserialize_v4_f32(s, e.texcoord);

				if (version >= 4) {
					deserialize_v3_f32(s, e.min_rotation);
					deserialize_v3_f32(s, e.max_rotation);
					deserialize_v3_f32(s, e.min_angular_velocity);
					deserialize_v3_f32(s, e.max_angular_velocity);
				}
				deserialize_v3_f32(s, e.min_velocity);
				deserialize_v3_f32(s, e.max_velocity);
				deserialize_f32(s, e.min_lifetime);
				deserialize_f32(s, e.max_lifetime);
				deserialize_f32(s, e.min_size);
				deserialize_f32(s, e.max_size);

				deserialize_color(s, e.init_color);
				deserialize_color(s, e.final_color);

				deserialize_f32(s, e.gravity_mult);
				if (version > 1) {
					deserialize_bool(s, e.relative_space);
				}

				deserialize_u32(s, (u32&)e.shape.type);
				deserialize_bool(s, e.shape.fill);

				switch (e.shape.type) {

				case ParticleShapeType_Sphere:
					deserialize_f32(s, e.shape.sphere.radius);
					break;

				case ParticleShapeType_Cube:
					deserialize_v3_f32(s, e.shape.cube.size);
					break;

				case ParticleShapeType_Plane:
					deserialize_v2_f32(s, e.shape.plane.size);
					break;
				
				}

				deserialize_f32(s, e.emission.rate);
				deserialize_f32(s, e.emission.offset_time);
				deserialize_f32(s, e.emission.spawn_time);
			}
		}
	}

	ParticleEmitter::~ParticleEmitter()
	{
		if (particles) {
			SV_FREE_MEMORY(particles);
			particles = NULL;
		}
	}

	SV_AUX void emit_particle(ParticleEmitter& emitter, ParticleEmitterModel& e, v3_f32 system_position)
	{
		Particle& p = emitter.particles[emitter.particle_count++];

		switch (e.shape.type) {

		case ParticleShapeType_Point:
		{
			p.position = {};
		}
		break;

		case ParticleShapeType_Cube:
		{
			// TODO: Fill
			p.position.x = math_random_f32(e.seed++, -0.5f, 0.5f) * e.shape.cube.size.x;
			p.position.y = math_random_f32(e.seed++, -0.5f, 0.5f) * e.shape.cube.size.y; 
			p.position.z = math_random_f32(e.seed++, -0.5f, 0.5f) * e.shape.cube.size.z;
		}
		break;

		case ParticleShapeType_Plane:
		{
			p.position.x = math_random_f32(e.seed++, -0.5f, 0.5f) * e.shape.plane.size.x;
			p.position.y = math_random_f32(e.seed++, -0.5f, 0.5f) * e.shape.plane.size.y;
			p.position.z = 0.f;
		}
		break;

		case ParticleShapeType_Sphere:
		{
			p.position.x = math_random_f32(e.seed++, -0.5f, 0.5f);
			p.position.y = math_random_f32(e.seed++, -0.5f, 0.5f);
			p.position.z = math_random_f32(e.seed++, -0.5f, 0.5f);

			if (e.shape.fill)
				p.position = vec3_normalize(p.position);
			
			p.position *= e.shape.sphere.radius;
		}
		break;
			
		}

		p.position += system_position;

		p.rotation.x = math_random_f32(e.seed++, e.min_rotation.x, e.max_rotation.x);
		p.rotation.y = math_random_f32(e.seed++, e.min_rotation.y, e.max_rotation.y);
		p.rotation.z = math_random_f32(e.seed++, e.min_rotation.z, e.max_rotation.z);

		p.angular_velocity.x = math_random_f32(e.seed++, e.min_angular_velocity.x, e.max_angular_velocity.x);
		p.angular_velocity.y = math_random_f32(e.seed++, e.min_angular_velocity.y, e.max_angular_velocity.y);
		p.angular_velocity.z = math_random_f32(e.seed++, e.min_angular_velocity.z, e.max_angular_velocity.z);

		p.velocity.x = math_random_f32(e.seed++, e.min_velocity.x, e.max_velocity.x);
		p.velocity.y = math_random_f32(e.seed++, e.min_velocity.y, e.max_velocity.y);
		p.velocity.z = math_random_f32(e.seed++, e.min_velocity.z, e.max_velocity.z);

		p.size = math_random_f32(e.seed++, e.min_size, e.max_size);

		f32 lifetime = math_random_f32(e.seed++, e.min_lifetime, e.max_lifetime);
		p.time_mult = 1.f / lifetime;
		p.time_count = 0.f;
	}

	void draw_particles(ParticleSystem& ps, ParticleSystemModel& model, v3_f32 system_position, const XMMATRIX& ivm, const XMMATRIX& vm, const XMMATRIX& pm, CommandList cmd)
	{
		XMMATRIX rm = ivm;
		{
			XMFLOAT3X3 m;
			XMStoreFloat3x3(&m, rm);
			rm = XMLoadFloat3x3(&m);
		}

		f32 dt = engine.deltatime;
		SceneData& scene = *get_scene_data();

		if (model.emitter_count > PARTICLE_EMITTER_MAX) {
			SV_LOG_ERROR("The particle system can't have more than '%u' particle emitters", PARTICLE_EMITTER_MAX);
			model.emitter_count = PARTICLE_EMITTER_MAX;
		}

		u32 emitter_indices[PARTICLE_EMITTER_MAX];

		foreach(i, model.emitter_count) {
			emitter_indices[i] = i;
		}

		std::sort(emitter_indices, emitter_indices + model.emitter_count, [&](u32 i0, u32 i1){

			ParticleEmitterModel& m0 = model.emitters[i0];
			ParticleEmitterModel& m1 = model.emitters[i1];

			return m0.sort_value < m1.sort_value;
		});

		imrend_begin_batch(cmd);

		imrend_camera(vm, pm, cmd);
		
		// TODO: Free unused emitters memory

		bool emit = false;
		bool reset = false;
		bool update = ps.state == ParticleSystemState_Running && ps.last_update_frame != engine.frame_count;

		if (update) {
			
			ps.last_update_frame = engine.frame_count;
		
			ps.time_count += dt;
		
			if (ps.time_count < model.simulation_time) {
				emit = true;
			}
			else {
				emit = false;
			
				f32 total_time = model.simulation_time + model.repeat_time;

				// Reset
				if (model.repeat && ps.time_count > total_time) {
						
					ps.time_count -= total_time;
					reset = true;
				}
				else {
					ps.state = ParticleSystemState_None;
				}
			}
		}

		foreach(i, model.emitter_count) {

			u32 emitter_index = emitter_indices[i];
			ParticleEmitter& e = ps.emitters[emitter_index];
			ParticleEmitterModel& em = model.emitters[emitter_index];

			// Allocate particles memory

			if (update) {
				
				if (e._last_max_particles != em.max_particles) {

					if (e.particles) {
						SV_FREE_MEMORY(e.particles);
					}
					e.particles = (Particle*)SV_ALLOCATE_MEMORY(em.max_particles * sizeof(Particle), "Particle");
					e._last_max_particles = em.max_particles;

					// TODO: Save particle state
					e.particle_count = 0u;
				}
			}

			// Update and erase particles
					
			for (u32 i = 0u; i < e.particle_count;) {

				Particle& p = e.particles[i];

				p.time_count += dt * p.time_mult;

				// Erase
						
				if (p.time_count >= 1.f) {

					if (i != e.particle_count - 1u) {

						// TODO: Optimize								
						memcpy(e.particles + i, e.particles + i + 1u, sizeof(Particle) * (e.particle_count - i - 1u));
					}
							
					--e.particle_count;
				}

				// Update and draw
						
				else {

					p.velocity += scene.physics.gravity * dt * em.gravity_mult;
					// TODO: 3 euler angles in mesh rendering
					p.rotation.z += p.velocity.z * dt;
					p.position += p.velocity * dt;
							
					++i;
				}
			}

			// Draw particles

			if (em.show) {
				
				GPUImage* image = em.texture.get();
			
				foreach(i, e.particle_count) {

					const Particle& p = e.particles[i];
				
					Color color = color_interpolate(em.init_color, em.final_color, p.time_count);

					v3_f32 pos = p.position;
					if (em.relative_space)
						pos += system_position;

					// TODO: For now using only roll rotation for sprites alwais facing to the camera
					XMMATRIX m = XMMatrixRotationRollPitchYaw(0.f, 0.f, p.rotation.z) * rm * XMMatrixTranslation(pos.x, pos.y, pos.z);
					
					imrend_push_matrix(m, cmd);
					imrend_draw_sprite({}, { p.size, p.size }, color, image, GPUImageLayout_ShaderResource, em.texcoord, cmd);
					imrend_pop_matrix(cmd);
				}
			}
				
			// Reset
			if (reset) {
				
				e.emission.count = 0.f;
				e.emission.spawn_count = 0.f;
				emit = true;
			}
			
			// Emit
			if (emit && e.particle_count < em.max_particles) {
				
				e.emission.count += dt;

				v3_f32 emit_position = em.relative_space ? v3_f32{} : system_position;
				
				if (e.emission.count > em.emission.offset_time) {

					f32 end_time = em.emission.offset_time + em.emission.spawn_time;

					if (e.emission.count < end_time) {

						e.emission.spawn_count += dt;
								
						f32 frq = 1.f / em.emission.rate;

						while (e.emission.spawn_count > frq) {
							
							emit_particle(e, em, emit_position);

							e.emission.spawn_count -= frq;

							if (e.particle_count == em.max_particles)
								break;
						}
					}
				}
			}
			
			imrend_flush(cmd);
		}
	}

#if SV_EDITOR

	void display_particle_system_data(DisplayComponentEvent* event)
	{
		if (event->comp_id == get_component_id("Particle System")) {
			
			ParticleSystem& p = *(ParticleSystem*)event->comp;

			static u32 show_emitter_index = 0u;

			gui_drag_u32("Layer", p.layer, 1u, 0u, RENDER_LAYER_COUNT);
		}
		else if (event->comp_id == get_component_id("Particle System Model")) {
			
			ParticleSystemModel& p = *(ParticleSystemModel*)event->comp;

			static u32 show_emitter_index = 0u;

			gui_drag_f32("Simulation Time", p.simulation_time, 0.01f, 0.f, f32_max);
			if (p.repeat)
				gui_drag_f32("Repeat Time", p.repeat_time, 0.01f, 0.f, f32_max);
			gui_checkbox("Repeat", p.repeat);
			gui_drag_u32("Emitter Count", p.emitter_count, 1u, 0u, PARTICLE_EMITTER_MAX);

			if (p.emitter_count) {
				
				gui_drag_u32("Show Emitter", show_emitter_index, 1u, 0u, p.emitter_count - 1u);

				gui_separator(1);

				ParticleEmitterModel& e = p.emitters[show_emitter_index];

				gui_checkbox("Show", e.show);
				gui_drag_u32("Sort Value", e.sort_value, 1u, 0u, u32_max);
				gui_texture_asset("Texture", e.texture);
				gui_drag_v4_f32("Texcoord", e.texcoord, 0.001f, 0.f, 1.f);

				gui_drag_u32("Max Particles", e.max_particles, 1u, 0u, 100000u);

				v3_f32 min_rotation = ToDegrees(e.min_rotation);
				v3_f32 max_rotation = ToDegrees(e.max_rotation);
				v3_f32 min_angular_velocity = ToDegrees(e.min_angular_velocity);
				v3_f32 max_angular_velocity = ToDegrees(e.max_angular_velocity);
				
				gui_drag_v3_f32("Min Rotation", min_rotation, 0.1f, -f32_max, f32_max);
				gui_drag_v3_f32("Max Rotation", max_rotation, 0.1f, -f32_max, f32_max);
				gui_drag_v3_f32("Min Angular Velocity", min_angular_velocity, 0.1f, -f32_max, f32_max);
				gui_drag_v3_f32("Max Angular Velocity", max_angular_velocity, 0.1f, -f32_max, f32_max);

				e.min_rotation = ToRadians(min_rotation);
				e.max_rotation = ToRadians(max_rotation);
				e.min_angular_velocity = ToRadians(min_angular_velocity);
				e.max_angular_velocity = ToRadians(max_angular_velocity);
				
				gui_drag_v3_f32("Min Velocity", e.min_velocity, 0.01f, -f32_max, f32_max);
				gui_drag_v3_f32("Max Velocity", e.max_velocity, 0.01f, -f32_max, f32_max);
				gui_drag_f32("Min Lifetime", e.min_lifetime, 0.01f, 0.f, f32_max);
				gui_drag_f32("Max Lifetime", e.max_lifetime, 0.01f, 0.f, f32_max);
				gui_drag_f32("Min Size", e.min_size, 0.01f, 0.f, f32_max);
				gui_drag_f32("Max Size", e.max_size, 0.01f, 0.f, f32_max);
				gui_drag_color("Init color", e.init_color);
				gui_drag_color("Final color", e.final_color);
				gui_drag_f32("Gravity Mult", e.gravity_mult, 0.001f, -f32_max, f32_max);
				gui_checkbox("Relative Space", e.relative_space);

				gui_separator(1);

				const char* preview = "";
				switch (e.shape.type) {
					
				case ParticleShapeType_Point:
					preview = "Point";
					break;
					
				case ParticleShapeType_Cube:
					preview = "Cube";
					break;
					
				case ParticleShapeType_Sphere:
					preview = "Sphere";
					break;

				case ParticleShapeType_Plane:
					preview = "Plane";
					break;
					
				}

				if (gui_begin_combobox(preview, 345345)) {

					if (e.shape.type != ParticleShapeType_Point && gui_button("Point")) {
						e.shape.type = ParticleShapeType_Point;
					}

					if (e.shape.type != ParticleShapeType_Sphere && gui_button("Sphere")) {
						e.shape.type = ParticleShapeType_Sphere;
						e.shape.sphere.radius = 1.f;
					}

					if (e.shape.type != ParticleShapeType_Cube && gui_button("Cube")) {
						e.shape.type = ParticleShapeType_Cube;
						e.shape.cube.size.x = 1.f;
						e.shape.cube.size.y = 1.f;
						e.shape.cube.size.z = 1.f;
					}
					
					if (e.shape.type != ParticleShapeType_Plane && gui_button("Plane")) {
						e.shape.type = ParticleShapeType_Plane;
						e.shape.plane.size.x = 1.f;
						e.shape.plane.size.y = 1.f;
					}
					
					gui_end_combobox();
				}

				gui_checkbox("Fill", e.shape.fill);
				
				switch (e.shape.type) {

				case ParticleShapeType_Sphere:
					gui_drag_f32("Sphere", e.shape.sphere.radius, 0.01f, 0.f, f32_max);
					break;

				case ParticleShapeType_Cube:
					gui_drag_v3_f32("Size", e.shape.cube.size, 0.01f, 0.f, f32_max);
					break;

				case ParticleShapeType_Plane:
					gui_drag_v2_f32("Size", e.shape.plane.size, 0.01f, 0.f, f32_max);
					break;
					
				}

				gui_drag_f32("Emission Rate", e.emission.rate, 0.1f, 0.f, f32_max);
				gui_drag_f32("Emission Offset Time", e.emission.offset_time, 0.01f, 0.f, f32_max);
				gui_drag_f32("Emission Spawn Time", e.emission.spawn_time, 0.01f, 0.f, f32_max);
			}
		}
	}
	
#endif

	void _particle_initialize()
	{
#if SV_EDITOR
		event_register("display_component_data", display_particle_system_data, 0);
#endif
	}
	
	void _particle_close()
	{
		
	}
	
}
