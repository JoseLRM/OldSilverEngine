#include "core.h"

#include "scene_internal.h"

namespace sv {

	void scene_renderer_draw(Scene* scene_, bool present)
	{
		parseScene();

		EntityView<CameraComponent> cameras(scene.ecs);

		for (CameraComponent& camera : cameras) {

			Transform trans = ecs_entity_transform_get(scene.ecs, camera.entity);

			CameraDrawDesc desc;
			desc.pSettings = &camera.settings;
			desc.position = trans.GetWorldPosition();
			desc.rotation = trans.GetLocalRotation(); // TODO: World rotation;
			desc.pOffscreen = &camera.offscreen;

			scene_renderer_camera_draw(scene_, &desc);

			if (present && camera.entity == scene.mainCamera) {
				GPUImage& image = camera.offscreen.renderTarget;
				GPUImageRegion region;
				region.offset = { 0u, 0u, 0u };
				region.size = { graphics_image_get_width(image), graphics_image_get_height(image), 1u };

				renderer_present(image, region, GPUImageLayout_RenderTarget, graphics_commandlist_get());
			}
		}
	}

	void scene_renderer_camera_draw(Scene* scene_, const CameraDrawDesc* desc)
	{
		parseScene();

		if (!desc->pSettings->active || desc->pOffscreen == nullptr) return;

		// Compute View Matrix
		XMMATRIX viewMatrix = math_matrix_view(desc->position, desc->rotation);

		XMMATRIX projectionMatrix = renderer_projection_matrix(desc->pSettings->projection);
		XMMATRIX viewProjectionMatrix = viewMatrix * projectionMatrix;

		// Render
		Offscreen& offscreen = *desc->pOffscreen;

		// Begin command list
		CommandList cmd = graphics_commandlist_begin();

		Viewport viewport = offscreen.GetViewport();
		graphics_viewport_set(&viewport, 1u, cmd);

		Scissor scissor = offscreen.GetScissor();
		graphics_scissor_set(&scissor, 1u, cmd);

		// Skybox
		//TODO: clear offscreen here :)

		// this is not good for performance - remember to do image barrier here
		graphics_image_clear(offscreen.renderTarget, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, { 0.f, 0.f, 0.f, 1.f }, 1.f, 0u, cmd);
		graphics_image_clear(offscreen.depthStencil, GPUImageLayout_DepthStencil, GPUImageLayout_DepthStencil, { 0.f, 0.f, 0.f, 1.f }, 1.f, 0u, cmd);

		// Sprite rendering
		{
			// TODO: Linear allocator
			EntityView<SpriteComponent> sprites(scene.ecs);
			std::vector<SpriteInstance> instances;

			for (SpriteComponent& sprite : sprites) {

				Transform trans = ecs_entity_transform_get(scene.ecs, sprite.entity);
				instances.emplace_back(trans.GetWorldMatrix(), sprite.sprite.texCoord, &sprite.sprite.texture.Get()->texture, sprite.color);

			}

			SpriteRenderingDesc desc;
			desc.pInstances = instances.data();
			desc.count = ui32(instances.size());
			desc.pViewProjectionMatrix = &viewProjectionMatrix;
			desc.pRenderTarget = &offscreen.renderTarget;

			renderer_sprite_rendering(&desc, cmd);
		}
		// Mesh rendering
		{
			std::vector<MeshInstance> instances; // TODO: Linear allocator
			std::vector<LightInstance> lightsInstances;
			std::vector<ui32> indices;

			// Get Meshes
			{
				EntityView<MeshComponent> meshes(scene.ecs);

				for (MeshComponent& mesh : meshes) {

					if (mesh.mesh.Get() == nullptr || mesh.material.Get() == nullptr) continue;

					Transform trans = ecs_entity_transform_get(scene.ecs, mesh.entity);
					instances.emplace_back(trans.GetWorldMatrix() * viewMatrix, mesh.mesh.Get(), mesh.material.Get());

				}
			}
			// Get Lights
			{
				EntityView<LightComponent> lights(scene.ecs);

				for (LightComponent& light : lights) {

					Transform trans = ecs_entity_transform_get(scene.ecs, light.entity);

					vec3f pos = trans.GetWorldPosition();
					vec3f dir; // TODO: Light Direction

					XMVECTOR posDX = pos.get_dx();
					pos = vec3f(XMVector3Transform(posDX, viewMatrix));

					lightsInstances.emplace_back(light.lightType, pos, dir, light.intensity, light.range, light.smoothness, light.color);

				}
			}

			// TODO: Frustum culling
			for (ui32 i = 0; i < instances.size(); ++i) indices.push_back(i);

			ForwardRenderingDesc desc;
			desc.pInstances = instances.data();
			desc.pIndices = indices.data();
			desc.count = ui32(indices.size());
			desc.transparent = false;
			desc.pViewMatrix = &viewMatrix;
			desc.pProjectionMatrix = &projectionMatrix;
			desc.pRenderTarget = &offscreen.renderTarget;
			desc.pDepthStencil = &offscreen.depthStencil;
			desc.lights = lightsInstances.data();
			desc.lightCount = ui32(lightsInstances.size());

			renderer_mesh_forward_rendering(&desc, cmd);
		}
	}

}