#include "core.h"

#include "scene_renderer_internal.h"
#include "rendering/postprocessing.h"

namespace sv {

	static SceneRendererContext g_Context;

	RenderLayer2D SceneRenderer::renderLayers2D[RENDERLAYER_COUNT];
	RenderLayer3D SceneRenderer::renderLayers3D[RENDERLAYER_COUNT];

	static u32 g_RenderLayerOrder2D[RENDERLAYER_COUNT];

	Result SceneRenderer_internal::initialize()
	{
		// Initialize Render Layers
		{
			for (u32 i = 0u; i < RENDERLAYER_COUNT; ++i) {

				RenderLayer2D& rl = SceneRenderer::renderLayers2D[i];

				rl.frustumTest = true;
				rl.sortValue = i;
				rl.lightMult = 1.f;
				rl.ambientMult = 1.f;

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
				g_RenderLayerOrder2D[i] = i;
			}
		}

		return Result_Success;
	}

	Result SceneRenderer_internal::close()
	{
		// Destroy Context
		SceneRendererContext& ctx = g_Context;

		svCheck(ctx.cameraBuffer.clear());
		ctx.spritesInstances.clear();

		svCheck(ctx.gBuffer.destroy());

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

	void drawCamera(Camera* pCamera, ECS* ecs, const vec3f& camPosition, const vec4f& camRotation);
	void prepareRenderLayers();
	void prepareLights(ECS* ecs);
	void present(GPUImage* image);

	// 2D

	void drawLayers(Camera* pCamera, const vec3f& camPosition, ECS* ecs, CommandList cmd);
	void drawLayer(Camera* pCamera, const vec3f& camPosition, ECS* ecs, u32 renderLayerIndex, CommandList cmd);

	// 3D

	void drawSprites(Camera* pCamera, const vec3f& camPosition, ECS* ecs, CommandList cmd);
	void drawMeshes(Camera* pCamera, const vec3f& camPosition, ECS* ecs, CommandList cmd);

	void SceneRenderer::draw(ECS* ecs, Entity mainCamera)
	{
		SceneRendererContext& ctx = g_Context;

		prepareRenderLayers();
		prepareLights(ecs);

		// Draw Cameras
		EntityView<CameraComponent> cameras(ecs);

		for (CameraComponent& camera : cameras) {

			Transform trans = ecs_entity_transform_get(ecs, camera.entity);

			drawCamera(&camera.camera, ecs, trans.getWorldPosition(), trans.getWorldRotation());

			if (camera.entity == mainCamera) {
				present(camera.camera.getOffscreen());
			}
		}
	}

	void SceneRenderer::drawDebug(ECS* ecs, Entity mainCamera, bool drawECSCameras, bool present_, u32 cameraCount, Camera** pCameras, vec3f* camerasPosition, vec4f* camerasRotation)
	{
		SceneRendererContext& ctx = g_Context;

		prepareRenderLayers();
		prepareLights(ecs);

		// Draw Cameras
		if (drawECSCameras) {
			EntityView<CameraComponent> cameras(ecs);

			for (CameraComponent& camera : cameras) {

				Transform trans = ecs_entity_transform_get(ecs, camera.entity);

				drawCamera(&camera.camera, ecs, trans.getWorldPosition(), trans.getWorldRotation());

				if (present_ && camera.entity == mainCamera) {
					present(camera.camera.getOffscreen());
				}
			}
		}

		for (u32 i = 0u; i < cameraCount; ++i) {

			Camera* pCamera = pCameras[i];
			drawCamera(pCamera, ecs, camerasPosition[i], camerasRotation[i]);
		}
	}

	void drawCamera(Camera* pCamera, ECS* ecs, const vec3f& camPosition, const vec4f& camRotation)
	{
		SceneRendererContext& ctx = g_Context;

		if (!pCamera->isActive()) return;

		// Compute View Matrix
		XMMATRIX viewMatrix = math_matrix_view(camPosition, camRotation);

		const XMMATRIX& projectionMatrix = pCamera->getProjectionMatrix();

		// Begin command list
		CommandList cmd = graphics_commandlist_begin();

		graphics_event_begin("Scene Rendering", cmd);

		graphics_viewport_set(pCamera->getViewport(), 0u, cmd);
		graphics_scissor_set(pCamera->getScissor(), 0u, cmd);

		// Update camera buffer
		{
			ctx.cameraBuffer.viewMatrix = viewMatrix;
			ctx.cameraBuffer.projectionMatrix = projectionMatrix;
			ctx.cameraBuffer.position = camPosition;
			ctx.cameraBuffer.direction = camRotation;

			ctx.cameraBuffer.updateGPU(cmd);
		}

		// Offscreen and gBuffer
		GPUImage* rt = pCamera->getOffscreen();
		{
			graphics_image_clear(rt, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, { 0.f, 0.f, 0.f, 1.f }, 1.f, 0u, cmd);

			if (ctx.gBuffer.diffuse == nullptr)
				ctx.gBuffer.create(1920u, 1080u);
			else {
				graphics_image_clear(ctx.gBuffer.diffuse, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, { 0.f, 0.f, 0.f, 1.f }, 1.f, 0u, cmd);
				graphics_image_clear(ctx.gBuffer.normal, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, { 0.f, 0.f, 0.f, 1.f }, 1.f, 0u, cmd);
				graphics_image_clear(ctx.gBuffer.depthStencil, GPUImageLayout_DepthStencil, GPUImageLayout_DepthStencil, { 0.f, 0.f, 0.f, 1.f }, 1.f, 0u, cmd);
			}
		}

		// Draw Render Components
		switch (pCamera->getCameraType())
		{
		case CameraType_2D:
			drawLayers(pCamera, camPosition, ecs, cmd);
			break;
		}

		// PostProcessing

		GPUImage* off = pCamera->getOffscreen();

		const CameraBloomData& bloom = pCamera->getBloom();
		const CameraToneMappingData& toneMapping = pCamera->getToneMapping();

		GPUBarrier barrier;

		barrier = GPUBarrier::Image(off, GPUImageLayout_RenderTarget, GPUImageLayout_ShaderResource);
		graphics_barrier(&barrier, 1u, cmd);

		if (bloom.enabled) {
			PostProcessing::bloom(off, GPUImageLayout_ShaderResource, GPUImageLayout_ShaderResource, bloom.threshold, bloom.blurRange, bloom.blurIterations, cmd);
		}

		if (toneMapping.enabled) {
			PostProcessing::toneMapping(off, GPUImageLayout_ShaderResource, GPUImageLayout_ShaderResource, toneMapping.exposure, cmd);
		}

		barrier = GPUBarrier::Image(off, GPUImageLayout_ShaderResource, GPUImageLayout_RenderTarget);
		graphics_barrier(&barrier, 1u, cmd);

		graphics_event_end(cmd);
	}

	void prepareRenderLayers()
	{
		SceneRendererContext& ctx = g_Context;

		// Sort render layers.
		{
			std::sort(g_RenderLayerOrder2D, g_RenderLayerOrder2D + RENDERLAYER_COUNT, [](u32 i0, u32 i1)
			{
				const RenderLayer2D& rl0 = SceneRenderer::renderLayers2D[i0];
				const RenderLayer2D& rl1 = SceneRenderer::renderLayers2D[i1];

				return rl0.sortValue < rl1.sortValue;
			});
		}
	}

	void prepareLights(ECS* ecs)
	{
		SceneRendererContext& ctx = g_Context;

		//TODO: In 2D do frustum culling to discard lights out of the camera

		// Light Instances
		FrameList<LightInstance>& lights = ctx.lightInstances;
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
			Color3f& ambient = ctx.ambientLight;
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

	void drawLayers(Camera* pCamera, const vec3f& camPosition, ECS* ecs, CommandList cmd)
	{
		SceneRendererContext& ctx = g_Context;

		for (u32 i = 0u; i < RENDERLAYER_COUNT; ++i) {

			u32 index = g_RenderLayerOrder2D[i];

			drawLayer(pCamera, camPosition, ecs, index, cmd);
		}
	}

	void drawLayer(Camera* pCamera, const vec3f& camPosition, ECS* ecs, u32 renderLayerIndex, CommandList cmd)
	{
		SceneRendererContext& ctx = g_Context;
		const RenderLayer2D& rl = SceneRenderer::renderLayers2D[renderLayerIndex];

		// Reset FrameLists
		ctx.spritesInstances.reset();

		bool noAmbient = rl.ambientMult <= 0.f || (ctx.ambientLight.r == 0.f && ctx.ambientLight.g == 0.f && ctx.ambientLight.b == 0.f);
		bool noLight = rl.lightMult <= 0.f || ctx.lightInstances.empty();

		if (noAmbient && noLight) return;

		// Sprite Rendering
		{
			FrustumOthographic frustum;
			frustum.init_center(camPosition.getVec2(), { pCamera->getWidth(), pCamera->getHeight() });

			EntityView<SpriteComponent> sprites(ecs);
			FrameList<SpriteInstance>& instances = ctx.spritesInstances;

			// Add sprites to instance list
			for (SpriteComponent& sprite : sprites) {

				if (sprite.renderLayer != renderLayerIndex) continue;

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

				if (anim.renderLayer != renderLayerIndex) continue;

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
				desc.renderTarget = pCamera->getOffscreen();
				desc.pGBuffer = &ctx.gBuffer;
				desc.pCameraBuffer = &ctx.cameraBuffer;
				desc.pSprites = ctx.spritesInstances.data();
				desc.spriteCount = u32(ctx.spritesInstances.size());
				desc.pLights = ctx.lightInstances.data();
				desc.lightCount = u32(ctx.lightInstances.size());
				desc.lightMult = rl.lightMult;
				desc.ambientLight = ctx.ambientLight;
				desc.ambientLight.r *= rl.ambientMult;
				desc.ambientLight.g *= rl.ambientMult;
				desc.ambientLight.b *= rl.ambientMult;

				SpriteRenderer::drawSprites(&desc, cmd);
			}
		}
	}

	void drawSprites(Camera* pCamera, const vec3f& camPosition, ECS* ecs, CommandList cmd)
	{
		SceneRendererContext& ctx = g_Context;

		SV_LOG_ERROR("TODO-> 3D Sprite Rendering");
	}

	void drawMeshes(Camera* pCamera, const vec3f& camPosition, ECS* ecs, CommandList cmd)
	{
		SceneRendererContext& ctx = g_Context;

		FrameList<MeshInstance>& meshes = ctx.meshInstances;
		FrameList<LightInstance>& lights = ctx.lightInstances;
		FrameList<MeshInstanceGroup>& meshesGroup = ctx.meshGroups;
		GBuffer& gBuffer = ctx.gBuffer;
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

		//MeshRenderer::drawMeshes(pCamera->getOffscreenRT(), gBuffer, ctx.cameraBuffer, meshesGroup, lights, true, true, cmd);
	}

	void present(GPUImage* image)
	{
		if (image == nullptr) return;
		GPUImageRegion region;
		region.offset = { 0u, 0u, 0u };
		region.size = { graphics_image_get_width(image), graphics_image_get_height(image), 1u };

		graphics_present(image, region, GPUImageLayout_RenderTarget, graphics_commandlist_get());
	}

	void SceneRenderer::initECS(ECS* ecs)
	{
		ecs_register<SpriteComponent>(ecs, scene_component_serialize_SpriteComponent, scene_component_deserialize_SpriteComponent);
		ecs_register<AnimatedSpriteComponent>(ecs, scene_component_serialize_AnimatedSpriteComponent, scene_component_deserialize_AnimatedSpriteComponent);
		ecs_register<MeshComponent>(ecs, scene_component_serialize_MeshComponent, scene_component_deserialize_MeshComponent);
		ecs_register<LightComponent>(ecs, scene_component_serialize_LightComponent, scene_component_deserialize_LightComponent);
		ecs_register<SkyComponent>(ecs, scene_component_serialize_SkyComponent, scene_component_deserialize_SkyComponent);
		ecs_register<CameraComponent>(ecs, scene_component_serialize_CameraComponent, scene_component_deserialize_CameraComponent);
	}

	// COMPONENTS

	void scene_component_serialize_SpriteComponent(BaseComponent* comp_, ArchiveO& archive)
	{
		SpriteComponent* comp = reinterpret_cast<SpriteComponent*>(comp_);
		archive << comp->color;
		archive << comp->sprite.texCoord;
		archive << comp->renderLayer;
		comp->sprite.texture.save(archive);
		comp->material.save(archive);
	}

	void scene_component_serialize_AnimatedSpriteComponent(BaseComponent* comp_, ArchiveO& archive)
	{
		AnimatedSpriteComponent* comp = reinterpret_cast<AnimatedSpriteComponent*>(comp_);
		archive << comp->color;
		archive << comp->sprite.getState();
		archive << comp->material.getHashCode();
		archive << comp->renderLayer;
	}

	void scene_component_serialize_MeshComponent(BaseComponent* comp_, ArchiveO& archive)
	{
		MeshComponent* comp = reinterpret_cast<MeshComponent*>(comp_);

		comp->mesh.save(archive);
		comp->material.save(archive);
	}

	void scene_component_serialize_LightComponent(BaseComponent* comp_, ArchiveO& archive)
	{
		LightComponent* comp = reinterpret_cast<LightComponent*>(comp_);

		archive << comp->intensity << comp->color << comp->lightType;
		switch (comp->lightType)
		{
		case LightType_Point:
			archive << comp->point;
			break;
		}
	}

	void scene_component_serialize_SkyComponent(BaseComponent* comp_, ArchiveO& archive)
	{
		SkyComponent* comp = reinterpret_cast<SkyComponent*>(comp_);
		archive << comp->ambient;
	}

	void scene_component_serialize_CameraComponent(BaseComponent* comp_, ArchiveO& archive)
	{
		CameraComponent* comp = reinterpret_cast<CameraComponent*>(comp_);
		comp->camera.serialize(archive);
	}

	void scene_component_deserialize_SpriteComponent(BaseComponent* comp_, ArchiveI& archive)
	{
		new(comp_) SpriteComponent();

		SpriteComponent* comp = reinterpret_cast<SpriteComponent*>(comp_);
		archive >> comp->color;
		archive >> comp->sprite.texCoord;
		archive >> comp->renderLayer;

		comp->sprite.texture.load(archive);
		comp->material.load(archive);
	}

	void scene_component_deserialize_AnimatedSpriteComponent(BaseComponent* comp_, ArchiveI& archive)
	{
		new(comp_) AnimatedSpriteComponent();

		AnimatedSpriteComponent* comp = reinterpret_cast<AnimatedSpriteComponent*>(comp_);
		archive >> comp->color;

		AnimatedSprite::State sprState;
		archive >> sprState;
		if (result_fail(comp->sprite.setState(sprState))) {
			SV_LOG_ERROR("Sprite Animation not found");
		}

		size_t hash;
		archive >> hash;
		if (hash != 0u) {
			if (comp->material.loadFromFile(hash) != Result_Success) {
				SV_LOG_ERROR("Material not found, hashcode: %u", hash);
			}
		}

		archive >> comp->renderLayer;
	}

	void scene_component_deserialize_MeshComponent(BaseComponent* comp_, ArchiveI& archive)
	{
		MeshComponent* comp = new(comp_) MeshComponent();

		comp->mesh.load(archive);
		comp->material.load(archive);
	}

	void scene_component_deserialize_LightComponent(BaseComponent* comp_, ArchiveI& archive)
	{
		LightComponent* comp = new(comp_) LightComponent();

		archive >> comp->intensity >> comp->color >> comp->lightType;
		switch (comp->lightType)
		{
		case LightType_Point:
			archive >> comp->point;
			break;
		}
	}

	void scene_component_deserialize_SkyComponent(BaseComponent* comp_, ArchiveI& archive)
	{
		SkyComponent* comp = new(comp_) SkyComponent();
		archive >> comp->ambient;
	}

	void scene_component_deserialize_CameraComponent(BaseComponent* comp_, ArchiveI& archive)
	{
		new(comp_) CameraComponent();

		CameraComponent* comp = reinterpret_cast<CameraComponent*>(comp_);
		comp->camera.deserialize(archive);
	}

}