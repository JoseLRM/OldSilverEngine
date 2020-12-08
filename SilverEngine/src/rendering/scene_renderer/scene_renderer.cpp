#include "core.h"

#include "scene_renderer_internal.h"

namespace sv {

	static SceneRendererTemp g_TempData[GraphicsLimit_CommandList];

	RenderLayer2D SceneRenderer::renderLayers2D[SceneRenderer::RENDER_LAYER_COUNT];
	RenderLayer3D SceneRenderer::renderLayers3D[SceneRenderer::RENDER_LAYER_COUNT];

	static ui32 g_RenderLayerOrder2D[SceneRenderer::RENDER_LAYER_COUNT];

	Result SceneRenderer_internal::initialize()
	{
		// Initialize Render Layers
		{
			for (ui32 i = 0u; i < SceneRenderer::RENDER_LAYER_COUNT; ++i) {

				RenderLayer2D& rl = SceneRenderer::renderLayers2D[i];

				rl.frustumTest = true;
				rl.sortValue = i;

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

			for (ui32 i = 0u; i < SceneRenderer::RENDER_LAYER_COUNT; ++i) {

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
			for (ui32 i = 0u; i < SceneRenderer::RENDER_LAYER_COUNT; ++i) {
				g_RenderLayerOrder2D[i] = i;
			}
		}

		return Result_Success;
	}

	Result SceneRenderer_internal::close()
	{
		for (ui32 i = 0u; i < GraphicsLimit_CommandList; ++i) {
			SceneRendererTemp& temp = g_TempData[i];

			svCheck(temp.cameraBuffer.clear());
			temp.spritesInstances.clear();

			for (ui32 i = 0u; i < SceneRenderer::RENDER_LAYER_COUNT; ++i)
				temp.spritesIntermediates[i].clear();
		}

		// Free heap memory from renderlayers
		{
			for (ui32 i = 0u; i < SceneRenderer::RENDER_LAYER_COUNT; ++i) {

				RenderLayer2D& rl = SceneRenderer::renderLayers2D[i];
				rl.name.clear();
			}

			for (ui32 i = 0u; i < SceneRenderer::RENDER_LAYER_COUNT; ++i) {

				RenderLayer3D& rl = SceneRenderer::renderLayers3D[i];
				rl.name.clear();
			}
		}

		return Result_Success;
	}

	void SceneRenderer::draw(ECS* ecs, Entity mainCamera, bool present)
	{
		// Draw Cameras
		EntityView<CameraComponent> cameras(ecs);

		for (CameraComponent& camera : cameras) {

			Transform trans = ecs_entity_transform_get(ecs, camera.entity);

			drawCamera(ecs, &camera.camera, trans.getWorldPosition(), trans.getWorldRotation());

			if (present && camera.entity == mainCamera) {
				GPUImage* image = camera.camera.getOffscreenRT();
				if (image == nullptr) continue;
				GPUImageRegion region;
				region.offset = { 0u, 0u, 0u };
				region.size = { graphics_image_get_width(image), graphics_image_get_height(image), 1u };

				graphics_present(image, region, GPUImageLayout_RenderTarget, graphics_commandlist_get());
			}
		}
	}

	inline bool frustumTest(const FrustumOthographic& frustum, Transform trans)
	{
		Circle2D circle;
		circle.position = trans.getWorldPosition().getVec2();
		circle.radius = trans.getWorldScale().getVec2().length() / 2.f;

		return frustum.intersects_circle(circle);
	}

	void SceneRenderer::drawCamera(ECS* ecs, Camera* pCamera, const vec3f& position, const vec4f& directionQuat)
	{
		//TODO: Move this to other place 
		// Sort render layers.
		{
			std::sort(g_RenderLayerOrder2D, g_RenderLayerOrder2D + RENDER_LAYER_COUNT, [](ui32 i0, ui32 i1)
			{
				const RenderLayer2D& rl0 = renderLayers2D[i0];
				const RenderLayer2D& rl1 = renderLayers2D[i1];

				return rl0.sortValue < rl1.sortValue;
			});
		}

		if (!pCamera->isActive()) return;

		// Compute View Matrix
		XMMATRIX viewMatrix = math_matrix_view(position, directionQuat);

		const XMMATRIX& projectionMatrix = pCamera->getProjectionMatrix();

		// Begin command list
		CommandList cmd = graphics_commandlist_begin();

		graphics_event_begin("Scene Rendering", cmd);

		SceneRendererTemp& ctx = g_TempData[cmd];

		graphics_viewport_set(pCamera->getViewport(), 0u, cmd);
		graphics_scissor_set(pCamera->getScissor(), 0u, cmd);

		// Update camera buffer
		{
			ctx.cameraBuffer.viewMatrix = viewMatrix;
			ctx.cameraBuffer.projectionMatrix = projectionMatrix;
			ctx.cameraBuffer.position = position;
			ctx.cameraBuffer.direction = directionQuat;

			ctx.cameraBuffer.updateGPU(cmd);
		}

		// Offscreen
		GPUImage* rt = pCamera->getOffscreenRT();
		GPUImage* ds = pCamera->getOffscreenDS();

		// this is not good for performance - remember to do image barrier here
		graphics_image_clear(rt, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, { 0.f, 0.f, 0.f, 1.f }, 1.f, 0u, cmd);
		graphics_image_clear(ds, GPUImageLayout_DepthStencil, GPUImageLayout_DepthStencil, { 0.f, 0.f, 0.f, 1.f }, 1.f, 0u, cmd);

		FrustumOthographic frustum;
		frustum.init_center(position.getVec2(), { pCamera->getWidth(), pCamera->getHeight() });

		// Sprite rendering
		{
			EntityView<SpriteComponent> sprites(ecs);
			EntityView<AnimatedSpriteComponent> animatedSprites(ecs);

			// Reset intermediate data
			for (ui32 i = 0u; i < RENDER_LAYER_COUNT; ++i) {
				ctx.spritesIntermediates[i].reset();
			}

			// Add sprites to intermediate list
			for (SpriteComponent& sprite : sprites) {
				Transform trans = ecs_entity_transform_get(ecs, sprite.entity);

				// Compute layer value
				FrameList<SpriteIntermediate>& inter = ctx.spritesIntermediates[sprite.renderLayer];
				const RenderLayer2D& rl = renderLayers2D[sprite.renderLayer];

				bool draw = false;

				// Frustrum culling
				if (rl.frustumTest) {
					draw = frustumTest(frustum, trans);  
				}
				else draw = true;

				if (draw) inter.emplace_back(trans.getWorldMatrix(), sprite.sprite.texCoord, sprite.sprite.texture.get(), sprite.color, sprite.material.get(), trans.getWorldPosition().z);
			}

			for (AnimatedSpriteComponent& sprite : animatedSprites) {
				Transform trans = ecs_entity_transform_get(ecs, sprite.entity);

				// Compute layer value
				FrameList<SpriteIntermediate>& inter = ctx.spritesIntermediates[sprite.renderLayer];
				const RenderLayer2D& rl = renderLayers2D[sprite.renderLayer];

				bool draw = false;

				// Frustrum culling
				if (rl.frustumTest) {
					draw = frustumTest(frustum, trans);
				}
				else draw = true;

				if (draw) {
					Sprite spr = sprite.sprite.getSprite();
					inter.emplace_back(trans.getWorldMatrix(), spr.texCoord, spr.texture.get(), sprite.color, sprite.material.get(), trans.getWorldPosition().z);
				}
			}

			// Sort sprites per material
			for (ui32 i = 0u; i < RENDER_LAYER_COUNT; ++i) {

				FrameList<SpriteIntermediate>& inter = ctx.spritesIntermediates[i];

				std::sort(inter.data(), inter.data() + inter.size(), [](const SpriteIntermediate& s0, const SpriteIntermediate& s1) {
					if (s0.depth == s1.depth) {
						if (s0.material != s1.material)
							return s0.material < s1.material;
						else return s0.instance.pTexture < s1.instance.pTexture;
					}
					else return s0.depth < s1.depth;
				});
			}

			// Draw sprites
			for (ui32 i = 0u; i < RENDER_LAYER_COUNT; ++i) {

				ui32 renderLayerIndex = g_RenderLayerOrder2D[i];
				const RenderLayer2D& rl = renderLayers2D[renderLayerIndex];
				FrameList<SpriteIntermediate>& inter = ctx.spritesIntermediates[renderLayerIndex];

				FrameList<SpriteInstance>& instances = ctx.spritesInstances;
				instances.reset();

				SpriteRenderer::prepare(rt, ctx.cameraBuffer, cmd);
				SpriteRenderer::disableDepthTest(cmd);

				auto drawCall = [&instances, cmd](Material* material) 
				{
					if (instances.empty()) return;

					SpriteRenderer::draw(material, instances.data(), ui32(instances.size()), cmd);
				};

				Material* mat = nullptr;

				for (auto it = inter.cbegin(); it != inter.cend(); ++it) {

					if (it->material != mat) {
						drawCall(mat);
						instances.reset();
						mat = it->material;
					}

					instances.emplace_back(it->instance);
				}
				drawCall(mat);
			}
		}

		// Draw Meshes
		{
			FrameList<MeshInstance>& meshes = ctx.meshInstances;
			FrameList<LightInstance>& lights = ctx.lightInstances;
			GBuffer& gBuffer = ctx.gBuffer;
			meshes.reset();
			lights.reset();

			if (gBuffer.diffuse == nullptr) 
				gBuffer.create(1920u, 1080u);

			EntityView<MeshComponent> meshesComp(ecs);
			for (MeshComponent& mesh : meshesComp) {
				
				Mesh* m = mesh.mesh.get();
				if (m) {

					Transform trans = ecs_entity_transform_get(ecs, mesh.entity);
					meshes.emplace_back(trans.getWorldMatrix(), m, mesh.material.get(), 10.f);
				}
			}

			EntityView<LightComponent> lightsComp(ecs);
			for (LightComponent& light : lightsComp) {

				Transform trans = ecs_entity_transform_get(ecs, light.entity);

				lights.emplace_back(trans.getWorldPosition(), 1.f, 1.f, 1.f);
			}

			MeshRenderer::drawMeshes(rt, gBuffer, ctx.cameraBuffer, meshes, lights, true, cmd);
		}

		graphics_event_end(cmd);
	}

	void SceneRenderer::initECS(ECS* ecs)
	{
		ecs_register<SpriteComponent>(ecs, scene_component_serialize_SpriteComponent, scene_component_deserialize_SpriteComponent);
		ecs_register<AnimatedSpriteComponent>(ecs, scene_component_serialize_AnimatedSpriteComponent, scene_component_deserialize_AnimatedSpriteComponent);
		ecs_register<MeshComponent>(ecs, scene_component_serialize_MeshComponent, scene_component_deserialize_MeshComponent);
		ecs_register<LightComponent>(ecs, scene_component_serialize_LightComponent, scene_component_deserialize_LightComponent);
		ecs_register<CameraComponent>(ecs, scene_component_serialize_CameraComponent, scene_component_deserialize_CameraComponent);
	}

	// COMPONENTS

	void scene_component_serialize_SpriteComponent(BaseComponent* comp_, ArchiveO& archive)
	{
		SpriteComponent* comp = reinterpret_cast<SpriteComponent*>(comp_);
		archive << comp->color;
		archive << comp->sprite.texCoord;
		archive << comp->sprite.texture.getHashCode();
		archive << comp->material.getHashCode();
		archive << comp->renderLayer;
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

	void scene_component_serialize_LightComponent(BaseComponent* comp, ArchiveO& archive)
	{
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
		size_t hash;
		archive >> hash;

		if (hash != 0u) {
			if (comp->sprite.texture.loadFromFile(hash) != Result_Success) {
				SV_LOG_ERROR("Texture not found, hashcode: %u", hash);
			}
		}

		archive >> hash;
		if (hash != 0u) {
			if (comp->material.loadFromFile(hash) != Result_Success) {
				SV_LOG_ERROR("Material not found, hashcode: %u", hash);
			}
		}

		archive >> comp->renderLayer;
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

	void scene_component_deserialize_LightComponent(BaseComponent* comp, ArchiveI& archive)
	{
	}

	void scene_component_deserialize_CameraComponent(BaseComponent* comp_, ArchiveI& archive)
	{
		new(comp_) CameraComponent();

		CameraComponent* comp = reinterpret_cast<CameraComponent*>(comp_);
		comp->camera.deserialize(archive);
	}

}