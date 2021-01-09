#include "spacestate.h"

void SpaceState::shipShot(ShipComponent& ship, f32 dir, bool condition, f32 dt)
{
	ship.shotTime += dt;

	f32 rate = f32_max;

	switch (ship.bulletType)
	{
	case BulletType_Noob:
		rate = 0.5f;
		break;
	case BulletType_Shotgun:
		rate = 1.f;
		break;
	case BulletType_MachineGun:
		rate = 0.01f;
		break;
	case BulletType_Sniper:
		rate = 0.7f;
		break;
	}

	if (condition) {

		while (ship.shotTime >= rate) {
			ship.shotTime -= rate;

			switch (ship.bulletType)
			{
			case BulletType_Noob:
				ship.acc -= 0.1f;
				createBullet(ship.entity, 0.1f, dir, 0.5f, 4.f, 2.f, false);
				break;

			case BulletType_Shotgun:
				ship.acc -= 0.6f;

				foreach(i, 30)
					createBullet(ship.entity, 1.f, dir + random.gen_f32(-PI * 0.25f, PI * 0.25f), 0.5f, 0.3f, 1.f, false);
				break;

			case BulletType_MachineGun:
				ship.acc -= 0.01f;
				createBullet(ship.entity, 1.f, dir + random.gen_f32(-PI * 0.05f, PI * 0.05f), 0.5f, 4.f, 0.5f, false);
				break;

			case BulletType_Sniper:
				ship.acc -= 0.5f;
				createBullet(ship.entity, 2.f, dir, 0.5f, 4.f, 4.f, true);
				break;
			}
		}
	}
	else ship.shotTime = std::min(ship.shotTime, rate);
}

Result createAssets()
{
	// TEXTURES
	TextureAsset playerTex;
	TextureAsset explosionTex;

	svCheck(playerTex.loadFromFile("images/ship.png"));
	svCheck(explosionTex.loadFromFile("images/explosion.png"));

	// ANIMATED SPRITES
	SpriteAnimationAsset spr;

	if (result_fail(spr.loadFromFile("animations/explosion.anim"))) {
		SpriteAnimation a;

		constexpr f32 offset = 1.f / 5.f;

		Sprite s;
		s.texture = explosionTex;
		s.texCoord = { 0.f, 0.f, 1.f, 1.f };

		a.sprites.push_back(Sprite{ explosionTex, { offset * 0.f, 0.f, offset * 1.f, 1.f } });
		a.sprites.push_back(Sprite{ explosionTex, { offset * 1.f, 0.f, offset * 2.f, 1.f } });
		a.sprites.push_back(Sprite{ explosionTex, { offset * 2.f, 0.f, offset * 3.f, 1.f } });
		a.sprites.push_back(Sprite{ explosionTex, { offset * 3.f, 0.f, offset * 4.f, 1.f } });
		a.sprites.push_back(Sprite{ explosionTex, { offset * 4.f, 0.f, offset * 5.f, 1.f } });

		spr.createFile("animations/explosion.anim", a);
	}

	svCheck(asset_refresh());
	svCheck(spr.loadFromFile("animations/explosion.anim"));

	return Result_Success;
}

