#include "core.h"

#include "scene_internal.h"

#include "physics_impl.h"

namespace sv {

	Result scene_physics_create(const SceneDesc* desc, Scene_internal& scene)
	{
		// BOX 2D
		{
			b2World& world = *new b2World({ desc->gravity.x, desc->gravity.y });
			scene.pWorld2D = &world;
		}
		return Result_Success;
	}

	Result scene_physics_destroy(Scene_internal& scene)
	{
		// BOX 2D
		{
			b2World* pWorld = reinterpret_cast<b2World*>(scene.pWorld2D);
			delete pWorld;
			scene.pWorld2D = nullptr;
		}
		return Result_Success;
	}

	void scene_physics2D_simulate(Scene_internal& scene, float dt)
	{
		b2World& world = *reinterpret_cast<b2World*>(scene.pWorld2D);

		// Adjust Box2D bodies
		{
			EntityView<RigidBody2DComponent> bodies(scene.ecs);
			for (RigidBody2DComponent& body : bodies) {

				// Create body
				if (body.pInternal == nullptr) {
					b2BodyDef def;
					def.type = body.dynamic ? b2_dynamicBody : b2_staticBody;
					def.fixedRotation = body.fixedRotation;
					def.linearVelocity.Set(body.velocity.x, body.velocity.y);
					def.angularVelocity = body.angularVelocity;

					body.pInternal = world.CreateBody(&def);
				}

				b2Body* b2body = reinterpret_cast<b2Body*>(body.pInternal);

				// Create fixtures
				if (body.collidersCount > 0u && body.colliders[body.collidersCount - 1u].pInternal == nullptr) {

					Transform trans = ecs_entity_transform_get(scene.ecs, body.entity);
					vec3f worldScale = trans.GetWorldScale();
					Collider2D* end = body.colliders - 1u;
					Collider2D* it = end + body.collidersCount;
					while (it != end) {

						if (it->pInternal == nullptr) {

							b2FixtureDef def;
							def.density = it->density;
							def.friction = it->friction;
							def.restitution = it->restitution;

							b2PolygonShape pShape;
							b2CircleShape cShape;

							switch (it->type)
							{
							case Collider2DType_Box:
							{
								vec2f scale = worldScale.get_vec2();
								scale *= it->box.size / 2.f;

								pShape.SetAsBox(scale.x, scale.y, { it->offset.x, it->offset.y }, it->box.angularOffset);
								def.shape = &pShape;
							}
								break;

							case Collider2DType_Circle:
							{								
								cShape.m_radius = it->circle.radius * std::max(worldScale.x, worldScale.y);
								cShape.m_p.Set(it->offset.x, it->offset.y);
								def.shape = &cShape;
							}
								break;

							default:
								--it;
								continue;
							}

							it->pInternal = b2body->CreateFixture(&def);
						}
						else break;

						--it;
					}

				}

				if (!b2body->IsAwake()) continue;

				Transform trans = ecs_entity_transform_get(scene.ecs, body.entity);

				vec3f position = trans.GetWorldPosition();
				//TODO: vec3f rotation	= trans.GetWorldRotation();
				const vec3f& rotation = trans.GetLocalRotation();
				vec3f worldScale = trans.GetWorldScale();

				// Update fixtures
				{
					Collider2D* end = body.colliders + body.collidersCount;
					Collider2D* it = body.colliders;
					while (it != end) {

						b2Fixture* fixture = reinterpret_cast<b2Fixture*>(it->pInternal);

						if (fixture == nullptr) {
							++it;
							continue;
						}

						switch (it->type)
						{

						case Collider2DType_Box:
						{
							vec2f scale = worldScale.get_vec2();
							scale *= it->box.size / 2.f;

							b2PolygonShape* shape = reinterpret_cast<b2PolygonShape*>(fixture->GetShape());
							shape->SetAsBox(scale.x, scale.y, { it->offset.x, it->offset.y }, it->box.angularOffset);
						}
							break;

						case Collider2DType_Circle:
						{
							b2CircleShape* shape = reinterpret_cast<b2CircleShape*>(fixture->GetShape());
							shape->m_p.Set(it->offset.x, it->offset.y);
							shape->m_radius = it->circle.radius * std::max(worldScale.x, worldScale.y);
						}
							break;

						}
						
						fixture->SetDensity(it->density);
						fixture->SetFriction(it->friction);
						fixture->SetRestitution(it->restitution);

						++it;
					}
				}

				// Update body
				b2body->SetTransform({ position.x, position.y }, rotation.z);
				b2body->SetType(body.dynamic ? b2_dynamicBody : b2_staticBody);
				b2body->SetFixedRotation(body.fixedRotation);
				b2body->SetLinearVelocity({ body.velocity.x, body.velocity.y });
				b2body->SetAngularVelocity(body.angularVelocity);
			}
		}

		// Simulation
		world.Step(dt * scene.timeStep, 6u, 2u);

		// Get simulation result
		{
			EntityView<RigidBody2DComponent> bodies(scene.ecs);
			for (RigidBody2DComponent& body : bodies) {

				b2Body* b2body = reinterpret_cast<b2Body*>(body.pInternal);

				if (b2body->IsAwake()) {

					const b2Transform& transform = b2body->GetTransform();
					Transform trans = ecs_entity_transform_get(scene.ecs, body.entity);

					if (ecs_entity_parent_get(scene.ecs, body.entity) == SV_ENTITY_NULL) {
						trans.SetPosition({ transform.p.x, transform.p.y, trans.GetLocalPosition().z });
						trans.SetRotation({ trans.GetLocalRotation().x, trans.GetLocalRotation().y, transform.q.GetAngle() });
					}
					else {
						log_error("TODO: Get simulation result from a child");
					}

					const b2Vec2& linearVelocity = b2body->GetLinearVelocity();
					body.velocity = { linearVelocity.x, linearVelocity.y };
					body.angularVelocity = b2body->GetAngularVelocity();
				}
			}
		}
	}

	void scene_physics_simulate(Scene* scene_, float dt)
	{
		parseScene();

		scene_physics2D_simulate(scene, dt);
	}

}