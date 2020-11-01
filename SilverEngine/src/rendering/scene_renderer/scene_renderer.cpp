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

	void SceneRenderer::drawCamera(ECS* ecs, Camera* pCamera, const vec3f& position, const vec4f& directionQuat)
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
			rend.cameraBuffer.setViewMatrix(viewMatrix);
			rend.cameraBuffer.setProjectionMatrix(projectionMatrix);
			rend.cameraBuffer.setViewProjectionMatrix(viewProjectionMatrix);
			rend.cameraBuffer.setPosition(position);
			rend.cameraBuffer.setDirection(directionQuat);

			rend.cameraBuffer.update(cmd);
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

	void SceneRenderer::create()
	{
		SceneRenderer_internal* rendering = new SceneRenderer_internal();
		pInternal = rendering;
		
		rendering->cameraBuffer.create();
	}
	void SceneRenderer::destroy()
	{
		SceneRenderer_internal* rendering = reinterpret_cast<SceneRenderer_internal*>(pInternal);
		if (rendering == nullptr) return;

		rendering->cameraBuffer.destroy();

		delete rendering;
		pInternal = nullptr;
	}

}