Result SpaceState::initialize()
{
	ecs_component_register<ShipComponent>("Ship");
	ecs_component_register<ProjectileComponent>("Projectile");
	ecs_component_register<AsteroidComponent>("Asteroid");
	ecs_component_register<BulletComponent>("Bullet");
	ecs_component_register<ExplosionComponent>("Explosion");
	
	scene.create();

	if (result_fail(createAssets())) {
		SV_LOG_ERROR("Can't create the assets");
	}

	random.setSeed(timer_now().GetMillisecondsUInt());

	// Create sky
	ecs_component_add<SkyComponent>(scene, ecs_entity_create(scene));

	// Create player
	player = createShip({}, ShipType_Player);

	// Adjust RenderLayers
	SceneRenderer::renderLayers2D[0].name = "Ships";
	SceneRenderer::renderLayers2D[0].sortValue = 10;
	SceneRenderer::renderLayers2D[0].enabled = true;

	SceneRenderer::renderLayers2D[1].name = "Bullets";
	SceneRenderer::renderLayers2D[1].sortValue = 5;
	SceneRenderer::renderLayers2D[1].enabled = true;

	SceneRenderer::renderLayers2D[2].name = "Background";
	SceneRenderer::renderLayers2D[2].sortValue = 0;
	SceneRenderer::renderLayers2D[2].enabled = true;

	shipGenerator.addDefaultGenerations();

	// Generate background particles
	{
		background = ecs_entity_create(scene);
		Particle2DEmitterComponent_CPU& e = *ecs_component_add<Particle2DEmitterComponent_CPU>(scene, background);
		e.renderLayer = 2u;
		e.rateTime = 1.f / 100.f;
		e.maxParticles = 20u;
		e.draw = Particle2DDraw_Sprite;
		e.start.velocity0 = 0.f;
		e.start.velocity1 = 0.2f;
		e.start.size0 = 0.05f;
		e.start.size1 = 0.2f;
		e.start.opacity0 = 10u;
		e.start.opacity1 = 50u;
		e.shape = Particle2DShape_Quad;
		e.shapeQuad.size = { 1080.f / 15.f, 720.f / 15.f };
	}

	// Adjust Main Camera
	CameraComponent& cam = *ecs_component_get<CameraComponent>(scene, scene.getMainCamera());
	cam.projection.projectionType = ProjectionType_Orthographic;
	cam.projection.width = 1080.f / 15.f;
	cam.projection.height = 720.f / 15.f;
	cam.projection.near = -100.f;
	cam.projection.far = 100.f;

	cam.projection.updateMatrix();

	return Result_Success;
}

