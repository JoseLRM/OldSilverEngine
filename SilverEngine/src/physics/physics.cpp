#include "core.h"

#include "physics.h"
#include "..//external/box2d/b2_world.h"
#include "..//external/box2d/b2_body.h"
#include "..//external/box2d/b2_polygon_shape.h"
#include "..//external/box2d/b2_circle_shape.h"
#include "..//external/box2d/b2_fixture.h"

#include "scene/Scene.h"
#include "physics_components.h"

using namespace sv;
using namespace _sv;

namespace _sv {

	static ui32 g_pCurrentSceneID = 0u;

	bool physics_initialize(const SV_PHYSICS_INITIALIZATION_DESC& desc)
	{
		return true;
	}

	bool physics_close()
	{
		return true;
	}

}

namespace sv {

	inline b2World& Parse2D(PhysicsWorld& world) { return *reinterpret_cast<b2World*>(world.pInternal2D); }
	inline b2Body* ParseBody(Body2D body) { return reinterpret_cast<b2Body*>(body); }
	inline b2Fixture* ParseCollider(Collider2D collider) { return reinterpret_cast<b2Fixture*>(collider); }

	////////////////////////////// UPDATE ////////////////////////////////////////

	void physics_update_input_2d(Entity entity, BaseComponent** comp, float dt)
	{
		RigidBody2DComponent& rigidBody = *reinterpret_cast<RigidBody2DComponent*>(comp[0]);
		Transform trans = scene_ecs_entity_get_transform(entity);

		vec3 pos = trans.GetWorldPosition();
		// TODO: World Rotation
		float rot = trans.GetLocalRotation().z;
		vec3 scale = trans.GetWorldScale() / 2.f;

		b2Body* body = ParseBody(rigidBody.GetBody());

		if (trans.GetInternal().wakePhysics) {

			body->SetAwake(true);

			b2Fixture* fixture = ParseCollider(rigidBody.GetCollider());
			if (fixture->GetShape()->GetType() == b2Shape::e_polygon) {
				b2PolygonShape& shape = *reinterpret_cast<b2PolygonShape*>(fixture->GetShape());
				shape.SetAsBox(scale.x, scale.y);
			}

			trans.GetInternal().wakePhysics = false;
		}

		body->SetTransform({ pos.x, pos.y }, rot);

	}

	void physics_update_output_2d(Entity entity, BaseComponent** comp, float dt)
	{
		RigidBody2DComponent& rigidBody = *reinterpret_cast<RigidBody2DComponent*>(comp[0]);
		Transform trans = scene_ecs_entity_get_transform(entity);

		// TODO: Preserve z coord and x - y rotation

		b2Body* body = ParseBody(rigidBody.GetBody());
		const b2Vec2& pos = body->GetPosition();
		float rot = body->GetAngle();

		trans.SetWorldTransformPR({ pos.x, pos.y, 0.f }, { 0.f, 0.f, rot });
	}

	void physics_update(float dt)
	{
		b2World& world = Parse2D(scene_physics_world_get());

		// Update Positions Input
		{
			SV_ECS_SYSTEM_DESC updatePositionsDesc;
			updatePositionsDesc.executionMode = SV_ECS_SYSTEM_EXECUTION_MODE_SAFE;
			updatePositionsDesc.optionalComponentsCount = 0u;
			updatePositionsDesc.requestedComponentsCount = 1u;
			CompID updatePositionsReq[] = { RigidBody2DComponent::ID };
			updatePositionsDesc.pRequestedComponents = updatePositionsReq;
			updatePositionsDesc.system = physics_update_input_2d;

			scene_ecs_system_execute(&updatePositionsDesc, 1u, dt);
		}

		// World Step
		world.Step(dt, 6, 2); //TODO: global variables

		// Update Positions Output
		{
			SV_ECS_SYSTEM_DESC updatePositionsDesc;
			updatePositionsDesc.executionMode = SV_ECS_SYSTEM_EXECUTION_MODE_SAFE;
			updatePositionsDesc.optionalComponentsCount = 0u;
			updatePositionsDesc.requestedComponentsCount = 1u;
			CompID updatePositionsReq[] = { RigidBody2DComponent::ID };
			updatePositionsDesc.pRequestedComponents = updatePositionsReq;
			updatePositionsDesc.system = physics_update_output_2d;

			scene_ecs_system_execute(&updatePositionsDesc, 1u, dt);
		}
	}

	/////////////////////////////// WORLD //////////////////////////////////////

