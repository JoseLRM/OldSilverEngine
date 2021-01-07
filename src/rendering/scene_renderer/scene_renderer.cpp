#include "core.h"

#include "scene_renderer_internal.h"
#include "rendering/postprocessing.h"
#include "simulation/particle_emission.h"

namespace sv {

	static SceneRendererContext g_Context[GraphicsLimit_CommandList];

	RenderLayer2D SceneRenderer::renderLayers2D[RENDERLAYER_COUNT];
	u32 SceneRenderer::renderLayerOrder2D[RENDERLAYER_COUNT];
	RenderLayer3D SceneRenderer::renderLayers3D[RENDERLAYER_COUNT];

	Result SceneRenderer_internal::initialize()
	{
		// Initialize Render Layers
		{
			for (u32 i = 0u; i < RENDERLAYER_COUNT; ++i) {

				RenderLayer2D& rl = SceneRenderer::renderLayers2D[i];

				rl.sortValue = i;
				rl.lightMult = 1.f;
				rl.ambientMult = 1.f;
				rl.frustumTest = true;
				rl.enabled = false;

				switch (i)
				{
				case 0:
					rl.name = "Default";
					break;

				default:
					rl.name = "Unnamed ";
					rl.name += std::to_string(i);
					break;
				}
			}
			SceneRenderer::renderLayers2D[0].enabled = true;

			for (u32 i = 0u; i < RENDERLAYER_COUNT; ++i) {

				RenderLayer3D& rl = SceneRenderer::renderLayers3D[i];

				switch (i)
				{
				case 0:
					rl.name = "Default";
					break;

				default:
					rl.name = "Unnamed ";
					rl.name += std::to_string(i);
					break;
				}
			}
		}

		// Set renderLayerSort indices
		{
			for (u32 i = 0u; i < RENDERLAYER_COUNT; ++i) {
				SceneRenderer::renderLayerOrder2D[i] = i;
			}
		}

		return Result_Success;
	}

	Result SceneRenderer_internal::close()
	{
		// Destroy Context
		foreach(i, GraphicsLimit_CommandList) {
			SceneRendererContext& ctx = g_Context[i];

			ctx.spritesInstances.clear();
			ctx.meshInstances.clear();
			ctx.meshGroups.clear();
		}
		

		// Free heap memory from renderlayers
		{
			for (u32 i = 0u; i < RENDERLAYER_COUNT; ++i) {

				RenderLayer2D& rl = SceneRenderer::renderLayers2D[i];
				rl.name.clear();
			}

			for (u32 i = 0u; i < RENDERLAYER_COUNT; ++i) {

				RenderLayer3D& rl = SceneRenderer::renderLayers3D[i];
				rl.name.clear();
			}
		}

		return Result_Success;
	}

	// RENDERING UTILS

	inline bool frustumTest(const FrustumOthographic& frustum, Transform trans)
	{
		Circle2D circle;
		circle.position = trans.getWorldPosition().getVec2();
		circle.radius = trans.getWorldScale().getVec2().length() / 2.f;

		return frustum.intersects_circle(circle);
	}

	// SCENE RENDERING

	void SceneRenderer::prepareRenderLayers2D()
	{
		// Sort render layers.
		{
			std::sort(renderLayerOrder2D, renderLayerOrder2D + RENDERLAYER_COUNT, [](u32 i0, u32 i1)
			{
				const RenderLayer2D& rl0 = renderLayers2D[i0];
				const RenderLayer2D& rl1 = renderLayers2D[i1];

				return rl0.sortValue < rl1.sortValue;
			});
		}
	}

	void sv::SceneRenderer::clearGBuffer(GBuffer& gBuffer, Color4f color, f32 depth, u32 stencil, CommandList cmd)
	{
		graphics_image_clear(gBuffer.offscreen, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, color, 1.f, 0u, cmd);
		graphics_image_clear(gBuffer.diffuse, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, color, depth, stencil, cmd);
		graphics_image_clear(gBuffer.normal, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, color, depth, stencil, cmd);
		graphics_image_clear(gBuffer.depthStencil, GPUImageLayout_DepthStencil, GPUImageLayout_DepthStencil, color, depth, stencil, cmd);
	}

	void SceneRenderer::processLighting(ECS* ecs, LightSceneData& lightData)
	{
		//TODO: In 2D do frustum culling to discard lights out of the camera

		// Light Instances
		FrameList<LightInstance>& lights = lightData.lights;
		lights.reset();
		{
			EntityView<LightComponent> comp(ecs);
			for (LightComponent& light : comp) {

				Transform trans = ecs_entity_transform_get(ecs, light.entity);

				lights.emplace_back(trans.getWorldPosition(), light.color, light.point.range, light.intensity, light.point.smoothness);
			}
		}

		// COMPUTE AMBIENT LIGHTING
		{
			Color3f& ambient = lightData.ambientLight;
			ambient = Color3f::Black();

			EntityView<SkyComponent> comp(ecs);
			for (SkyComponent& sky : comp) {
				ambient.r += sky.ambient.r;
				ambient.g += sky.ambient.g;
				ambient.b += sky.ambient.b;
			}

			ambient.r = std::max(std::min(ambient.r, 1.f), 0.f);
			ambient.g = std::max(std::min(ambient.g, 1.f), 0.f);
			ambient.b = std::max(std::min(ambient.b, 1.f), 0.f);
		}
	}

