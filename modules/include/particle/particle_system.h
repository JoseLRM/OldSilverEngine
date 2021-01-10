#pragma once

#include "core/rendering/render_utils.h"
#include "core/utils/allocator.h"
#include "core/system/task_system.h"	
#include "core/system/entity_system.h"	

#ifdef SV_MODULE_SPRITE
#include "sprite/sprite_animator.h"
#endif

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

#ifdef SV_MODULE_SPRITE
		Particle2DDraw_Sprite,
		Particle2DDraw_AnimatedSprite,
#endif

	};

	struct Particle2D {

		v2_f32	position;
		v2_f32	velocity;
		f32		rotation;
		f32		angularVelocity;
		f32		size;
		f32		lifeTime;
		Color	color;

	};

	struct Particle2DList {

		FrameList<Particle2D> begin;
		FrameList<Particle2D> current;

	};

	struct Particle2DEmissionData {

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

		Particle2DEmissionData() { shapeEllipse.radius = {}; }

		Particle2DEmissionData(const Particle2DEmissionData& copy);

	};

	struct Particle2DStartData {

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

		u8 red0 = 255u;
		u8 red1 = 255u;
		u8 green0 = 255u;
		u8 green1 = 255u;
		u8 blue0 = 255u;
		u8 blue1 = 255u;

		u8 opacity0 = 100u;
		u8 opacity1 = 200u;

	};

	struct Particle2DRenderData {

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

#ifdef SV_MODULE_SPRITE
		// This should be over the union because has reference count
		Sprite drawSprite;
		AnimatedSprite drawAnimatedSprite;
#endif

	};

	struct Particle2DFadeData {
		
		bool enabled = true;

		f32 fadeTime = 0.1f;

	};

	struct Particle2DEmitterCPU {

		ParticleEmitterFlags	flags = ParticleEmitterFlag_Enable | ParticleEmitterFlag_Loop;
		u32						maxParticles = 1000u;
		f32						duration = 0.f;
		f32						timer = 0.f;
		Random					random;

		Particle2DEmissionData	emission;
		Particle2DStartData		start;
		Particle2DRenderData	render;

		Particle2DFadeData fadeIn;
		Particle2DFadeData fadeOut;

		// PARTICLES

		Particle2DList particles;

	};

	struct Particle2DSystemCPU {

		std::vector<Particle2DEmitterCPU> emitters;

	};

	struct Particle2DInstance {
		
		v2_f32					position;
		f32						rotation;
		Particle2DSystemCPU*	pParticleSystem;

		Particle2DInstance() = default;
		Particle2DInstance(const v2_f32& position, f32 rotation, Particle2DSystemCPU* pParticleSystem)
			: position(position), rotation(rotation), pParticleSystem(pParticleSystem) {}

	};

	void partsys_update(const v2_f32& position, f32 rotation, Particle2DSystemCPU& particleSystem, ThreadContext* threadCtx, f32 dt);
	void partsys_render(const Particle2DInstance* instances, u32 instanceCount, const XMMATRIX& vpMatrix, GPUImage* offscreen, GPUImage* emissive, CommandList cmd);

	// COMPONENTS

	struct Particle2DEmitterComponent_CPU : public Component<Particle2DEmitterComponent_CPU> {

		Particle2DSystemCPU ps;
		u32	renderLayer = 0u; // TODO: Remove this

		void serialize(ArchiveO& file) {}
		void deserialize(ArchiveI& file) {}

	};

}