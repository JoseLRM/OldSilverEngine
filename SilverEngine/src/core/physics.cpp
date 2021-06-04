#include "core/scene.h"

namespace sv {

	struct PhysicsState {
		
		List<BodyCollision> last_collisions;
		List<BodyCollision> current_collisions;
		
	};

	static PhysicsState* physics_state = NULL;

	bool physics_initialize()
	{
		physics_state = SV_ALLOCATE_STRUCT(PhysicsState);
		
		return true;
	}

	void physics_close()
	{
		if (physics_state) {
			
			SV_FREE_STRUCT(physics_state);
			physics_state = NULL;
		}
	}

	void physics_update()
    {
		PhysicsState& state = *physics_state;
		SceneData& scene = *get_scene_data();
	
		f32 dt = engine.deltatime;

		ComponentIterator it;
		CompView<BodyComponent> view;

		List<BodyCollision>& last_collisions = state.last_collisions;
		List<BodyCollision>& current_collisions = state.current_collisions;

		last_collisions.reset();
		foreach(i, current_collisions.size())
			last_collisions.push_back(current_collisions[i]);
	
		current_collisions.reset();

		if (comp_it_begin(it, view)) {
	    
			do {

				BodyComponent& body = *view.comp;
		
				if (body.body_type == BodyType_Static)
					continue;

				v2_f32 position = get_entity_position2D(view.entity) + vec3_to_vec2(body.offset);
				v2_f32 scale = get_entity_scale2D(view.entity) * vec3_to_vec2(body.size);

				// Reset values
				body.in_ground = false;
	
				// Gravity
				if (body.body_type == BodyType_Dynamic)
					body.vel += scene.physics.gravity * dt;
	
				CompView<BodyComponent> vertical_collision = {};
				f32 vertical_depth = f32_max;
	
				CompView<BodyComponent> horizontal_collision = {};
				f32 horizontal_depth = f32_max;
				f32 vertical_offset = 0.f; // Used to avoid the small bumps

				v2_f32 next_pos = position;
				v2_f32 next_vel = vec3_to_vec2(body.vel);
	
				// Simulate collisions
				{
					// Detection

					v2_f32 final_pos = position + vec3_to_vec2(body.vel) * dt;

					v2_f32 step = final_pos - next_pos;
					f32 adv = SV_MIN(scale.x, scale.y);
					u32 step_count = SV_MIN(u32(vec2_length(step) / adv) + 1u, 5u);

					step /= f32(step_count);

					foreach(i, step_count) {

						next_pos += step;

						ComponentIterator it0;
						CompView<BodyComponent> v;

						if (comp_it_begin(it0, v)) {
			    
							do {

								BodyComponent& b = *v.comp;

								if (b.body_type == BodyType_Static) {

									v2_f32 p = get_entity_position2D(v.entity) + vec3_to_vec2(b.offset);
									v2_f32 s = get_entity_scale2D(v.entity) * vec3_to_vec2(b.size);

									v2_f32 to = p - next_pos;
									to.x = abs(to.x);
									to.y = abs(to.y);
			
									v2_f32 min_distance = (s + scale) * 0.5f;
									min_distance.x = abs(min_distance.x);
									min_distance.y = abs(min_distance.y);
			
									if (to.x < min_distance.x && to.y < min_distance.y) {

										v2_f32 depth = min_distance - to;

										// Vertical collision
										if (depth.x > depth.y) {

											if (depth.y < abs(vertical_depth)) {
			    
												vertical_collision.comp = &b;
												vertical_collision.entity = v.entity;
												vertical_depth = depth.y * ((next_pos.y < p.y) ? -1.f : 1.f);
											}
										}

										// Horizontal collision
										else {

											if (depth.x < abs(horizontal_depth)) {
				    
												horizontal_collision.comp = &b;
												horizontal_collision.entity = v.entity;
												horizontal_depth = depth.x * ((next_pos.x < p.x) ? -1.f : 1.f);

												if (depth.y < scale.y * 0.05f && next_pos.y > p.y) {
													vertical_offset = depth.y;
												}
											}
										}
									}
								}
							}
							while (comp_it_next(it0, v));
						}
			

						if (vertical_collision.comp || horizontal_collision.comp)
							break;
					}

					// Solve collisions
					if (vertical_collision.comp) {

						if (!(vertical_collision.comp->flags & BodyComponentFlag_Trigger)) {

							if (body.body_type == BodyType_Dynamic) {

								if (vertical_depth > 0.f) {

									body.in_ground = true;
									// Ground friction
									next_vel.x *= (f32)pow(1.f - vertical_collision.comp->friction, dt);
								}

								next_pos.y += vertical_depth;
								next_vel.y = next_vel.y * -body.bounciness;
							}
							else if (body.body_type == BodyType_Projectile) {
								next_pos.y += vertical_depth;
								next_vel.y = -next_vel.y;
							}
						}

						BodyCollision& col = current_collisions.emplace_back();
						col.b0 = view.comp;
						col.b1 = vertical_collision.comp;
						col.e0 = view.entity;
						col.e1 = vertical_collision.entity;
						col.trigger = vertical_collision.comp->flags & BodyComponentFlag_Trigger;
					}

					if (horizontal_collision.comp) {

						if (!(horizontal_collision.comp->flags & BodyComponentFlag_Trigger)) {

							if (body.body_type == BodyType_Dynamic) {
		
								next_pos.x += horizontal_depth;
								if (vertical_offset != 0.f)
									next_pos.y += vertical_offset;
								else
									next_vel.x = next_vel.x * -body.bounciness;

							}
							else if (body.body_type == BodyType_Projectile) {
								next_pos.x += horizontal_depth;
								next_vel.x = -next_vel.x;
							}
						}

						BodyCollision& col = current_collisions.emplace_back();
						col.b0 = view.comp;
						col.b1 = horizontal_collision.comp;
						col.e0 = view.entity;
						col.e1 = horizontal_collision.entity;
						col.trigger = horizontal_collision.comp->flags & BodyComponentFlag_Trigger;
					}

					// Air friction
					if (body.body_type == BodyType_Dynamic)
						next_vel *= (f32)pow(1.f - scene.physics.air_friction, dt);
				}
	
				position = next_pos;
				body.vel = vec2_to_vec3(next_vel, 0.f);

				set_entity_position2D(view.entity, position - vec3_to_vec2(body.offset));
			}
			while (comp_it_next(it, view));

			// Dispatch events
			for (BodyCollision& col : current_collisions) {

				if (col.e1 < col.e0) {
					std::swap(col.e0, col.e1);
					std::swap(col.b0, col.b1);
				}

				CollisionState state = CollisionState_Enter;
		
				for (BodyCollision& c : last_collisions) {
					if (col.e0 == c.e0 && col.e1 == c.e1) {
						state = CollisionState_Stay;
						c.leave = false;
						break;
					}
				}

				if (col.trigger) {

					TriggerCollisionEvent event;
					if (col.b0->flags & BodyComponentFlag_Trigger) {
						event.trigger = CompView<BodyComponent>{ col.e0, col.b0 };
						event.body = CompView<BodyComponent>{ col.e1, col.b1 };
					}
					else {
						event.body = CompView<BodyComponent>{ col.e0, col.b0 };
						event.trigger = CompView<BodyComponent>{ col.e1, col.b1 };
					}
					event.state = state;

					event_dispatch("on_trigger_collision", &event);
				}
				else {
		    
					BodyCollisionEvent event;
					event.body0 = CompView<BodyComponent>{ col.e0, col.b0 };
					event.body1 = CompView<BodyComponent>{ col.e1, col.b1 };
					event.state = state;

					event_dispatch("on_body_collision", &event);
				}

				col.leave = true;
			}
			for (BodyCollision& col : last_collisions) {

				if (col.leave) {

					if (col.trigger) {

						TriggerCollisionEvent event;
						if (col.b0->flags & BodyComponentFlag_Trigger) {
							event.trigger = CompView<BodyComponent>{ col.e0, col.b0 };
							event.body = CompView<BodyComponent>{ col.e1, col.b1 };
						}
						else {
							event.body = CompView<BodyComponent>{ col.e0, col.b0 };
							event.trigger = CompView<BodyComponent>{ col.e1, col.b1 };
						}
						event.state = CollisionState_Leave;

						event_dispatch("on_trigger_collision", &event);
					}
					else {

						BodyCollisionEvent event;
						event.body0 = CompView<BodyComponent>{ col.e0, col.b0 };
						event.body1 = CompView<BodyComponent>{ col.e1, col.b1 };
						event.state = CollisionState_Leave;
		    
						event_dispatch("on_body_collision", &event);
					}
				}
			}
		}
    }
	
}
