#pragma once

#include "game.h"

#include "SilverEngine.h"
#include "SilverEngine/utils/allocators/FrameList.h"

enum ShipType : u32 {
	ShipType_Player,
	ShipType_Kamikaze,
	ShipType_Shooter,
	ShipType_Daddy
};

enum BulletType {
	BulletType_Noob,
	BulletType_Shotgun,
	BulletType_MachineGun,
	BulletType_Sniper,
};

struct ShipComponent : public Component<ShipComponent> {

	Color color = Color::Black();

	v2_f32 vel;
	f32 maxVel = 10.f;
	f32 acc = 0.f;
	ShipType shipType;
	f32 health = 1.f;
	f32 shotTime = 0.f;
	BulletType bulletType = BulletType_Noob;

	void serialize(ArchiveO& file) {}
	void deserialize(ArchiveI& file) {}
	
};

struct ProjectileComponent : public Component<ProjectileComponent> {
	f32 vel = 1.f;
	f32 dir = 0.f;

	void serialize(ArchiveO& file) {}
	void deserialize(ArchiveI& file) {}
};

struct BulletComponent : public Component<BulletComponent> {
	Entity from = SV_ENTITY_NULL;
	Entity last = SV_ENTITY_NULL;
	f32 lifeTime = 1.f;
	f32 damage = 1.f;
	bool penetrate = false;

	void serialize(ArchiveO& file) {}
	void deserialize(ArchiveI& file) {}
};

struct AsteroidComponent : public Component<AsteroidComponent> {
	f32 rotVel = PI;
	f32 health = 1.f;
	u32 asteroidID = 0u;

	void serialize(ArchiveO& file) {}
	void deserialize(ArchiveI& file) {}
};

struct SpaceState;

struct ShipGeneration {

	struct Ship {
		u32			min;
		u32			max;
		ShipType	type;
	};

	u32					rate = 0;
	std::vector<Ship>	ships;
	f32					time = 1.f;

};

struct ShipGenerator {

	void generateShips(SpaceState& state, f32 dt);

	void addDefaultGenerations();
	void updateGenerations();

	f32 timer = 0.f;
	f32 shipRate = 2.f;

	u32 maxGenValue = 0u;
	std::vector<ShipGeneration> generations;
	ShipGeneration gen;
	f32 genRate = 0.f;

};

struct SpaceState : public GameState {

	Result initialize() override;
	void update(f32 dt) override;
	void fixedUpdate() override;
	void render() override;
	Result close() override;

	CameraProjection& getCameraProjection() override;

	void shipShot(ShipComponent& ship, f32 dir, bool condition, f32 dt);

	SV_INLINE std::pair<v2_f32, f32> genPos()
	{
		v2_f32 pos;
		u32 side;
		f32 dir;

		f32 w = camWidth();
		f32 h = camHeight();

		f32 maxDistance = math_sqrt(w * w + h * h) * 0.2f;

		v2_f32 playerPos;
		if (player != SV_ENTITY_NULL) {
			Transform trans = ecs_entity_transform_get(ecs, player);
			playerPos = trans.getLocalPosition().getVec2();
		}
		else playerPos = { w, h };

		while (true) {

			side = random.gen_u32(4u);

			switch (side)
			{
			case 0:
				pos.x = random.gen_f32(w) - w * 0.5f;
				pos.y = h * -0.5f;
				dir = PI / 2.f;
				break;

			case 1:
				pos.x = random.gen_f32(w) - w * 0.5f;
				pos.y = h * 0.5f;
				dir = 3.f * PI / 2.f;
				break;

			case 2:
				pos.x = w * -0.5f;
				pos.y = random.gen_f32(h) - h * 0.5f;
				dir = 0.f;
				break;

			case 3:
				pos.x = w * 0.5f;
				pos.y = random.gen_f32(h) - h * 0.5f;
				dir = PI;
				break;
			}

			f32 dist = (playerPos - pos).length();
			if (dist >= maxDistance)
				break;
		}

		return { pos, dir };
	}

	Entity createShip(v2_f32 pos, ShipType type)
	{
		Entity e = ecs_entity_create(ecs);
		ShipComponent& ship = *ecs_component_add<ShipComponent>(ecs, e);
		ship.shipType = type;

		Transform trans = ecs_entity_transform_get(ecs, e);
		trans.setPosition(pos.getVec3());

		switch (type)
		{
		case ShipType_Player:
			ship.health = 1.f;
			trans.setScale({ 2.f, 2.f, 0.f });
			break;

		case ShipType_Kamikaze:
			ship.health = 1.f;
			trans.setScale({ 1.f, 1.f, 0.f });
			break;

		case ShipType_Shooter:
			ship.health = 1.f;
			trans.setScale({ 1.f, 1.f, 0.f });
			break;

		case ShipType_Daddy:
			ship.health = 20.f;
			trans.setScale({ 10.f, 10.f, 0.f });
			break;

		default:
			SV_LOG_ERROR("Unknown ship type");
			break;
		}

		return e;
	}

