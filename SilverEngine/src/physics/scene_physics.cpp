#include "core.h"

#include "scene_physics_internal.h"

#define PARSE_PHYSICS() SV_ASSERT(pInternal); sv::ScenePhysics_internal& phy = *reinterpret_cast<sv::ScenePhysics_internal*>(pInternal);

namespace sv {

	ScenePhysics::~ScenePhysics()
	{
		destroy();
	}

	void ScenePhysics::create(ECS* ecs)
	{
		destroy();
		ScenePhysics_internal* phy = new ScenePhysics_internal();
		pInternal = phy;

		ComponentRegisterDesc desc;

		// RIGID BODY 2D COMPONENT
		{
			desc.name = "RigidBody 2D";
			desc.size = sizeof(RigidBody2DComponent);

			desc.createFn = [this](BaseComponent* comp_)
			{
				new(comp_) RigidBody2DComponent();

				RigidBody2DComponent* comp = reinterpret_cast<RigidBody2DComponent*>(comp_);
				b2World& world = reinterpret_cast<ScenePhysics_internal*>(pInternal)->world2D;

				b2BodyDef def;
				comp->pInternal = world.CreateBody(&def);
			};

			desc.destroyFn = [this](BaseComponent* comp_)
			{
				RigidBody2DComponent* comp = reinterpret_cast<RigidBody2DComponent*>(comp_);
				b2Body* body = reinterpret_cast<b2Body*>(comp->pInternal);
				SV_ASSERT(body);

				b2World& world = reinterpret_cast<ScenePhysics_internal*>(pInternal)->world2D;
				world.DestroyBody(body);
				
				comp->pInternal = nullptr;
				comp->boxInternal = nullptr;
				comp->circleInternal = nullptr;
			};

			desc.copyFn = [this](BaseComponent* from_, BaseComponent* to_)
			{
				b2Body* from = reinterpret_cast<b2Body*>(reinterpret_cast<RigidBody2DComponent*>(from_)->pInternal);
				b2World& world = reinterpret_cast<ScenePhysics_internal*>(pInternal)->world2D;

				b2BodyDef def = scene_physics2D_body_def(from);

				new(to_) RigidBody2DComponent();
				reinterpret_cast<RigidBody2DComponent*>(to_)->pInternal = world.CreateBody(&def);
			};

			desc.moveFn = [this](BaseComponent* from_, BaseComponent* to_)
			{
				RigidBody2DComponent* from = reinterpret_cast<RigidBody2DComponent*>(from_);
				RigidBody2DComponent* to = reinterpret_cast<RigidBody2DComponent*>(to_);
				*to = *from;
				new(from) RigidBody2DComponent();
			};

			desc.serializeFn = [](BaseComponent* comp_, ArchiveO& archive)
			{
				RigidBody2DComponent* comp = reinterpret_cast<RigidBody2DComponent*>(comp_);

				b2BodyDef def = scene_physics2D_body_def(reinterpret_cast<b2Body*>(comp->pInternal));
				archive << def;
			};

			desc.deserializeFn = [this](BaseComponent* comp_, ArchiveI& archive)
			{
				new(comp_) RigidBody2DComponent();
				RigidBody2DComponent* comp = reinterpret_cast<RigidBody2DComponent*>(comp_);

				b2World& world = reinterpret_cast<ScenePhysics_internal*>(pInternal)->world2D;

				b2BodyDef def;
				archive >> def;
				comp->pInternal = world.CreateBody(&def);
			};

			ecs_register(ecs, RigidBody2DComponent::ID, &desc);

			ecs_register<BoxCollider2DComponent>(ecs, "BoxCollider 2D", 
				[](BaseComponent* comp_, ArchiveO& archive) 
			{
				BoxCollider2DComponent* comp = reinterpret_cast<BoxCollider2DComponent*>(comp_);
				archive << comp->density << comp->friction << comp->restitution << comp->size << comp->offset;
			}, 
				[](BaseComponent* comp_, ArchiveI& archive) 
			{
				BoxCollider2DComponent* comp = new(comp_) BoxCollider2DComponent();
				archive >> comp->density >> comp->friction >> comp->restitution >> comp->size >> comp->offset;
			});

			ecs_register<CircleCollider2DComponent>(ecs, "CircleCollider 2D",
				[](BaseComponent* comp_, ArchiveO& archive)
			{
				CircleCollider2DComponent* comp = reinterpret_cast<CircleCollider2DComponent*>(comp_);
				archive << comp->density << comp->friction << comp->restitution << comp->radius << comp->offset;
			},
				[](BaseComponent* comp_, ArchiveI& archive)
			{
				CircleCollider2DComponent* comp = new(comp_) CircleCollider2DComponent();
				archive >> comp->density >> comp->friction >> comp->restitution >> comp->radius >> comp->offset;
			});
		}

	}
	void ScenePhysics::destroy()
	{
		if (pInternal) {
			delete reinterpret_cast<ScenePhysics_internal*>(pInternal);
			pInternal = nullptr;
		}
	}

	void ScenePhysics::setGravity2D(const vec2f& gravity) noexcept
	{
		PARSE_PHYSICS();
		phy.world2D.SetGravity({ gravity.x, gravity.y });
	}

