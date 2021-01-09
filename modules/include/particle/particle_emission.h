#pragma once

#include "core/system/entity_system.h"
#include "core/rendering/render_utils.h"
#include "sprite/sprite_animator.h"
#include "core/utils/allocator.h"

namespace sv {

	enum ParticleEmitterFlag {
		ParticleEmitterFlag_Enable			= SV_BIT(0),
		ParticleEmitterFlag_Loop			= SV_BIT(1),
		ParticleEmitterFlag_LocalEmission	= SV_BIT(2),
	};
	typedef u32 ParticleEmitterFlags;

	enum Particle2DShape : u32 {

		Particle2DShape_Point,
		Particle2DShape_Quad,
		Particle2DShape_Circle,
		Particle2DShape_Ellipse,
		Particle2DShape_Cone,
		
	};

	enum Particle2DDraw : u32 {

		Particle2DDraw_Quad,
		Particle2DDraw_Circle,
		Particle2DDraw_Triangle,
		Particle2DDraw_Point,
		Particle2DDraw_Sprite,
		Particle2DDraw_AnimatedSprite,
		
	};

	struct Particle2D_CPU {

		v2_f32	position;
		v2_f32	velocity;
		f32		rotation;
		f32		angularVelocity;
		f32		size;
		f32		lifeTime;
		Color	color;

	};

	struct Particle2DEmitterComponent_CPU : public Component<Particle2DEmitterComponent_CPU> {

		ParticleEmitterFlags	flags = ParticleEmitterFlag_Enable | ParticleEmitterFlag_Loop;
		u32						maxParticles = 1000u;
		f32						duration = 0.f;
		f32						timer = 0.f;
		u32						renderLayer = 0u;
		Random					random;

		// EMISSION DATA

		f32 rateTime = 0.1f;
		f32 emissionTimer = 0.f;

		Particle2DShape shape = Particle2DShape_Point;
		v2_f32			shapeOffset;

		union {
			struct {
			} shapePoint;

			struct {
				v2_f32 size;
			} shapeQuad;

			struct {
				f32 radius;
			} shapeCircle;

			struct {
				v2_f32 radius;
			} shapeEllipse;

			struct {
				f32 direction;
				f32 angle;
			} shapeCone;
		};

		// START VALUES

		struct {

			f32 velocity0 = 0.1f;
			f32 velocity1 = 1.f;

			f32 rotation0 = 0.f;
			f32 rotation1 = PI * 2.f;

			f32 angularVelocity0 = 0.01f;
			f32 angularVelocity1 = 0.2f;

			f32 size0 = 0.4f;
			f32 size1 = 0.8f;

			f32 lifeTime0 = 1.f;
			f32 lifeTime1 = 2.f;

			u8 red0		= 255u;
			u8 red1		= 255u;
			u8 green0	= 255u;
			u8 green1	= 255u;
			u8 blue0	= 255u;
			u8 blue1	= 255u;

			u8 opacity0 = 100u;
			u8 opacity1 = 200u;

		} start;

		// RENDERING

		Particle2DDraw draw = Particle2DDraw_Quad;

		union {
			struct {
			} drawQuad;

			struct {
			} drawCircle;

			struct {
			} drawTriangle;

			struct {
			} drawPoint;

		};

		// This should be over the union because has reference count
		Sprite drawSprite;
		AnimatedSprite drawAnimatedSprite;
		
		// PARTICLES

		FrameList<Particle2D_CPU> particles;

		Particle2DEmitterComponent_CPU() { shapeEllipse.radius = {}; }
		~Particle2DEmitterComponent_CPU() { particles.clear(); }

		Particle2DEmitterComponent_CPU(const Particle2DEmitterComponent_CPU& copy);
		Particle2DEmitterComponent_CPU(Particle2DEmitterComponent_CPU&& copy) noexcept;

		void serialize(ArchiveO& file) {}
		void deserialize(ArchiveI& file) {}

	};

	void partsys_update(ECS* ecs, f32 dt);
	void partsys_render(ECS* ecs, Particle2DEmitterComponent_CPU& emitter, CameraData& cameraData, CommandList cmd);

}