	Entity createAsteroid(v2_f32 pos, f32 size, f32 vel, f32 dir)
	{
		Entity e = ecs_entity_create(ecs);

		u32 type = random.gen_u32(4u);

		ProjectileComponent& prj = *ecs_component_add<ProjectileComponent>(ecs, e);
		prj.vel = vel;
		prj.dir = dir;

		AsteroidComponent& a = *ecs_component_add<AsteroidComponent>(ecs, e);
		a.rotVel = random.gen_f32(0.01f, 0.1f);
		a.health = size * 0.8f;
		a.asteroidID = random.gen_u32(4u);

		Transform trans = ecs_entity_transform_get(ecs, e);
		trans.setScale({ size, size, 0.f });
		trans.setPosition(pos.getVec3());
		trans.setEulerRotation({ 0.f, 0.f, dir });

		return e;
	}

	Entity createBullet(Entity from, f32 vel, f32 dir, f32 size, f32 lifeTime = 4.f, f32 damage = 1.f, bool penetrate = false)
	{
		Entity e = ecs_entity_create(ecs);
		//spr.color = Color::Orange();
		ProjectileComponent& prj = *ecs_component_add<ProjectileComponent>(ecs, e);
		prj.vel = vel;
		prj.dir = dir;

		BulletComponent& bullet = *ecs_component_add<BulletComponent>(ecs, e);
		bullet.from = from;
		bullet.lifeTime = lifeTime;
		bullet.damage = damage;
		bullet.penetrate = penetrate;

		Transform fromTrans = ecs_entity_transform_get(ecs, from);
		Transform trans = ecs_entity_transform_get(ecs, e);
		trans.setPosition(fromTrans.getWorldPosition());
		trans.setScale({ size, size, 0.f });

		return e;
	}

	void destroyShip(ShipComponent& s)
	{
		switch (s.shipType)
		{
		case ShipType_Player:
			if (s.entity == player)
				player = SV_ENTITY_NULL;
			break;

		}

		Transform trans = ecs_entity_transform_get(ecs, s.entity);

		ecs_entity_destroy(ecs, s.entity);
	}

	void destroyAsteroid(AsteroidComponent& a)
	{
		ProjectileComponent& p = *ecs_component_get<ProjectileComponent>(ecs, a.entity);

		Transform trans = ecs_entity_transform_get(ecs, a.entity);
		v2_f32 pos = trans.getWorldPosition().getVec2();
		f32 scale = trans.getWorldScale().getVec2().length();

		if (inScreen(pos, scale)) {

			if (scale >= 2.5f) {
				f32 size = scale * random.gen_f32(0.3f, 0.7f);

				createAsteroid(pos, size * 0.9f, p.vel, p.dir + PI / 6.f);
				createAsteroid(pos, (scale - size) * 0.9f, p.vel, p.dir - PI / 6.f);
			}
		}

		ecs_entity_destroy(ecs, a.entity);
	}

	void destroyBullet(BulletComponent& bullet)
	{
		ecs_entity_destroy(ecs, bullet.entity);
	}

	template<typename Comp0, typename Comp1, typename OnCollideFn>
	void collideObjects(f32 radiusMult0, f32 radiusMult1, OnCollideFn onCollide)
	{
		EntityView<Comp0> comp0(ecs);

		for (Comp0& c0 : comp0) {

			Transform trans0 = ecs_entity_transform_get(ecs, c0.entity);
			f32 radius0 = trans0.getWorldScale().getVec2().length() * radiusMult0;
			v2_f32 pos0 = trans0.getWorldPosition().getVec2();

			EntityView<Comp1> comp1(ecs);
			for (Comp1& c1 : comp1) {

				Transform trans1 = ecs_entity_transform_get(ecs, c1.entity);
				f32 radius1 = trans1.getWorldScale().getVec2().length() * radiusMult0;
				v2_f32 pos1 = trans1.getWorldPosition().getVec2();

				f32 dist = (pos0 - pos1).length();

				if (dist < radius0 + radius1) {
					if (!onCollide(c0, c1)) break;
				}
			}
		}
	}

	ECS* ecs;

	FrameList<SpriteInstance>	sprite_instances;
	CameraProjection			projection;
	Font font;

	Entity player, background;
	Random random;

	ShipGenerator shipGenerator;

	GPUImage* offscreen = nullptr;

	Sprite sprite_Player;
	Sprite sprite_Kamikaze;
	Sprite sprite_Shooter;
	Sprite sprite_Daddy;
	Sprite sprite_Asteroid0;
	Sprite sprite_Asteroid1;
	Sprite sprite_Asteroid2;
	Sprite sprite_Asteroid3;

};