void SpaceState::update(f32 dt)
{
	partsys_update(scene, dt);

	//Particle2DEmitterComponent_CPU& e = *ecs_component_get<Particle2DEmitterComponent_CPU>(scene, background);
	//SV_LOG("Particles: %u", e.particles.size());
	//return;


	// Asteroid Generator
	{
		static f32 timer = 0.f;
		static f32 asteroidRate = 2.f;
		static f32 rate = asteroidRate;

		timer += dt;

		if (timer >= rate) {
			timer -= rate;

			rate = asteroidRate + random.gen_f32(-0.2f, 0.2f);

			auto [pos, dir] = genPos();

			createAsteroid(pos, random.gen_f32(1.3f, 4.f), random.gen_f32(0.1f, 0.5f), dir);
		}
	}

	shipGenerator.generateShips(*this, dt);

	// Player controller
	if (player != SV_ENTITY_NULL) {
		Transform trans = ecs_entity_transform_get(scene, player);

		ShipComponent& ship = *ecs_component_get<ShipComponent>(scene, player);
		v2_f32 pos = trans.getLocalPosition().getVec2();
		v2_f32 scale = trans.getLocalScale().getVec2();

		v2_f32 mousePos = input_mouse_position_get() * camSize();
		v2_f32 toMouse = mousePos - pos;
		f32 dir = toMouse.angle();

		trans.setEulerRotation({ 0.f, 0.f, dir });

		if (input_mouse(SV_MOUSE_RIGHT)) {
			ship.acc += dt;
		}

		ship.bulletType = BulletType_Shotgun;
		shipShot(ship, dir, input_mouse(SV_MOUSE_LEFT), dt);
	}
	else {
		static f32 exitCount = 0.f;
		exitCount += dt;

		if (exitCount > 1.5f) {
			engine_request_close();
		}
	}

	// Ship AI
	{
		EntityView<ShipComponent> ships(scene);

		v2_f32 pPos;
		if (player != SV_ENTITY_NULL) {
			Transform pTrans = ecs_entity_transform_get(scene, player);
			pPos = pTrans.getLocalPosition().getVec2();
		}

		for (ShipComponent& ship : ships) {

			if (ship.shipType == ShipType_Player)
				continue;

			Transform trans = ecs_entity_transform_get(scene, ship.entity);

			switch (ship.shipType)
			{
			case ShipType_Kamikaze:
				if (player != SV_ENTITY_NULL) {

					v2_f32 to = pPos - trans.getWorldPosition().getVec2();

					ship.acc += dt * random.gen_f32(0.8f, 1.2f);
					trans.setEulerRotation({ 0.f, 0.f, to.angle() });
				}
				break;

			case ShipType_Shooter:
				if (player != SV_ENTITY_NULL) {

					v2_f32 to = pPos - trans.getWorldPosition().getVec2();

					if (to.length() < 20.f) {
						f32 dir = to.angle();
						trans.setEulerRotation({ 0.f, 0.f, dir });
						shipShot(ship, dir, true, dt);
					}
					else {
						ship.acc += dt * 0.8f;
						trans.setEulerRotation({ 0.f, 0.f, to.angle() });
					}
				}
				break;

			case ShipType_Daddy:
			{
				v2_f32 pos = trans.getWorldPosition().getVec2();
				v2_f32 to = pPos - pos;

				if (to.length() > 15.f) {
					f32 dir = to.angle();
					trans.setEulerRotation({ 0.f, 0.f, dir });
					ship.acc += dt * 0.3f;
				}

				ship.shotTime += dt;

				if (ship.shotTime > 2.f)
				{
					ship.shotTime -= 2.f;
					createShip(pos, ShipType_Kamikaze);
				}

			}	break;
			}
		}
	}

	// DESTROY ENTITIES
	{
		// Destroy Bullets
		{
			EntityView<BulletComponent> bullets(scene);

			for (BulletComponent& bullet : bullets) {

				bullet.lifeTime -= dt;

				if (bullet.lifeTime <= 0.f || bullet.damage <= 0.f) {
					destroyBullet(bullet);
					break;
				}
			}
		}

		// Destroy Ships
		{
			EntityView<ShipComponent> ships(scene);

			for (ShipComponent& s : ships) {

				if (s.health <= 0.f) {
					destroyShip(s);
					continue;
				}
			}
		}
		// Destroy Asteroids
		{
			EntityView<AsteroidComponent> asteroids(scene);

			for (AsteroidComponent& a : asteroids) {

				Transform trans = ecs_entity_transform_get(scene, a.entity);
				v2_f32 pos = trans.getWorldPosition().getVec2();
				f32 scale = trans.getWorldScale().getVec2().length();

				if (a.health <= 0.f) {
					destroyAsteroid(a);
					continue;
				}

				if (!inScreen(pos, scale * 0.5f)) {
					destroyAsteroid(a);
					continue;
				}
			}
		}
		// Destroy Explosions
		{
			EntityView<ExplosionComponent> explosions(scene);

			for (ExplosionComponent& e : explosions) {

				AnimatedSpriteComponent& s = *ecs_component_get<AnimatedSpriteComponent>(scene, e.entity);

				if (!s.sprite.isRunning())
				{
					destroyExplosion(e);
					continue;
				}
			}
		}
	}
}

