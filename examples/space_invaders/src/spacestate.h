#pragma once

#include "game.h"
#include "particle/particle_emission.h"

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

	void serialize(ArchiveO& file) {}
	void deserialize(ArchiveI& file) {}
};

enum ExplosionType : u32 {
	ExplosionType_Ship,
	ExplosionType_Asteroid,
	ExplosionType_Dangerous
};

struct ExplosionComponent : public Component<ExplosionComponent> {
	ExplosionType explosionType;

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

	CameraComponent& getCamera() override;

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
			Transform trans = ecs_entity_transform_get(scene, player);
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
		Entity e = ecs_entity_create(scene);
		SpriteComponent& spr = *ecs_component_add<SpriteComponent>(scene, e);
		ShipComponent& ship = *ecs_component_add<ShipComponent>(scene, e);
		ship.shipType = type;

		Particle2DEmitterComponent_CPU& part = *ecs_component_add<Particle2DEmitterComponent_CPU>(scene, e);
		part.rateTime = 0.01f;

		part.start.lifeTime0 = 0.1f;
		part.start.lifeTime1 = 0.4f;
		part.shape = Particle2DShape_Cone;
		part.shapeCone.angle = PI / 3.f;
		part.shapeCone.direction = PI;
		part.duration = 0.f;
		part.start.velocity0 = 20.f;
		part.start.velocity1 = 28.f;
		part.start.opacity0 = 20u;
		part.start.opacity1 = 100u;
		part.draw = Particle2DDraw_Sprite;

		Color partColor;

		Transform trans = ecs_entity_transform_get(scene, e);
		trans.setPosition(pos.getVec3());

		switch (type)
		{
		case ShipType_Player:
			spr.sprite.texture.loadFromFile("images/ships0.png");
			spr.sprite.texCoord = { 2.f / 3.f, 2.f / 3.f, 3.f / 3.f, 3.f / 3.f };
			ship.health = 1.f;
			trans.setScale({ 2.f, 2.f, 0.f });
			part.start.size0 = 0.4f;
			part.start.size1 = 0.8f;
			partColor = Color::Orange();
			break;

		case ShipType_Kamikaze:
			spr.sprite.texture.loadFromFile("images/ships0.png");
			spr.sprite.texCoord = { 0.f / 3.f, 1.f / 3.f, 1.f / 3.f, 2.f / 3.f };
			ship.health = 1.f;
			trans.setScale({ 1.f, 1.f, 0.f });
			part.start.size0 = 0.1f;
			part.start.size1 = 0.5f;
			partColor = Color::Orange();
			break;

		case ShipType_Shooter:
			spr.sprite.texture.loadFromFile("images/ships0.png");
			spr.sprite.texCoord = { 1.f / 3.f, 0.f / 3.f, 2.f / 3.f, 1.f / 3.f };
			ship.health = 1.f;
			trans.setScale({ 1.f, 1.f, 0.f });
			part.start.size0 = 0.1f;
			part.start.size1 = 0.5f;
			partColor = Color::Orange();
			break;

		case ShipType_Daddy:
			spr.sprite.texture.loadFromFile("images/ships0.png");
			spr.sprite.texCoord = { 0.f / 3.f, 2.f / 3.f, 1.f / 3.f, 3.f / 3.f };
			ship.health = 20.f;
			trans.setScale({ 10.f, 10.f, 0.f });
			part.start.size0 = 0.9f;
			part.start.size1 = 2.f;
			partColor = Color::Orange();
			break;

		default:
			SV_LOG_ERROR("Unknown ship type");
			break;
		}

		spr.renderLayer = 0u;

		part.start.red0 = partColor.r;
		part.start.red1 = partColor.r;
		part.start.green0 = partColor.g;
		part.start.green1 = partColor.g;
		part.start.blue0 = partColor.b;
		part.start.blue1 = partColor.b;

