#include "core.h"

#include "particle_emission_internal.h"
#include "simulation/task_system.h"
#include "rendering/sprite_renderer.h"

namespace sv {

	struct {

		FrameList<SpriteInstance> sprites;

	} g_Context[GraphicsLimit_CommandList];

	Result partsys_initialize()
	{
		// Register components
		ecs_component_register<Particle2DEmitterComponent_CPU>("Particle2D CPU");

		return Result_Success;
	}

	Result partsys_close()
	{
		// Clear Context
		foreach(i, GraphicsLimit_CommandList) {
			
			auto& ctx = g_Context[i];

			ctx.sprites.clear();
		}

		return Result_Success;
	}

	static SV_INLINE void copydata_Particle2DEmitterComponent_CPU(Particle2DEmitterComponent_CPU& e, const Particle2DEmitterComponent_CPU& copy)
	{
		e.start = copy.start;
		e.draw = copy.draw;

		switch (e.draw)
		{
		case Particle2DDraw_Quad:
			e.drawQuad = copy.drawQuad;
			break;

		case Particle2DDraw_Circle:
			e.drawCircle = copy.drawCircle;
			break;

		case Particle2DDraw_Triangle:
			e.drawTriangle = copy.drawTriangle;
			break;

		case Particle2DDraw_Point:
			e.drawPoint = copy.drawPoint;
			break;

		case Particle2DDraw_Sprite:
			e.drawSprite = copy.drawSprite;
			break;

		case Particle2DDraw_AnimatedSprite:
			e.drawAnimatedSprite = copy.drawAnimatedSprite;
			break;

		default:
			SV_LOG_ERROR("Unknown particle draw type while copying");
			break;
		}

		e.random = copy.random;
		e.rateTime = copy.rateTime;
		e.duration = copy.duration;
		e.emissionTimer = copy.emissionTimer;
		e.flags = copy.flags;
		e.maxParticles = copy.maxParticles;
		e.rateTime = copy.rateTime;
		e.renderLayer = copy.renderLayer;
		e.shape = copy.shape;

		switch (e.shape)
		{
		case Particle2DShape_Point:
			e.shapePoint = copy.shapePoint;
			break;
		case Particle2DShape_Quad:
			e.shapeQuad = copy.shapeQuad;
			break;
		case Particle2DShape_Circle:
			e.shapeCircle = copy.shapeCircle;
			break;
		case Particle2DShape_Ellipse:
			e.shapeEllipse = copy.shapeEllipse;
			break;
		case Particle2DShape_Cone:
			e.shapeCone = copy.shapeCone;
			break;

		default:
			SV_LOG_ERROR("Unknown particle shape while copying");
			break;
		}
	}

	Particle2DEmitterComponent_CPU::Particle2DEmitterComponent_CPU(const Particle2DEmitterComponent_CPU& copy)
	{
		copydata_Particle2DEmitterComponent_CPU(*this, copy);
		particles = copy.particles;
	}

	Particle2DEmitterComponent_CPU::Particle2DEmitterComponent_CPU(Particle2DEmitterComponent_CPU&& copy) noexcept
	{
		copydata_Particle2DEmitterComponent_CPU(*this, copy);
		particles = std::move(copy.particles);
	}

	static SV_INLINE void genParticle(ECS* ecs, Particle2DEmitterComponent_CPU& e)
	{
		if (e.particles.size() >= e.maxParticles) return;

		Particle2D_CPU& p = e.particles.emplace_back();

		Transform trans = ecs_entity_transform_get(ecs, e.entity);
		v2_f32 pos = trans.getWorldPosition().getVec2();
		f32 rot = trans.getWorldEulerRotation().z;

		f32 velDir = 0.f;

		switch (e.shape)
		{
		case Particle2DShape_Point:
		{
			p.position = pos;
			velDir = e.random.gen_f32(0.f, PI * 2.f);
		} break;

		case Particle2DShape_Quad:
		{
			p.position.x = e.random.gen_f32(-e.shapeQuad.size.x, e.shapeQuad.size.x) * 0.5f + pos.x;
			p.position.y = e.random.gen_f32(-e.shapeQuad.size.y, e.shapeQuad.size.y) * 0.5f + pos.y;

			velDir = e.random.gen_f32(0.f, PI * 2.f);
		} break;

		case Particle2DShape_Circle:
		{
			// TODO: There are more ratio in the center
			f32 radiusPos = e.random.gen_f32(0.f, e.shapeCircle.radius);
			f32 radiusDir = e.random.gen_f32(0.f, 2.f * PI);

			p.position.x = cos(radiusDir) * radiusPos + pos.x;
			p.position.y = sin(radiusDir) * radiusPos + pos.y;
			velDir = e.random.gen_f32(0.f, PI * 2.f);
		} break;

		case Particle2DShape_Ellipse:
		{
			SV_LOG_ERROR("TODO-> Ellipse 2D Particle");
			velDir = e.random.gen_f32(0.f, PI * 2.f);
		} break;

		case Particle2DShape_Cone:
		{
			p.position = pos;
			velDir = e.random.gen_f32(e.shapeCone.angle * -0.5f, e.shapeCone.angle * 0.5f) + e.shapeCone.direction;
		} break;

		}

		p.position += e.shapeOffset;
		f32 velMag = e.random.gen_f32(e.start.velocity0, e.start.velocity1);
		p.velocity.x = cos(velDir + rot) * velMag;
		p.velocity.y = sin(velDir + rot) * velMag;

		p.rotation = e.random.gen_f32(e.start.rotation0, e.start.rotation1) + rot;
		p.angularVelocity = e.random.gen_f32(e.start.angularVelocity1, e.start.angularVelocity1);
		p.size = e.random.gen_f32(e.start.size0, e.start.size1);
		p.lifeTime = e.random.gen_f32(e.start.lifeTime0, e.start.lifeTime1);
		p.color.r = u8(e.random.gen_u32(e.start.red0, e.start.red1)); // TODO: u8 generator
		p.color.g = u8(e.random.gen_u32(e.start.green0, e.start.green1));
		p.color.b = u8(e.random.gen_u32(e.start.blue0, e.start.blue1));
		p.color.a = u8(e.random.gen_u32(e.start.opacity0, e.start.opacity1));
	}