	void SceneRenderer::drawSprites2D(ECS* ecs, CameraData& cameraData, LightSceneData& lightData, u32 index, CommandList cmd)
	{
		const RenderLayer2D& rl = SceneRenderer::renderLayers2D[index];

		if (!rl.enabled)
			return;

		SceneRendererContext& ctx = g_Context[cmd];

		// Reset FrameLists
		ctx.spritesInstances.reset();

		bool noAmbient = rl.ambientMult <= 0.f || (lightData.ambientLight.r == 0.f && lightData.ambientLight.g == 0.f && lightData.ambientLight.b == 0.f);
		bool noLight = rl.lightMult <= 0.f || lightData.lights.empty();

		if (noAmbient && noLight) return;

		// Sprite Rendering
		{
			FrustumOthographic frustum;
			frustum.init_center(cameraData.position.getVec2(), { cameraData.pProjection->width, cameraData.pProjection->height });

			EntityView<SpriteComponent> sprites(ecs);
			FrameList<SpriteInstance>& instances = ctx.spritesInstances;

			// Add sprites to instance list
			for (SpriteComponent& sprite : sprites) {

				if (sprite.renderLayer != index) continue;

				Transform trans = ecs_entity_transform_get(ecs, sprite.entity);

				bool draw = false;

				// Frustrum culling
				if (rl.frustumTest) {
					draw = frustumTest(frustum, trans);
				}
				else draw = true;

				if (draw) instances.emplace_back(trans.getWorldMatrix(), sprite.sprite.texCoord, sprite.material.get(), sprite.sprite.texture.get(), sprite.color);
			}

			EntityView<AnimatedSpriteComponent> animatedSprites(ecs);

			// Add animated sprites to instance list
			for (AnimatedSpriteComponent& anim : animatedSprites) {

				if (anim.renderLayer != index) continue;

				Transform trans = ecs_entity_transform_get(ecs, anim.entity);

				bool draw = false;

				// Frustrum culling
				if (rl.frustumTest) {
					draw = frustumTest(frustum, trans);
				}
				else draw = true;

				Sprite& spr = anim.sprite.getSprite();
				if (draw) instances.emplace_back(trans.getWorldMatrix(), spr.texCoord, anim.material.get(), spr.texture.get(), anim.color);
			}

			// If there are something
			if (instances.size()) {

				// Draw sprites
				SpriteRendererDrawDesc desc;
				desc.pCameraData = &cameraData;
				desc.pSprites = ctx.spritesInstances.data();
				desc.spriteCount = u32(ctx.spritesInstances.size());
				desc.pLights = lightData.lights.data();
				desc.lightCount = u32(lightData.lights.size());
				desc.lightMult = rl.lightMult;
				desc.ambientLight = lightData.ambientLight;
				desc.ambientLight.r *= rl.ambientMult;
				desc.ambientLight.g *= rl.ambientMult;
				desc.ambientLight.b *= rl.ambientMult;
				desc.depthTest = false;
				desc.transparent = false;

				SpriteRenderer::drawSprites(&desc, cmd);
			}
		}
	}

	void sv::SceneRenderer::drawParticles2D(ECS* ecs, CameraData& cameraData, LightSceneData& lightData, u32 index, CommandList cmd)
	{
		if (!renderLayers2D[index].enabled) return;

		EntityView<Particle2DEmitterComponent_CPU> particles(ecs);

		for (Particle2DEmitterComponent_CPU& emitter : particles) {

			if (emitter.renderLayer != index) continue;

			partsys_render(ecs, emitter, cameraData, cmd);
		}
	}