	vec2f ScenePhysics::getGravity2D() const noexcept
	{
		PARSE_PHYSICS();
		b2Vec2 g = phy.world2D.GetGravity();
		return { g.x, g.y };
	}

	void ScenePhysics::simulate(float dt, ECS* ecs)
	{
		PARSE_PHYSICS();

		// SIMULATE 2D
		{
			b2World& world = phy.world2D;

			// Update Body and Fixtures
			{
				EntityView<RigidBody2DComponent> bodies(ecs);

				for (RigidBody2DComponent& comp : bodies) {

					SV_ASSERT(comp.pInternal);
					b2Body* body = reinterpret_cast<b2Body*>(comp.pInternal);

					// Update Transform
					Transform trans = ecs_entity_transform_get(ecs, comp.entity);

					vec3f pos = trans.getWorldPosition();
					// TODO: Compute single euler angle
					vec3f rot = trans.getWorldEulerRotation();

					if (!body->IsAwake()) {

						const b2Vec2& bodyPos = body->GetPosition();

						if (bodyPos.x != pos.x || bodyPos.y != pos.y || body->GetAngle() != rot.z) body->SetAwake(true);

					}
					body->SetTransform({ pos.x, pos.y }, rot.z);

					// Update Box Collider
					BoxCollider2DComponent* boxComp = ecs_component_get<BoxCollider2DComponent>(ecs, comp.entity);
					if (boxComp) {

						if (boxComp->update) {
							boxComp->update = false;

							vec3f scale = trans.getWorldScale();
							body->SetAwake(true);

							// Update fixture
							if (comp.boxInternal) {
								b2Fixture* fixture = reinterpret_cast<b2Fixture*>(comp.boxInternal);

								fixture->SetFriction(boxComp->friction);
								fixture->SetRestitution(boxComp->restitution);
								fixture->SetDensity(boxComp->density);

								b2PolygonShape* shape = reinterpret_cast<b2PolygonShape*>(fixture->GetShape());
								shape->SetAsBox(scale.x * boxComp->size.x, scale.y * boxComp->size.y, { boxComp->offset.x, boxComp->offset.y }, 0.f);
							}
							// Create fixture
							else {
								b2FixtureDef def;
								def.userData = nullptr;
								def.friction = boxComp->friction;
								def.restitution = boxComp->restitution;
								def.density = boxComp->density;
								def.isSensor = false;

								b2PolygonShape shape;
								shape.SetAsBox(scale.x * boxComp->size.x, scale.y * boxComp->size.y, { boxComp->offset.x, boxComp->offset.y }, 0.f);

								def.shape = &shape;
								comp.boxInternal = body->CreateFixture(&def);
							}
						}
					}
					else if (comp.boxInternal) {

						body->DestroyFixture(reinterpret_cast<b2Fixture*>(comp.boxInternal));
						comp.boxInternal = nullptr;

					} 

					// Update Circle Collider
					CircleCollider2DComponent* circleComp = ecs_component_get<CircleCollider2DComponent>(ecs, comp.entity);
					if (circleComp) {

						if (circleComp->update) {
							circleComp->update = false;

							vec3f scale = trans.getWorldScale();
							float radius = std::max(scale.x, scale.y);
							body->SetAwake(true);

							// Update fixture
							if (comp.circleInternal) {
								b2Fixture* fixture = reinterpret_cast<b2Fixture*>(comp.circleInternal);

								fixture->SetFriction(circleComp->friction);
								fixture->SetRestitution(circleComp->restitution);
								fixture->SetDensity(circleComp->density);

								b2CircleShape* shape = reinterpret_cast<b2CircleShape*>(fixture->GetShape());

								shape->m_radius = radius * circleComp->radius;
								shape->m_p.Set(circleComp->offset.x, circleComp->offset.y);
							}
							// Create fixture
							else {
								b2FixtureDef def;
								def.userData = nullptr;
								def.friction = circleComp->friction;
								def.restitution = circleComp->restitution;
								def.density = circleComp->density;
								def.isSensor = false;

								b2CircleShape shape;
								shape.m_radius = radius * circleComp->radius;
								shape.m_p.Set(circleComp->offset.x, circleComp->offset.y);

								def.shape = &shape;
								comp.circleInternal = body->CreateFixture(&def);
							}
						}
					}
					else if (comp.circleInternal) {

						body->DestroyFixture(reinterpret_cast<b2Fixture*>(comp.circleInternal));
						comp.circleInternal = nullptr;

					}
				}
			}

			world.Step(dt, 3, 3);

			// Update sv entities
			{
				EntityView<RigidBody2DComponent> bodies(ecs);

				for (RigidBody2DComponent& comp : bodies) {

					b2Body* body = reinterpret_cast<b2Body*>(comp.pInternal);

					if (body->IsAwake()) {

						Transform trans = ecs_entity_transform_get(ecs, comp.entity);
						vec3f lastPos = trans.getLocalPosition();

						const b2Transform& b2Trans = body->GetTransform();

						trans.setPosition({ b2Trans.p.x, b2Trans.p.y, lastPos.z });
						trans.setEulerRotation({ 0.f, 0.f, b2Trans.q.GetAngle() });
					}
				}
			}
		}
	}

}