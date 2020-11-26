#include "core.h"

#include "scene_renderer_internal.h"

namespace sv {

	SceneRenderer::~SceneRenderer()
	{
		destroy();
	}

	void SceneRenderer::draw(ECS* ecs, Entity mainCamera, bool present)
	{
		EntityView<CameraComponent> cameras(ecs);

		for (CameraComponent& camera : cameras) {

			Transform trans = ecs_entity_transform_get(ecs, camera.entity);

			drawCamera2D(ecs, &camera.camera, trans.getWorldPosition(), trans.getWorldRotation());

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

	void SceneRenderer::drawCamera2D(ECS* ecs, Camera* pCamera, const vec3f& position, const vec4f& directionQuat)
	{
		if (!pCamera->isActive()) return;

		SceneRenderer_internal& rend = *reinterpret_cast<SceneRenderer_internal*>(pInternal);

		// Compute View Matrix
		XMMATRIX viewMatrix = math_matrix_view(position, directionQuat);

		const XMMATRIX& projectionMatrix = pCamera->getProjectionMatrix();
		XMMATRIX viewProjectionMatrix = viewMatrix * projectionMatrix;

		// Begin command list
		CommandList cmd = graphics_commandlist_begin();

		graphics_viewport_set(pCamera->getViewport(), 0u, cmd);
		graphics_scissor_set(pCamera->getScissor(), 0u, cmd);

		// Update camera buffer
		{
			rend.cameraBuffer.viewMatrix = viewMatrix;
			rend.cameraBuffer.projectionMatrix = projectionMatrix;
			rend.cameraBuffer.position = position;
			rend.cameraBuffer.direction = directionQuat;

			rend.cameraBuffer.updateGPU(cmd);
		}

		// Offscreen
		GPUImage* rt = pCamera->getOffscreenRT();
		GPUImage* ds = pCamera->getOffscreenDS();

		// Skybox
		//TODO: clear offscreen here :)

		// this is not good for performance - remember to do image barrier here
		graphics_image_clear(rt, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, { 0.f, 0.f, 0.f, 1.f }, 1.f, 0u, cmd);
		graphics_image_clear(ds, GPUImageLayout_DepthStencil, GPUImageLayout_DepthStencil, { 0.f, 0.f, 0.f, 1.f }, 1.f, 0u, cmd);

		// Sprite rendering
		{
			EntityView<SpriteComponent> sprites(ecs);
			EntityView<AnimatedSpriteComponent> animatedSprites(ecs);

			FrameList<SpriteIntermediate>& inter = rend.spritesIntermediates;
			inter.reset();

			// Add sprites to intermediate list
			for (SpriteComponent& sprite : sprites) {
				Transform trans = ecs_entity_transform_get(ecs, sprite.entity);

				// Compute layer value

				inter.emplace_back(trans.getWorldMatrix(), sprite.sprite.texCoord, sprite.sprite.texture.get(), sprite.color, sprite.material.get(), trans.getWorldPosition().z);
			}

			for (AnimatedSpriteComponent& sprite : animatedSprites) {
				Transform trans = ecs_entity_transform_get(ecs, sprite.entity);

				// Compute layer value

				Sprite spr = sprite.sprite.getSprite();
				inter.emplace_back(trans.getWorldMatrix(), spr.texCoord, spr.texture.get(), sprite.color, sprite.material.get(), trans.getWorldPosition().z);
			}

			// Sort sprites per material
			std::sort(inter.data(), inter.data() + inter.size(), [](const SpriteIntermediate& s0, const SpriteIntermediate& s1) {
				if (s0.depth == s1.depth) {
					if (s0.material != s1.material)
						return s0.material < s1.material;
					else return s0.instance.pTexture < s1.instance.pTexture;
				}
				else return s0.depth < s1.depth;
			});

			// Draw sprites
			{
				FrameList<SpriteInstance>& instances = rend.spritesInstances;
				instances.reset();

				SpriteRendererDrawDesc drawDesc;
				drawDesc.renderTarget = rt;
				drawDesc.cameraBuffer = &rend.cameraBuffer;

				auto drawCall = [&drawDesc, &instances, cmd](Material* material) 
				{
					if (instances.empty()) return;

					drawDesc.pInstances = instances.data();
					drawDesc.count = ui32(instances.size());
					drawDesc.material = material;

					sprite_renderer_draw(&drawDesc, cmd);
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
	}

	void SceneRenderer::create(ECS* ecs)
	{
		SceneRenderer_internal* rendering = new SceneRenderer_internal();
		pInternal = rendering;
		
		ecs_register<SpriteComponent>(ecs, scene_component_serialize_SpriteComponent, scene_component_deserialize_SpriteComponent);
		ecs_register<AnimatedSpriteComponent>(ecs, scene_component_serialize_AnimatedSpriteComponent, scene_component_deserialize_AnimatedSpriteComponent);
		ecs_register<CameraComponent>(ecs, scene_component_serialize_CameraComponent, scene_component_deserialize_CameraComponent);
	}
	void SceneRenderer::destroy()
	{
		SceneRenderer_internal* rendering = reinterpret_cast<SceneRenderer_internal*>(pInternal);
		if (rendering == nullptr) return;

		rendering->cameraBuffer.clear();

		delete rendering;
		pInternal = nullptr;
	}

	// COMPONENTS

	void scene_component_serialize_SpriteComponent(BaseComponent* comp_, ArchiveO& archive)
	{
		SpriteComponent* comp = reinterpret_cast<SpriteComponent*>(comp_);
		archive << comp->color;
		archive << comp->sprite.texCoord;
		archive << comp->sprite.texture.getHashCode();
		archive << comp->material.getHashCode();
	}

	void scene_component_serialize_AnimatedSpriteComponent(BaseComponent* comp_, ArchiveO& archive)
	{
		AnimatedSpriteComponent* comp = reinterpret_cast<AnimatedSpriteComponent*>(comp_);
		archive << comp->color;
		archive << comp->sprite.getState();
		archive << comp->material.getHashCode();
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
			if (comp->sprite.texture.load(hash) != Result_Success) {
				SV_LOG_ERROR("Texture not found, hashcode: %u", hash);
			}
		}

		archive >> hash;
		if (hash != 0u) {
			if (comp->material.load(hash) != Result_Success) {
				SV_LOG_ERROR("Material not found, hashcode: %u", hash);
			}
		}
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
			if (comp->material.load(hash) != Result_Success) {
				SV_LOG_ERROR("Material not found, hashcode: %u", hash);
			}
		}
	}

	void scene_component_deserialize_CameraComponent(BaseComponent* comp_, ArchiveI& archive)
	{
		new(comp_) CameraComponent();

		CameraComponent* comp = reinterpret_cast<CameraComponent*>(comp_);
		comp->camera.deserialize(archive);
	}

}