		return e;
	}

	Entity createAsteroid(v2_f32 pos, f32 size, f32 vel, f32 dir)
	{
		Entity e = ecs_entity_create(scene);
		SpriteComponent& spr = *ecs_component_add<SpriteComponent>(scene, e);
		spr.sprite.texture.loadFromFile("images/asteroid.png");

		u32 type = random.gen_u32(4u);

		switch (type)
		{
		case 0:
			spr.sprite.texCoord = { 0.f, 0.f, 0.5f, 0.5f };
			break;
		case 1:
			spr.sprite.texCoord = { 0.5f, 0.f, 1.f, 0.5f };
			break;
		case 2:
			spr.sprite.texCoord = { 0.f, 0.5f, 0.5f, 1.f };
			break;
		case 3:
			spr.sprite.texCoord = { 0.5f, 0.5f, 1.f, 1.f };
			break;
		}

		spr.color = { 115u, 77u, 38u, 255u };

		ProjectileComponent& prj = *ecs_component_add<ProjectileComponent>(scene, e);
		prj.vel = vel;
		prj.dir = dir;

		AsteroidComponent& a = *ecs_component_add<AsteroidComponent>(scene, e);
		a.rotVel = random.gen_f32(0.01f, 0.1f);
		a.health = size * 0.8f;

		Transform trans = ecs_entity_transform_get(scene, e);
		trans.setScale({ size, size, 0.f });
		trans.setPosition(pos.getVec3());
		trans.setEulerRotation({ 0.f, 0.f, dir });

		return e;
	}

	Entity createBullet(Entity from, f32 vel, f32 dir, f32 size, f32 lifeTime = 4.f, f32 damage = 1.f, bool penetrate = false)
	{
		Entity e = ecs_entity_create(scene);
		SpriteComponent& spr = *ecs_component_add<SpriteComponent>(scene, e);
		spr.color = Color::Orange();
		ProjectileComponent& prj = *ecs_component_add<ProjectileComponent>(scene, e);
		prj.vel = vel;
		prj.dir = dir;

		BulletComponent& bullet = *ecs_component_add<BulletComponent>(scene, e);
		bullet.from = from;
		bullet.lifeTime = lifeTime;
		bullet.damage = damage;
		bullet.penetrate = penetrate;

		Particle2DEmitterComponent_CPU& emitter = *ecs_component_add<Particle2DEmitterComponent_CPU>(scene, e);
		emitter.shape = Particle2DShape_Circle;
		emitter.shapeCircle.radius = size * 0.5f;
		emitter.start.velocity0 = 0.4f;
		emitter.start.velocity1 = 0.8f;
		emitter.start.size0 = 0.05f;
		emitter.start.size1 = 0.1f;
		emitter.start.lifeTime0 = 0.8f;
		emitter.start.lifeTime1 = 2.5f;
		emitter.start.opacity0 = 5u;
		emitter.start.opacity1 = 10u;
		emitter.rateTime = 1.f / 500.f;
		emitter.start.red0 = spr.color.r;
		emitter.start.green0 = spr.color.g;
		emitter.start.blue0 = spr.color.b;
		emitter.draw = Particle2DDraw_Sprite;

		Transform fromTrans = ecs_entity_transform_get(scene, from);
		Transform trans = ecs_entity_transform_get(scene, e);
		trans.setPosition(fromTrans.getWorldPosition());
		trans.setScale({ size, size, 0.f });

		spr.renderLayer = 1u;
		emitter.renderLayer = 1u;

		ecs_component_remove<Particle2DEmitterComponent_CPU>(scene, e);

		return e;
	}

	Entity createExplosion(const v2_f32& pos, f32 size, ExplosionType type)
	{
		Entity e = ecs_entity_create(scene);
		AnimatedSpriteComponent& spr = *ecs_component_add<AnimatedSpriteComponent>(scene, e);

		SpriteAnimationAsset asset;

		switch (type)
		{
		case ExplosionType_Ship:
			asset.loadFromFile("animations/explosion.anim");
			break;

		case ExplosionType_Asteroid:
			asset.loadFromFile("animations/explosion.anim");
			break;

		case ExplosionType_Dangerous:
			asset.loadFromFile("animations/explosion.anim");
			break;
		}

		spr.sprite.setAnimation(asset);
		spr.sprite.setSpriteDuration(0.1f);
		spr.sprite.setRepeatCount(1u);
		spr.sprite.start();

		ExplosionComponent& exp = *ecs_component_add<ExplosionComponent>(scene, e);
		exp.explosionType = type;

		Transform trans = ecs_entity_transform_get(scene, e);
		trans.setScale({ size, size, 0.f });
		trans.setPosition(pos.getVec3());

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

		Transform trans = ecs_entity_transform_get(scene, s.entity);

		createExplosion(trans.getWorldPosition().getVec2(), trans.getWorldScale().getVec2().length() * 2.f, (s.shipType == ShipType_Kamikaze ? ExplosionType_Dangerous : ExplosionType_Ship));

		ecs_entity_destroy(scene, s.entity);
	}

	void destroyAsteroid(AsteroidComponent& a)
	{
		ProjectileComponent& p = *ecs_component_get<ProjectileComponent>(scene, a.entity);

		Transform trans = ecs_entity_transform_get(scene, a.entity);
		v2_f32 pos = trans.getWorldPosition().getVec2();
		f32 scale = trans.getWorldScale().getVec2().length();

		if (inScreen(pos, scale)) {

			if (scale >= 2.5f) {
				f32 size = scale * random.gen_f32(0.3f, 0.7f);

				createAsteroid(pos, size * 0.9f, p.vel, p.dir + PI / 6.f);
				createAsteroid(pos, (scale - size) * 0.9f, p.vel, p.dir - PI / 6.f);
			}
		}

		createExplosion(pos, scale, ExplosionType_Asteroid);

		ecs_entity_destroy(scene, a.entity);
	}

	void destroyBullet(BulletComponent& bullet)
	{
		ecs_entity_destroy(scene, bullet.entity);
	}

	void destroyExplosion(ExplosionComponent& e)
	{
		ecs_entity_destroy(scene, e.entity);
	}

	template<typename Comp0, typename Comp1, typename OnCollideFn>
	void collideObjects(f32 radiusMult0, f32 radiusMult1, OnCollideFn onCollide)
	{
		EntityView<Comp0> comp0(scene);

		for (Comp0& c0 : comp0) {

			Transform trans0 = ecs_entity_transform_get(scene, c0.entity);
			f32 radius0 = trans0.getWorldScale().getVec2().length() * radiusMult0;
			v2_f32 pos0 = trans0.getWorldPosition().getVec2();

			EntityView<Comp1> comp1(scene);
			for (Comp1& c1 : comp1) {

				Transform trans1 = ecs_entity_transform_get(scene, c1.entity);
				f32 radius1 = trans1.getWorldScale().getVec2().length() * radiusMult0;
				v2_f32 pos1 = trans1.getWorldPosition().getVec2();

				f32 dist = (pos0 - pos1).length();

				if (dist < radius0 + radius1) {
					if (!onCollide(c0, c1)) break;
				}
			}
		}
	}

	Scene scene;
	Entity player, background;
	Random random;

	ShipGenerator shipGenerator;

};