#include "core.h"

#include "scene_internal.h"

#include "rendering/sprite_renderer.h"
#include "utils/allocator.h"

namespace sv {

	struct SpriteIntermediate {

		SpriteInstance	instance;
		Material* material;

		SpriteIntermediate() = default;
		SpriteIntermediate(const XMMATRIX& m, const vec4f& texCoord, GPUImage* pTex, Color color, Material* material)
			: instance(m, texCoord, pTex, color), material(material) {}

	};

	struct SceneRendering {

		CameraBuffer cameraBuffer;

		// Allocators used to draw the scene without allocate memory every frame
		FrameList<SpriteIntermediate>	spritesIntermediates;
		FrameList<SpriteInstance>		spritesInstances;

	};


	void Scene::draw(bool present)
	{
		EntityView<CameraComponent> cameras(m_ECS);

		for (CameraComponent& camera : cameras) {

			Transform trans = ecs_entity_transform_get(m_ECS, camera.entity);

			// TODO: World rotation;
			drawCamera(&camera.camera, trans.GetWorldPosition(), trans.GetLocalRotation());

			if (present && camera.entity == m_MainCamera) {
				GPUImage* image = camera.camera.getOffscreenRT();
				if (image == nullptr) continue;
				GPUImageRegion region;
				region.offset = { 0u, 0u, 0u };
				region.size = { graphics_image_get_width(image), graphics_image_get_height(image), 1u };

				graphics_present(image, region, GPUImageLayout_RenderTarget, graphics_commandlist_get());
			}
		}
	}

	void Scene::drawCamera(Camera* pCamera, const vec3f& position, const vec3f& rotation)
	{
		if (!pCamera->isActive()) return;

		SceneRendering& rend = *reinterpret_cast<SceneRendering*>(m_pRendering);

		// Compute View Matrix
		XMMATRIX viewMatrix = math_matrix_view(position, rotation);

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
			rend.cameraBuffer.setDirection(rotation);

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
			EntityView<SpriteComponent> sprites(m_ECS);

			FrameList<SpriteIntermediate>& inter = rend.spritesIntermediates;
			inter.reset();

			// Add sprites to intermediate list
			for (SpriteComponent& sprite : sprites) {
				Transform trans = ecs_entity_transform_get(m_ECS, sprite.entity);
				inter.emplace_back(trans.GetWorldMatrix(), sprite.sprite.texCoord, sprite.sprite.texture.getImage(), sprite.color, sprite.material.getMaterial());
			}

			// Sort sprites per material
			std::sort(inter.data(), inter.data() + inter.size(), [](const SpriteIntermediate& s0, const SpriteIntermediate& s1) {
				if (s0.material != s1.material)
					return s0.material < s1.material;
				else return s0.instance.pTexture < s1.instance.pTexture;
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

	void Scene::createRendering()
	{
		SceneRendering* rendering = new SceneRendering();
		m_pRendering = rendering;
		
		rendering->cameraBuffer.create();
	}
	void Scene::destroyRendering()
	{
		SceneRendering* rendering = reinterpret_cast<SceneRendering*>(m_pRendering);
		if (rendering == nullptr) return;

		rendering->cameraBuffer.destroy();

		delete rendering;
		m_pRendering = nullptr;
	}

}