	void partsys_update(ECS* ecs, f32 dt)
	{
		ThreadContext threadCtx;

		// UPDATE CPU 2D
		{
			EntityView<Particle2DEmitterComponent_CPU> emitters(ecs);
		
			for (Particle2DEmitterComponent_CPU& e : emitters) {
				
				if (e.flags & ParticleEmitterFlag_Enable) {

					// Update Duration
					if (!(e.flags & ParticleEmitterFlag_Loop)) {

						e.timer += dt;
						if (e.timer >= e.duration) {
							e.flags &= ~ParticleEmitterFlag_Enable;
							continue;
						}
					}

					// Emit particles
					e.emissionTimer += dt;

					while (e.emissionTimer >= e.rateTime) {
						e.emissionTimer -= e.rateTime;

						genParticle(ecs, e);
					}
				}

				// Update Particles: Velocity addition and lifetime reduction
				for (auto it = e.particles.begin(); it != e.particles.end();) {

					Particle2D_CPU& p = *it;

					p.position += p.velocity * dt;
					p.rotation += p.angularVelocity * dt;

					p.lifeTime -= dt;
					if (p.lifeTime <= 0.f) {

						if (it == (--e.particles.end())) {
							e.particles.pop_back();
							break;
						}
						else {
							std::swap(*it, e.particles.back());
							e.particles.pop_back();
						}
					}
					else ++it;
				}
			}
		}

		// TODO: UPDATE CPU 3D
		{

		}

		task_wait(threadCtx);
	}

	void partsys_render(ECS* ecs, Particle2DEmitterComponent_CPU& emitter, CameraData& cameraData, CommandList cmd)
	{
		if (emitter.particles.empty()) return;

		auto& ctx = g_Context[cmd];

		Transform trans = ecs_entity_transform_get(ecs, emitter.entity);

		switch (emitter.draw)
		{

		case Particle2DDraw_Quad:
			SV_LOG_ERROR("TODO");
			break;

		case Particle2DDraw_Circle:
			SV_LOG_ERROR("TODO");
			break;

		case sv::Particle2DDraw_Triangle:
			SV_LOG_ERROR("TODO");
			break;

		case sv::Particle2DDraw_Point:
			SV_LOG_ERROR("TODO");
			break;

		case Particle2DDraw_Sprite:
		case Particle2DDraw_AnimatedSprite:
		{
			GPUImage* image;
			v4_f32 texCoord;

			if (emitter.draw == Particle2DDraw_Sprite) {
				texCoord = emitter.drawSprite.texCoord;
				image = emitter.drawSprite.texture.get();
			}
			else {
				Sprite spr = emitter.drawAnimatedSprite.getSprite();
				texCoord = spr.texCoord;
				image = spr.texture.get();
			}

			// TODO: Material
			// TODO: Use SameSprites

			SpriteRendererDrawDesc desc;
			desc.pCameraData = &cameraData;
			desc.pSprites = ctx.sprites.data();
			desc.spriteCount = u32(ctx.sprites.size());
			desc.pLights = nullptr; // TODO: Particle 2D Lighting
			desc.lightCount = 0u;
			desc.lightMult = 1.f;
			desc.ambientLight = { 1.f, 1.f, 1.f };
			desc.depthTest = false;

			ctx.sprites.reset();
			XMMATRIX tm;

			for (Particle2D_CPU& p : emitter.particles) {

				tm = XMMatrixScaling(p.size, p.size, 0.f) * 
					XMMatrixRotationZ(p.rotation) *
					XMMatrixTranslation(p.position.x, p.position.y, 0.f);

				ctx.sprites.emplace_back(tm, texCoord, nullptr, image, p.color);
			}

			SpriteRenderer::drawSprites(&desc, cmd);

		} break;

		default:
			break;
		}
	}

}