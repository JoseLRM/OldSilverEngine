#include "core.h"

#include "physics_internal.h"
#include "scene/scene_internal.h"
#include "components.h"
#include "engine.h"

#include "external/box2d/b2_world.h"
#include "external/box2d/b2_body.h"
#include "external/box2d/b2_polygon_shape.h"
#include "external/box2d/b2_circle_shape.h"
#include "external/box2d/b2_fixture.h"

namespace sv {

	static bool g_Enabled = true;

	bool physics_initialize(const InitializationPhysicsDesc& desc)
	{
		return true;
	}

	bool physics_close()
	{
		return true;
	}

	inline b2World& GetWorld2D() { return *reinterpret_cast<b2World*>(scene_physicsWorld_get().pInternal2D); }
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

		EntityTransform& internalTrans = *reinterpret_cast<EntityTransform*>(trans.GetInternal());
		if (internalTrans.wakePhysics) {

			body->SetAwake(true);

			b2Fixture* fixture = ParseCollider(rigidBody.GetCollider());
			if (fixture->GetShape()->GetType() == b2Shape::e_polygon) {
				b2PolygonShape& shape = *reinterpret_cast<b2PolygonShape*>(fixture->GetShape());
				shape.SetAsBox(scale.x, scale.y);
			}

			internalTrans.wakePhysics = false;
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
		if (!g_Enabled) return;

		b2World& world = GetWorld2D();

		//TODO: Use EntityViews

		// Update Positions Input
		{
			SystemDesc updatePositionsDesc;
			updatePositionsDesc.executionMode = SystemExecutionMode_Safe;
			updatePositionsDesc.optionalComponentsCount = 0u;
			updatePositionsDesc.requestedComponentsCount = 1u;
			CompID updatePositionsReq[] = { RigidBody2DComponent::ID };
			updatePositionsDesc.pRequestedComponents = updatePositionsReq;
			updatePositionsDesc.system = physics_update_input_2d;

			scene_ecs_system_execute(&updatePositionsDesc, 1u, dt);
		}

		// World Step
		float timestep = dt * engine_timestep_get();
		world.Step(timestep, 6, 2); //TODO: global variables

		// Update Positions Output
		{
			SystemDesc updatePositionsDesc;
			updatePositionsDesc.executionMode = SystemExecutionMode_Safe;
			updatePositionsDesc.optionalComponentsCount = 0u;
			updatePositionsDesc.requestedComponentsCount = 1u;
			CompID updatePositionsReq[] = { RigidBody2DComponent::ID };
			updatePositionsDesc.pRequestedComponents = updatePositionsReq;
			updatePositionsDesc.system = physics_update_output_2d;

			scene_ecs_system_execute(&updatePositionsDesc, 1u, dt);
		}
	}

	/////////////////////////////// WORLD //////////////////////////////////////

	PhysicsWorld physics_world_create()
	{
		b2World* world = new b2World({ 0.f, 3.f });
		return { world };
	}
	void physics_world_destroy(PhysicsWorld& world_)
	{
		b2World& world = GetWorld2D();
		delete &world;
	}
	void physics_world_clear(PhysicsWorld& world_)
	{
		b2World& world = GetWorld2D();
		world.~b2World();
	}

	/////////////////////////////// BODY 2D //////////////////////////////////////

	void physics_enable_set(bool enabled)
	{
		g_Enabled = enabled;
	}

	bool physics_enable_get()
	{
		return g_Enabled;
	}

	Body2D body2D_create(const Body2DDesc* desc)
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

		b2World& world = GetWorld2D();
		b2Body* body = world.CreateBody(&def);

		return body;
	}

	void body2D_destroy(Body2D body_)
	{
		if (body_ == 0) return;
		b2Body* body = ParseBody(body_);
		b2World* world = body->GetWorld();
		world->DestroyBody(body);
	}

	void body2D_dynamic_set(bool dynamic, Body2D body_)
	{
		b2Body* body = ParseBody(body_);
		body->SetType(dynamic ? b2_dynamicBody : b2_staticBody);
	}

	bool body2D_dynamic_get(Body2D body_)
	{
		b2Body* body = ParseBody(body_);
		return body->GetType() == b2_dynamicBody;
	}

	void body2D_fixedRotation_set(bool fixedRotation, Body2D body_)
	{
		b2Body* body = ParseBody(body_);
		body->SetFixedRotation(fixedRotation);
	}

	bool body2D_fixedRotation_get(Body2D body_)
	{
		b2Body* body = ParseBody(body_);
		return body->IsFixedRotation();
	}

	void body2D_apply_force(vec2 force, ForceType forceType, Body2D body_)
	{
		b2Body* body = ParseBody(body_);
	}

	// COLLIDERS 2D

	Collider2D collider2D_create(const Collider2DDesc* desc)
	{
		b2CircleShape circleShape;
		b2PolygonShape polygonShape;

		b2FixtureDef fDef;
		fDef.userData = nullptr;
		fDef.friction = 0.2f;
		fDef.restitution = 0.0f;
		fDef.density = desc->density;
		fDef.isSensor = false;

		if (desc->colliderType == Collider2DType_Box) {
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

	void collider2D_destroy(Collider2D collider)
	{
		if (collider == nullptr) return;
		b2Fixture* fixture = ParseCollider(collider);
		b2Body* body = fixture->GetBody();

		body->DestroyFixture(fixture);
	}

	void collider2D_density_set(float density, Collider2D collider)
	{
		b2Fixture* fixture = ParseCollider(collider);
		fixture->SetDensity(density);
	}

	float collider2D_density_get(Collider2D collider)
	{
		b2Fixture* fixture = ParseCollider(collider);
		return fixture->GetDensity();
	}

	Collider2DType collider2D_type_get(Collider2D collider)
	{
		b2Fixture* fixture = ParseCollider(collider);
		return (fixture->GetShape()->GetType() == b2Shape::e_polygon) ? Collider2DType_Box : Collider2DType_Circle;
	}

}