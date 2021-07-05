#include "core/physics3D.h"

#include "PxPhysicsAPI.h"

#define SV_PHYSX() PxPhysics* p = physics->engine; PxScene* scene = physics->scene

using namespace physx;

namespace sv {

	SV_AUX PxVec3 vec3_to_px(v3_f32 v)
	{
		return PxVec3(v.x, v.y, v.z);
	}
	SV_AUX v3_f32 px_to_vec3(PxVec3 v)
	{
		return v3_f32(v.x, v.y, v.z);
	}

	struct PhysxErrorCallback : physx::PxErrorCallback {

		void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line) override {
			SV_LOG_ERROR("[PHYSX] %s: code %u", message, code);
		}
		
	};

	struct PhysxAllocatorCallback : physx::PxAllocatorCallback {

		void* allocate(size_t size, const char* typeName, const char* filename, int line) override {
			// TODO
			return SV_ALLOCATE_MEMORY(size, "Physx");
		}
		void deallocate(void* ptr) override {
			return SV_FREE_MEMORY(ptr);
		}
		
	};

	struct PhysxSimulationCallback : public PxSimulationEventCallback {

		void onConstraintBreak(PxConstraintInfo *constraints, PxU32 count) override {}
		void onWake(PxActor **actors, PxU32 count) override {}
		void onSleep(PxActor **actors, PxU32 count) override {}
			
		
		void onContact(const PxContactPairHeader &pairHeader, const PxContactPair *pairs, PxU32 nbPairs) override {

			foreach(i, nbPairs) {

				const PxContactPair& pair = pairs[i];

				if (pair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND) {

					PxActor* a0 = pairHeader.actors[0];
					PxActor* a1 = pairHeader.actors[1];

					BodyComponent* b0 = (BodyComponent*)a0->userData;
					BodyComponent* b1 = (BodyComponent*)a1->userData;

					OnBodyCollisionEvent e;
					e.body0 = b0;
					e.body1 = b1;

					// TODO:
					e.entity0 = (Entity)e.body0->id;
					e.entity1 = (Entity)e.body1->id;
					
					event_dispatch("on_body_collision", &e);
				}
			}
		}

		void onTrigger(PxTriggerPair *pairs, PxU32 count) override {
			SV_LOG_INFO("Trigger");
		}

		void onAdvance(const PxRigidBody *const *bodyBuffer, const PxTransform *poseBuffer, const PxU32 count) override {
			SV_LOG_INFO("Advance");
		}
		
	};

	physx::PxFilterFlags contactReportFilterShader(physx::PxFilterObjectAttributes attributes0,
												   physx::PxFilterData filterData0,
												   physx::PxFilterObjectAttributes attributes1,
												   physx::PxFilterData filterData1,
												   physx::PxPairFlags& pairFlags,
												   const void* constantBlock,
												   physx::PxU32 constantBlockSize)
	{
		if(PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1)) {
			pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
			return PxFilterFlag::eDEFAULT;
		}
		
		pairFlags = PxPairFlag::eCONTACT_DEFAULT;
		pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;

		return PxFilterFlag::eDEFAULT;
	}

	struct Physics3DData {
		PhysxErrorCallback error_callback;
		PhysxAllocatorCallback allocator;
		PhysxSimulationCallback simulation_callback;

		PxFoundation* foundation = NULL;
		PxPhysics* engine = NULL;
		PxScene* scene = NULL;

		PxMaterial* default_material = NULL;
	};

	static Physics3DData* physics = NULL;
	
	bool _physics3D_initialize()
	{
		physics = SV_ALLOCATE_STRUCT(Physics3DData, "Physx");
		
		physics->foundation = PxCreateFoundation(PX_PHYSICS_VERSION, physics->allocator, physics->error_callback);

		if (!physics->foundation) {
			SV_LOG_ERROR("Can't create physx foundation");
			_physics3D_close();
			return false;
		}

		//auto mPvd = PxCreatePvd(*physics->foundation);
		//PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
		//mPvd->connect(*transport,PxPvdInstrumentationFlag::eALL);
		

		physics->engine = PxCreatePhysics(PX_PHYSICS_VERSION, *physics->foundation, PxTolerancesScale(), true, NULL);
		if(!physics->engine) {
			SV_LOG_ERROR("Can't initialize physx engine");
			_physics3D_close();
			return false;
		}

		PxPhysics* p = physics->engine;

		PxSceneDesc desc(p->getTolerancesScale());
		desc.gravity = { 0.f, -9.81f, 0.f };

		desc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
		
		desc.simulationEventCallback = &physics->simulation_callback;
		desc.kineKineFilteringMode = physx::PxPairFilteringMode::eKEEP;
		desc.staticKineFilteringMode = physx::PxPairFilteringMode::eKEEP;
		desc.filterShader = contactReportFilterShader;

		if(!desc.cpuDispatcher)
		{
			// TODO
			auto dispatcher = PxDefaultCpuDispatcherCreate(4);
			if(!dispatcher)
				SV_LOG_ERROR("PxDefaultCpuDispatcherCreate failed!");
			desc.cpuDispatcher = dispatcher;
		}
		if(!desc.filterShader)
			desc.filterShader = PxDefaultSimulationFilterShader;
		
		physics->scene = p->createScene(desc);

		if (physics->scene == NULL) {
			SV_LOG_ERROR("Can't create physx scene");
			_physics3D_close();
			return false;
		}

		physics->default_material = p->createMaterial(0.5f, 0.5f, 0.1f);

		SV_LOG_INFO("Physx initialized");

		return true;
	}

	void _physics3D_close()
	{
		if (physics) {
			
			if (physics->scene)
				physics->scene->release();
			
			if (physics->engine)
				physics->engine->release();

			if (physics->foundation)
				physics->foundation->release();
			
			SV_FREE_STRUCT(physics);
			physics = NULL;
		}
	}

	SV_AUX void create_box_collider(BoxCollider& col, PxRigidActor& rigid, Entity entity)
	{
		v3_f32 half_scale = get_entity_world_scale(entity) * col.size * 0.5f;
		
		PxShape* box = physics->engine->createShape(PxBoxGeometry(half_scale.x, half_scale.y, half_scale.z), *physics->default_material);
		rigid.attachShape(*box);
		col._internal = box;
	}

	SV_AUX void create_sphere_collider(SphereCollider& col, PxRigidActor& rigid, Entity entity)
	{
		v3_f32 scale = get_entity_world_scale(entity);

		f32 radius = SV_MAX(SV_MAX(scale.x, scale.y), scale.z) * col.radius * 0.5f;
		
		PxShape* sphere = physics->engine->createShape(PxSphereGeometry(radius), *physics->default_material);
		rigid.attachShape(*sphere);
		col._internal = sphere;
	}

	SV_AUX void free_rigid(BodyComponent& body)
	{
		switch (body._type) {
							
		case BodyType_Dynamic:
		{
			PxRigidDynamic* rigid = (PxRigidDynamic*)body._internal;
			physics->scene->removeActor(*rigid);
			rigid->release();
		}
		break;

		case BodyType_Static:
		{
			PxRigidStatic* rigid = (PxRigidStatic*)body._internal;
			physics->scene->removeActor(*rigid);
			rigid->release();
		}
		break;
							
		}
	}

	SV_AUX void create_rigid(BodyComponent& body, Entity entity, BodyType type)
	{
		SV_PHYSX();
		
		if (body._internal != NULL) {

			free_rigid(body);
		}
		
		switch (type) {
							
		case BodyType_Dynamic:
		{
			PxRigidDynamic* rigid = p->createRigidDynamic(PxTransform(PxTransform({0.f, 0.f, 0.f})));
			SV_ASSERT(rigid);
			PxRigidBodyExt::updateMassAndInertia(*rigid, 0.2f);
			
			rigid->setLinearVelocity({0.f, 0.f, 0.f});
			rigid->userData = &body;

			scene->addActor(*rigid);
			
			body._internal = rigid;
		}
		break;

		case BodyType_Static:
		{
			v3_f32 pos = get_entity_world_position(entity);
			v4_f32 rot = get_entity_world_rotation(entity);
			
			PxRigidStatic* rigid = PxCreateStatic(*p, PxTransform({pos.x, pos.y, pos.z}, {rot.x,rot.y,rot.z,rot.w}), PxBoxGeometry(0.5f, 0.5f, 0.5f), *physics->default_material);
			rigid->userData = &body;

			scene->addActor(*rigid);
			
			body._internal = rigid;
		}
		break;
							
		}

		body._type = type;

		BoxCollider* box = (BoxCollider*)get_entity_component(entity, get_component_id("Box Collider"));
		SphereCollider* sphere = (SphereCollider*)get_entity_component(entity, get_component_id("Sphere Collider"));

		PxRigidActor& rigid = *(PxRigidActor*)body._internal;
		
		if (box) create_box_collider(*box, rigid, entity);
		if (sphere) create_sphere_collider(*sphere, rigid, entity);
	}

	void BodyComponent_create(BodyComponent* comp, Entity entity)
	{
		comp->_internal = NULL;
		create_rigid(*comp, entity, BodyType_Dynamic);
	}
	
	void BodyComponent_destroy(BodyComponent* comp, Entity entity)
	{
		if (comp->_internal) {

			free_rigid(*comp);
		}
	}
	
	void BodyComponent_copy(BodyComponent* dst, const BodyComponent* src, Entity entity)
	{
		dst->_internal = NULL;
		create_rigid(*dst, entity, src->_type);

		if (dst->_type == BodyType_Dynamic) {

			PxRigidDynamic& r0 = *(PxRigidDynamic*)dst->_internal;
			const PxRigidDynamic& r1 = *(const PxRigidDynamic*)src->_internal;
			r0.setLinearDamping(r1.getLinearDamping());
			r0.setAngularDamping(r1.getAngularDamping());
			r0.setLinearVelocity(r1.getLinearVelocity());
			r0.setAngularVelocity(r1.getAngularVelocity());
			r0.setMass(r1.getMass());
		}
	}
	
	void BodyComponent_serialize(BodyComponent* comp, Serializer& s)
	{
		serialize_u32(s, comp->_type);

		if (comp->_type == BodyType_Dynamic) {
			
			PxRigidDynamic& rigid = *(PxRigidDynamic*)comp->_internal;
			
			serialize_f32(s, rigid.getLinearDamping());
			serialize_f32(s, rigid.getAngularDamping());
			serialize_v3_f32(s, px_to_vec3(rigid.getLinearVelocity()));
			serialize_v3_f32(s, px_to_vec3(rigid.getAngularVelocity()));
			serialize_f32(s, rigid.getMass());
		}
	}
	
	void BodyComponent_deserialize(BodyComponent* comp, Deserializer& d, u32 version)
	{
		if (version > 0) {

			BodyType body_type;
			deserialize_u32(d, (u32&)body_type);

			// TODO
			Entity e = comp->id;
			create_rigid(*comp, e, body_type);

			if (comp->_type == BodyType_Dynamic) {
				
				PxRigidDynamic& rigid = *(PxRigidDynamic*)comp->_internal;

				f32 linear_damping;
				f32 angular_damping;
				v3_f32 velocity;
				v3_f32 angular_velocity;
				f32 mass;
				
				deserialize_f32(d, linear_damping);
				deserialize_f32(d, angular_damping);
				deserialize_v3_f32(d, velocity);
				deserialize_v3_f32(d, angular_velocity);
				deserialize_f32(d, mass);

				rigid.setLinearDamping(linear_damping);
				rigid.setAngularDamping(angular_damping);
				rigid.setLinearVelocity(vec3_to_px(velocity));
				rigid.setAngularVelocity(vec3_to_px(angular_velocity));
				rigid.setMass(mass);
			}
		}
	}

	void BoxCollider_create(BoxCollider* comp, Entity entity)
	{
		comp->_internal = NULL;

		comp->size = { 1.f, 1.f, 1.f };

		BodyComponent* body = (BodyComponent*)get_entity_component(entity, get_component_id("Body"));
		if (body) {

			PxRigidActor* rigid = (PxRigidActor*)body->_internal;
			SV_ASSERT(rigid);
			
			create_box_collider(*comp, *rigid, entity);
		}
	}
	
	void BoxCollider_destroy(BoxCollider* comp, Entity entity)
	{
		if (comp->_internal) {
			PxShape* shape = (PxShape*)comp->_internal;
			shape->release();
		}
	}
	
	void BoxCollider_copy(BoxCollider* dst, const BoxCollider* src, Entity entity)
	{
		dst->_internal = NULL;
		BoxCollider_create(dst, entity);
	}
	
	void BoxCollider_serialize(BoxCollider* comp, Serializer& s)
	{
		serialize_v3_f32(s, comp->size);
	}
	
	void BoxCollider_deserialize(BoxCollider* comp, Deserializer& d, u32 version)
	{
		if (version > 0) {

			deserialize_v3_f32(d, comp->size);
		}
	}

	void SphereCollider_create(SphereCollider* comp, Entity entity)
	{
		comp->_internal = NULL;
		comp->radius = 1.f;

		BodyComponent* body = (BodyComponent*)get_entity_component(entity, get_component_id("Body"));
		if (body) {

			PxRigidActor* rigid = (PxRigidActor*)body->_internal;
			SV_ASSERT(rigid);
			
			create_sphere_collider(*comp, *rigid, entity);
		}
	}
	
	void SphereCollider_destroy(SphereCollider* comp, Entity entity)
	{
		if (comp->_internal) {
			PxShape* shape = (PxShape*)comp->_internal;
			shape->release();
		}
	}
	
	void SphereCollider_copy(SphereCollider* dst, const SphereCollider* src, Entity entity)
	{
		dst->_internal = NULL;
		SphereCollider_create(dst, entity);
	}
	
	void SphereCollider_serialize(SphereCollider* comp, Serializer& s)
	{
		serialize_f32(s, comp->radius);
	}
	
	void SphereCollider_deserialize(SphereCollider* comp, Deserializer& d, u32 version)
	{
		if (version > 0) {
			deserialize_f32(d, comp->radius);
		}
	}

	void body_type_set(BodyComponent& body, BodyType body_type)
	{
		// TODO
		Entity entity = body.id;		
		create_rigid(body, entity, body_type);
	}
	
	void body_linear_damping_set(BodyComponent& body, f32 linear_damping)
	{
		SV_ASSERT(linear_damping >= 0.f);
		
		if (body._type != BodyType_Static) {

			PxRigidDynamic& rigid = *(PxRigidDynamic*)body._internal;
			rigid.setLinearDamping(linear_damping);
		}
		else SV_ASSERT(0);
	}
	
	void body_angular_damping_set(BodyComponent& body, f32 angular_damping)
	{
		if (body._type != BodyType_Static) {

			PxRigidDynamic& rigid = *(PxRigidDynamic*)body._internal;
			rigid.setAngularDamping(angular_damping);
		}
		else SV_ASSERT(0);
	}
	
	void body_velocity_set(BodyComponent& body, v3_f32 velocity)
	{
		if (body._type != BodyType_Static) {

			PxRigidDynamic& rigid = *(PxRigidDynamic*)body._internal;
			rigid.setLinearVelocity({ velocity.x, velocity.y, velocity.z });
		}
		else SV_ASSERT(0);
	}
	
	void body_angular_velocity_set(BodyComponent& body, v3_f32 velocity)
	{
		if (body._type != BodyType_Static) {

			PxRigidDynamic& rigid = *(PxRigidDynamic*)body._internal;
			rigid.setAngularVelocity({ velocity.x, velocity.y, velocity.z });
		}
		else SV_ASSERT(0);
	}
	
	void body_mass_set(BodyComponent& body, f32 mass)
	{
		if (body._type != BodyType_Static) {

			PxRigidDynamic& rigid = *(PxRigidDynamic*)body._internal;
			rigid.setMass(mass);
		}
		else SV_ASSERT(0);
	}

	BodyType body_type_get(const BodyComponent& body)
	{
		return body._type;
	}
	
	f32 body_linear_damping_get(const BodyComponent& body)
	{
		if (body._type == BodyType_Dynamic) {

			PxRigidDynamic& rigid = *(PxRigidDynamic*)body._internal;
			return rigid.getLinearDamping();
		}

		return 0.f;
	}
	
	f32 body_angular_damping_get(const BodyComponent& body)
	{
		if (body._type == BodyType_Dynamic) {

			PxRigidDynamic& rigid = *(PxRigidDynamic*)body._internal;
			return rigid.getAngularDamping();
		}

		return 0.f;
	}
	
	v3_f32 body_velocity_get(const BodyComponent& body)
	{
		if (body._type == BodyType_Dynamic) {

			PxRigidDynamic& rigid = *(PxRigidDynamic*)body._internal;
			PxVec3 vec = rigid.getLinearVelocity();
			return { vec.x, vec.y, vec.z };
		}

		return 0.f;
	}
	
	v3_f32 body_angular_velocity_get(const BodyComponent& body)
	{
		if (body._type == BodyType_Dynamic) {

			PxRigidDynamic& rigid = *(PxRigidDynamic*)body._internal;
			PxVec3 vec = rigid.getAngularVelocity();
			return { vec.x, vec.y, vec.z };
		}

		return 0.f;
	}
	
	f32 body_mass_get(const BodyComponent& body)
	{
		if (body._type == BodyType_Dynamic) {

			PxRigidDynamic& rigid = *(PxRigidDynamic*)body._internal;
			return rigid.getMass();
		}

		return 0.f;
	}

	void body_apply_force(BodyComponent& body, v3_f32 force, ForceType force_type)
	{
		if (body._type != BodyType_Static) {

			PxRigidDynamic& rigid = *(PxRigidDynamic*)body._internal;

			PxForceMode::Enum mode;

			switch (force_type) {

			case ForceType_Force:
				mode = PxForceMode::Enum::eFORCE;
				break;

			case ForceType_Impulse:
				mode = PxForceMode::Enum::eIMPULSE;
				break;

			case ForceType_VelocityChange:
				mode = PxForceMode::Enum::eVELOCITY_CHANGE;
				break;

			case ForceType_Acceleration:
				mode = PxForceMode::Enum::eACCELERATION;
				break;

			default:
				mode = PxForceMode::Enum::eFORCE;
				SV_ASSERT(0);
				
			}
			
			rigid.addForce(vec3_to_px(force), mode);
		}
	}

	void _physics3D_update()
	{
		f32 dt = engine.deltatime;
		PxScene* scene = physics->scene;

		// Update actors transforms
		{
			for (CompIt it = comp_it_begin(get_component_id("Body"));
				 it.has_next;
				 comp_it_next(it))
			{
				BodyComponent& body = *(BodyComponent*)it.comp;

				// TODO: check if dirty_physics is true
				if (body._type != BodyType_Static) {

					PxRigidDynamic* rigid = (PxRigidDynamic*) body._internal;
					
					PxTransform p;
					v3_f32 position = get_entity_world_position(it.entity);
					v4_f32 rotation = get_entity_world_rotation(it.entity);
					p.p.x = position.x;
					p.p.y = position.y;
					p.p.z = position.z;
					p.q.x = rotation.x;
					p.q.y = rotation.y;
					p.q.z = rotation.z;
					p.q.w = rotation.w;
					rigid->setGlobalPose(p);

					// Update shapes

					v3_f32 scale = get_entity_world_scale(it.entity);

					BoxCollider* box = (BoxCollider*)get_entity_component(it.entity, get_component_id("Box Collider"));
					SphereCollider* sphere = (SphereCollider*)get_entity_component(it.entity, get_component_id("Sphere Collider"));

					if (box) {

						PxShape* shape = (PxShape*)box->_internal;

						rigid->detachShape(*shape);
						
						PxBoxGeometry b;
						if (shape->getBoxGeometry(b)) {

							b.halfExtents = vec3_to_px(scale * box->size * 0.5f);

							shape->setGeometry(b);
						}
						else SV_ASSERT(0);

						rigid->attachShape(*shape);
					}

					if (sphere) {

						PxShape* shape = (PxShape*)sphere->_internal;

						rigid->detachShape(*shape);
						
						PxSphereGeometry s;
						if (shape->getSphereGeometry(s)) {

							s.radius = SV_MAX(SV_MAX(scale.x, scale.y), scale.z) * sphere->radius * 0.5f;
							shape->setGeometry(s);
						}
						else SV_ASSERT(0);

						rigid->attachShape(*shape);
					}
				}
			}
		}

		// TODO: Thats not good
		scene->simulate(dt);

		scene->fetchResults(true);

		u32 actor_count;
		PxActor** actors = scene->getActiveActors(actor_count);

		foreach(i, actor_count) {

			PxRigidActor* actor = (PxRigidActor*)actors[i];
			BodyComponent* body = (BodyComponent*)actor->userData;
			
			// TODO
			Entity e = body->id;

			PxTransform p = actor->getGlobalPose();
			v3_f32& position = *get_entity_position_ptr(e);
			v4_f32& rotation = *get_entity_rotation_ptr(e);
			position.x = p.p.x;
			position.y = p.p.y;
			position.z = p.p.z;
			rotation.x = p.q.x;
			rotation.y = p.q.y;
			rotation.z = p.q.z;
			rotation.w = p.q.w;

			// TODO: Set the dirty_physics to true
		}
	}
	
}
