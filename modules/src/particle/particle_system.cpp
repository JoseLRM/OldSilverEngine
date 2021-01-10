#include "core.h"

#include "particle_system_internal.h"
#include "core/system/task_system.h"
#include "sprite/sprite.h"

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

	Particle2DEmissionData::Particle2DEmissionData(const Particle2DEmissionData& copy)
	{
		rateTime = copy.rateTime;
		emissionTimer = copy.emissionTimer;
		rateTime = copy.rateTime;
		shape = copy.shape;

		switch (shape)
		{
		case Particle2DShape_Point:
			shapePoint = copy.shapePoint;
			break;
		case Particle2DShape_Quad:
			shapeQuad = copy.shapeQuad;
			break;
		case Particle2DShape_Circle:
			shapeCircle = copy.shapeCircle;
			break;
		case Particle2DShape_Ellipse:
			shapeEllipse = copy.shapeEllipse;
			break;
		case Particle2DShape_Cone:
			shapeCone = copy.shapeCone;
			break;

		default:
			SV_LOG_ERROR("Unknown particle shape while copying");
			break;
		}
	}

	static SV_INLINE void genParticle(const v2_f32& pos, f32 rot, Particle2DSystemCPU& ps, Particle2DEmitterCPU& e)
	{
		if (e.particles.current.size() >= e.maxParticles) return;

		Particle2D& p = e.particles.current.emplace_back();
		Particle2D& beginP = e.particles.begin.emplace_back();

		f32 velDir = 0.f;

		switch (e.emission.shape)
		{
		case Particle2DShape_Point:
		{
			p.position = pos;
			velDir = e.random.gen_f32(0.f, PI * 2.f);
		} break;

		case Particle2DShape_Quad:
		{
			p.position.x = e.random.gen_f32(-e.emission.shapeQuad.size.x, e.emission.shapeQuad.size.x) * 0.5f + pos.x;
			p.position.y = e.random.gen_f32(-e.emission.shapeQuad.size.y, e.emission.shapeQuad.size.y) * 0.5f + pos.y;

			velDir = e.random.gen_f32(0.f, PI * 2.f);
		} break;

		case Particle2DShape_Circle:
		{
			// TODO: There are more ratio in the center
			f32 radiusPos = e.random.gen_f32(0.f, e.emission.shapeCircle.radius);
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
			velDir = e.random.gen_f32(e.emission.shapeCone.angle * -0.5f, e.emission.shapeCone.angle * 0.5f) + e.emission.shapeCone.direction;
		} break;

		}

		p.position += e.emission.shapeOffset;
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

		beginP = p;
	}

	void partsys_update(const v2_f32& position, f32 rotation, Particle2DSystemCPU& ps, ThreadContext* threadCtx, f32 dt)
	{
		// TODO: Multithreaded

		for (Particle2DEmitterCPU& e : ps.emitters) {
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
				e.emission.emissionTimer += dt;

				while (e.emission.emissionTimer >= e.emission.rateTime) {
					e.emission.emissionTimer -= e.emission.rateTime;

					genParticle(position, rotation, ps, e);
				}
			}

			// Update Fade
			if (e.fadeIn.enabled) {

				

			}
			if (e.fadeOut.enabled) {

				for (u32 i = 0u; i < e.particles.current.size(); ++i) {
					
					Particle2D& p = e.particles.current[i];
					Particle2D& b = e.particles.begin[i];

					if (p.lifeTime < e.fadeOut.fadeTime) {

						p.color.a = u8(f32(b.color.a) * (p.lifeTime / std::min(b.lifeTime, e.fadeOut.fadeTime)));
						if (p.color.a)
							p.color.a--;
					}
				}
			}

			// Update Particles: Velocity addition and lifetime reduction
			for (auto it = e.particles.current.begin(); it != e.particles.current.end();) {

				Particle2D& p = *it;

				p.position += p.velocity * dt;
				p.rotation += p.angularVelocity * dt;

				p.lifeTime -= dt;
				if (p.lifeTime <= 0.f) {

					if (it == (--e.particles.current.end())) {
						e.particles.current.pop_back();
						e.particles.begin.pop_back();
						break;
					}
					else {
						std::swap(*it, e.particles.current.back());
						std::swap(e.particles.begin[it.ptr - e.particles.current.begin().ptr], e.particles.begin.back());
						e.particles.current.pop_back();
					}
				}
				else ++it;
			}
		}
	}

	void partsys_render(const Particle2DInstance* instances, u32 instanceCount, const XMMATRIX& vpMatrix, GPUImage* offscreen, GPUImage* emissive, CommandList cmd)
	{
		if (instanceCount == 0u) return;

		auto& ctx = g_Context[cmd];

		ctx.sprites.reset();

		foreach(i, instanceCount) {

			Particle2DSystemCPU& ps = *instances[i].pParticleSystem;

			if (ps.emitters.empty()) return;

			for (Particle2DEmitterCPU& emitter : ps.emitters) {

				if (emitter.particles.current.empty()) continue;

				switch (emitter.render.draw)
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

					if (emitter.render.draw == Particle2DDraw_Sprite) {
						texCoord = emitter.render.drawSprite.texCoord;
						image = emitter.render.drawSprite.texture.get();
					}
					else {
						Sprite spr = emitter.render.drawAnimatedSprite.getSprite();
						texCoord = spr.texCoord;
						image = spr.texture.get();
					}

					// TODO: Material
					// TODO: Use SameSprites

					XMMATRIX tm;

					for (Particle2D& p : emitter.particles.current) {

						tm = XMMatrixScaling(p.size, p.size, 0.f) *
							XMMatrixRotationZ(p.rotation) *
							XMMatrixTranslation(p.position.x, p.position.y, 0.f);

						ctx.sprites.emplace_back(tm, texCoord, image, p.color);
					}

					

				} break;

				default:
					break;
				}
			}
		}

		// Draw Sprites
		if (ctx.sprites.size()) {

			draw_sprites(ctx.sprites.data(), ctx.sprites.size(), vpMatrix, offscreen, emissive, cmd);
		}
	}

}