	void sv::SceneRenderer::drawSprites3D(ECS* ecs, CameraData& cameraData, LightSceneData& lightData, CommandList cmd)
	{
		SceneRendererContext& ctx = g_Context[cmd];

		// Reset FrameLists
		ctx.spritesInstances.reset();

		bool noAmbient = lightData.ambientLight.r == 0.f && lightData.ambientLight.g == 0.f && lightData.ambientLight.b == 0.f;
		bool noLight = lightData.lights.empty();

		if (noAmbient && noLight) return;

		EntityView<SpriteComponent> sprites(ecs);
		FrameList<SpriteInstance>& instances = ctx.spritesInstances;

		// Add sprites to instance list
		for (SpriteComponent& sprite : sprites) {

			Transform trans = ecs_entity_transform_get(ecs, sprite.entity);
			instances.emplace_back(trans.getWorldMatrix(), sprite.sprite.texCoord, sprite.material.get(), sprite.sprite.texture.get(), sprite.color);
		}

		EntityView<AnimatedSpriteComponent> animatedSprites(ecs);

		// Add animated sprites to instance list
		for (AnimatedSpriteComponent& anim : animatedSprites) {

			Transform trans = ecs_entity_transform_get(ecs, anim.entity);
			Sprite& spr = anim.sprite.getSprite();
			instances.emplace_back(trans.getWorldMatrix(), spr.texCoord, anim.material.get(), spr.texture.get(), anim.color);
		}

		// If there are something
		if (instances.size()) {

			// Draw sprites
			SpriteRendererDrawDesc desc;
			desc.pCameraData = &cameraData;
			desc.pSprites = ctx.spritesInstances.data();
			desc.spriteCount = u32(ctx.spritesInstances.size());
			desc.pLights = lightData.lights.data();
			desc.lightCount = u32(lightData.lights.size());
			desc.lightMult = 1.f;
			desc.ambientLight = lightData.ambientLight;
			desc.depthTest = true;
			desc.transparent = false;

			SpriteRenderer::drawSprites(&desc, cmd);
		}
	}

	void sv::SceneRenderer::drawMeshes3D(ECS* ecs, CameraData& cameraData, LightSceneData& lightData, CommandList cmd)
	{
		SceneRendererContext& ctx = g_Context[cmd];

		FrameList<MeshInstance>& meshes = ctx.meshInstances;
		FrameList<LightInstance>& lights = lightData.lights;
		FrameList<MeshInstanceGroup>& meshesGroup = ctx.meshGroups;
		meshes.reset();
		meshesGroup.reset();

		EntityView<MeshComponent> meshesComp(ecs);
		for (MeshComponent& mesh : meshesComp) {

			Mesh* m = mesh.mesh.get();
			if (m) {

				Transform trans = ecs_entity_transform_get(ecs, mesh.entity);
				meshes.emplace_back(trans.getWorldMatrix(), m, mesh.material.get(), 10.f);
			}
		}

		meshesGroup.emplace_back(&meshes, RasterizerCullMode_Back, false);

		MeshRenderer::drawMeshes(cameraData.pGBuffer->offscreen, *cameraData.pGBuffer, *cameraData.pCameraBuffer, meshesGroup, lights, true, true, cmd);
	}

	void sv::SceneRenderer::present(GPUImage* image)
	{
		if (image == nullptr) return;
		GPUImageRegion region;
		region.offset = { 0u, 0u, 0u };
		region.size = { graphics_image_get_width(image), graphics_image_get_height(image), 1u };

		graphics_present(image, region, GPUImageLayout_RenderTarget, graphics_commandlist_get());
	}

	// COMPONENTS

	void SpriteComponent::serialize(ArchiveO& file)
	{
		file << color;
		file << sprite.texCoord;
		file << renderLayer;
		sprite.texture.save(file);
		material.save(file);
	}

	void SpriteComponent::deserialize(ArchiveI& file)
	{
		file >> color;
		file >> sprite.texCoord;
		file >> renderLayer;

		sprite.texture.load(file);
		material.load(file);
	}

	void AnimatedSpriteComponent::serialize(ArchiveO& file)
	{
		file << color;
		file << sprite.getState();
		material.save(file);
		file << renderLayer;
	}

	void AnimatedSpriteComponent::deserialize(ArchiveI& file)
	{
		file >> color;

		AnimatedSprite::State sprState;
		file >> sprState;
		if (result_fail(sprite.setState(sprState))) {
			SV_LOG_ERROR("Sprite Animation not found");
		}

		if (result_fail(material.load(file))) {
			SV_LOG_ERROR("Material in AnimatedSprite not found");
		}

		file >> renderLayer;
	}

	void MeshComponent::serialize(ArchiveO& file)
	{
		mesh.save(file);
		material.save(file);
	}

	void MeshComponent::deserialize(ArchiveI& file)
	{
		mesh.load(file);
		material.load(file);
	}

	void LightComponent::serialize(ArchiveO& file)
	{
		file << intensity << color << lightType;
		switch (lightType)
		{
		case LightType_Point:
			file << point;
			break;
		}
	}

	void LightComponent::deserialize(ArchiveI& file)
	{
		file >> intensity >> color >> lightType;
		switch (lightType)
		{
		case LightType_Point:
			file >> point;
			break;
		}
	}

	void SkyComponent::serialize(ArchiveO& file)
	{
		file << ambient;
	}

	void SkyComponent::deserialize(ArchiveI& file)
	{
		file >> ambient;
	}

	void CameraComponent::serialize(ArchiveO& file)
	{
		file << projection.width << projection.height << projection.near << projection.far << projection.projectionType;
		file << gBuffer.getResolution();
	}

	void CameraComponent::deserialize(ArchiveI& file)
	{
		file >> projection.width >> projection.height >> projection.near >> projection.far >> projection.projectionType;
		v2_u32 res;
		file >> res;
		gBuffer.create(res.x, res.y);
	}

}