void SpaceState::fixedUpdate()
{
	// Move projectiles
	{
		EntityView<ProjectileComponent> projectiles(scene);

		for (ProjectileComponent& prj : projectiles) {

			Transform trans = ecs_entity_transform_get(scene, prj.entity);

			v2_f32 vel = { prj.vel, 0.f };
			vel.setAngle(prj.dir);

			trans.setPosition(trans.getLocalPosition() + vel.getVec3());
		}
	}

	// Rotate asteroids
	{
		EntityView<AsteroidComponent> asteroids(scene);

		for (AsteroidComponent& a : asteroids) {

			Transform trans = ecs_entity_transform_get(scene, a.entity);
			v3_f32 rot = trans.getLocalEulerRotation();
			rot.z += a.rotVel;
			trans.setEulerRotation(rot);
		}
	}

	// Move ships
	{
		EntityView<ShipComponent> ships(scene);

		for (ShipComponent& ship : ships) {

			Transform trans = ecs_entity_transform_get(scene, ship.entity);

			f32 dir = trans.getWorldEulerRotation().z;

			Particle2DEmitterComponent_CPU* emitter = ecs_component_get<Particle2DEmitterComponent_CPU>(scene, ship.entity);

			// Active particles
			if (emitter) {
				if (ship.acc > 0.f) {

					emitter->flags |= ParticleEmitterFlag_Enable;
				}
				else emitter->flags &= ~ParticleEmitterFlag_Enable;
			}

			// Apply acc
			if (ship.acc != 0.f) {

				v2_f32 acc = { ship.acc, 0.f };
				acc.setAngle(dir);
				ship.vel += acc * (ship.acc > 0.f ? 1.f : -1.f);
				ship.acc = 0.f;
			}

			// Apply vel and friction
			trans.setPosition(trans.getLocalPosition() + ship.vel.getVec3());
			ship.vel *= 0.98f;

			// Velocity limits
			{
				f32 v = ship.vel.length();

				if (v < 0.005f)
					ship.vel = 0.f;
				else if (v > ship.maxVel) {
					ship.vel.normalize();
					ship.vel *= ship.maxVel;
				}
			}

			// Move ships when goes out of screen
			{
				v2_f32 pos = trans.getLocalPosition().getVec2();
				f32 scale = trans.getLocalScale().getVec2().length() * 0.5f;

				i32 x = inScreenX(pos.x, scale);
				i32 y = inScreenY(pos.y, scale);

				v2_f32 cam = 0.5f * camSize();

				if (x == -1) pos.x = cam.x;
				else if (x == 1) pos.x = -cam.x;
				if (y == -1) pos.y = cam.y;
				else if (y == 1) pos.y = -cam.y;

				trans.setPosition(pos.getVec3());
			}
		}
	}

	// Collisions
	{
		// Bullets - Asteroids
		collideObjects<AsteroidComponent, BulletComponent>(0.5f, 0.35f,
			[](AsteroidComponent& a, BulletComponent& b)
		{
			f32 damage = std::min(a.health, b.damage);
			b.last = a.entity;

			a.health -= damage;

			if (!b.penetrate)
				b.damage -= damage;

			if (a.health == 0.f) {
				return false;
			}
			return true;
		});

		// Asteroids - Ships
		collideObjects<ShipComponent, AsteroidComponent>(0.35f, 0.35f,
			[this](ShipComponent& s, AsteroidComponent& a)
		{
			if (s.shipType == ShipType_Daddy) {
				destroyAsteroid(a);
			}
			else destroyShip(s);
			return false;
		});

		// Ships - Ships
		collideObjects<ShipComponent, ShipComponent>(0.35f, 0.35f,
			[this](ShipComponent& s0, ShipComponent& s1)
		{
			if (s0.entity == s1.entity || s0.shipType == ShipType_Daddy || s1.shipType == ShipType_Daddy)
				return true;

			destroyShip(s0);
			destroyShip(s1);
			return false;
		});

		// Bullets - Bullets
		collideObjects<BulletComponent, BulletComponent>(0.5f, 0.5f,
			[this](BulletComponent& b0, BulletComponent& b1)
		{
			if (b0.from == b1.from)
				return true;

			destroyBullet(b0);
			destroyBullet(b1);
			return false;
		});

		// Ships - Bullets
		collideObjects<ShipComponent, BulletComponent>(0.35f, 0.35f,
			[this](ShipComponent& s, BulletComponent& b)
		{
			if (b.from == s.entity)
				return true;

			f32 damage = std::min(s.health, b.damage);
			b.last = s.entity;

			s.health -= damage;

			if (!b.penetrate)
				b.damage -= damage;

			if (s.health == 0.f) {
				return false;
			}
			return true;
		});
	}
}

void SpaceState::render()
{
	scene.draw();
	// partsys_render(scene, getCamera(), scene.getGBuffer(), graphics_commandlist_get());
	SceneRenderer::present(ecs_component_get<CameraComponent>(scene, scene.getMainCamera())->gBuffer.offscreen);
}

Result SpaceState::close()
{
	scene.destroy();

	return Result_Success;
}

CameraComponent& SpaceState::getCamera()
{
	return *ecs_component_get<CameraComponent>(scene, scene.getMainCamera());
}