	PhysicsWorld physics_world_create(const SV_PHY_WORLD_DESC* desc)
	{
		b2World* world = new b2World({desc->gravity.x, desc->gravity.y});
		return { world };
	}
	void physics_world_destroy(PhysicsWorld& world_)
	{
		b2World& world = Parse2D(world_);
		delete &world;
	}
	void physics_world_clear(PhysicsWorld& world_)
	{
		b2World& world = Parse2D(world_);
		world.~b2World();
	}

	/////////////////////////////// BODY 2D //////////////////////////////////////

	Body2D physics_2d_body_create(const SV_PHY2D_BODY_DESC* desc, PhysicsWorld& world_)
	{
		b2BodyDef def;
		def.userData = nullptr;
		def.position.Set(desc->position.x, desc->position.y);
		def.angle = desc->rotation;
		def.linearVelocity.Set(desc->velocity.x, desc->velocity.y);
		def.angularVelocity = desc->angularVelocity;
		def.linearDamping = 0.0f;
		def.angularDamping = 0.0f;
		def.allowSleep = true;
		def.awake = true;
		def.fixedRotation = desc->fixedRotation;
		def.bullet = false;
		def.type = desc->dynamic ? b2_dynamicBody : b2_staticBody;
		def.enabled = true;
		def.gravityScale = 1.0f;

		b2World& world = Parse2D(world_);
		b2Body* body = world.CreateBody(&def);

		return body;
	}

	void physics_2d_body_destroy(Body2D body_)
	{
		if (body_ == 0) return;
		b2Body* body = ParseBody(body_);
		b2World* world = body->GetWorld();
		world->DestroyBody(body);
	}

	void physics_2d_body_set_dynamic(bool dynamic, Body2D body_)
	{
		b2Body* body = ParseBody(body_);
		body->SetType(dynamic ? b2_dynamicBody : b2_staticBody);
	}

	bool physics_2d_body_get_dynamic(Body2D body_)
	{
		b2Body* body = ParseBody(body_);
		return body->GetType() == b2_dynamicBody;
	}

	void physics_2d_body_set_fixed_rotation(bool fixedRotation, Body2D body_)
	{
		b2Body* body = ParseBody(body_);
		body->SetFixedRotation(fixedRotation);
	}

	bool physics_2d_body_get_fixed_rotation(Body2D body_)
	{
		b2Body* body = ParseBody(body_);
		return body->IsFixedRotation();
	}

	void physics_2d_body_apply_force(vec2 force, SV_PHY_FORCE_TYPE forceType, Body2D body_)
	{
		b2Body* body = ParseBody(body_);
	}

	// COLLIDERS 2D

	Collider2D physics_2d_collider_create(const SV_PHY2D_COLLIDER_DESC* desc)
	{
		b2CircleShape circleShape;
		b2PolygonShape polygonShape;

		b2FixtureDef fDef;
		fDef.userData = nullptr;
		fDef.friction = 0.2f;
		fDef.restitution = 0.0f;
		fDef.density = desc->density;
		fDef.isSensor = false;

		if (desc->colliderType == SV_PHY2D_COLLIDER_TYPE_BOX) {
			fDef.shape = &polygonShape;

			polygonShape.SetAsBox(desc->box.width, desc->box.height);
		}
		else {
			fDef.shape = &circleShape;

			circleShape.m_radius = desc->circle.radius;
		}

		b2Body* body = ParseBody(desc->body);
		b2Fixture* fixture = body->CreateFixture(&fDef);
		return { fixture };
	}

	void physics_2d_collider_destroy(Collider2D collider)
	{
		if (collider == nullptr) return;
		b2Fixture* fixture = ParseCollider(collider);
		b2Body* body = fixture->GetBody();

		body->DestroyFixture(fixture);
	}

	void physics_2d_collider_set_density(float density, Collider2D collider)
	{
		b2Fixture* fixture = ParseCollider(collider);
		fixture->SetDensity(density);
	}

	float physics_2d_collider_get_density(Collider2D collider)
	{
		b2Fixture* fixture = ParseCollider(collider);
		return fixture->GetDensity();
	}

	SV_PHY2D_COLLIDER_TYPE physics_2d_collider_get_type(Collider2D collider)
	{
		b2Fixture* fixture = ParseCollider(collider);
		return (fixture->GetShape()->GetType() == b2Shape::e_polygon) ? SV_PHY2D_COLLIDER_TYPE_BOX : SV_PHY2D_COLLIDER_TYPE_CIRCLE;
	}

}