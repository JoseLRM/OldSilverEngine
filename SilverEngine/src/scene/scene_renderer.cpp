#include "core.h"

#include "scene_internal.h"

namespace sv {

	void scene_renderer_draw(Scene& scene)
	{
		EntityView<CameraComponent> cameras(scene.ecs);

		for (CameraComponent& camera : cameras) {

			Transform trans = ecs_entity_transform_get(scene.ecs, camera.entity);

			CameraDrawDesc desc;
			desc.pSettings = &camera.settings;
			desc.position = trans.GetWorldPosition();
			desc.rotation = trans.GetLocalRotation(); // TODO: World rotation;

			if (camera.entity == scene.mainCamera) {

				camera.settings.active = true;

				desc.pOffscreen = &renderer_offscreen_get();
				scene_renderer_camera_draw(&desc, scene);

			}
			else {

				desc.pOffscreen = camera.GetOffscreen();
				scene_renderer_camera_draw(&desc, scene);

			}
		}
	}

	void scene_renderer_camera_draw(const CameraDrawDesc* desc, Scene& scene)
	{
		if (!desc->pSettings->active || desc->pOffscreen == nullptr) return;

		// Compute View Matrix
		XMVECTOR lookAt = XMVectorSet(0.f, 0.f, 1.f, 0.f);
		lookAt = XMVector3Transform(lookAt, XMMatrixRotationRollPitchYaw(desc->rotation.x, desc->rotation.y, desc->rotation.z));
		lookAt = XMVectorAdd(lookAt, desc->position);
		XMMATRIX viewMatrix = XMMatrixLookAtLH(desc->position, lookAt, XMVectorSet(0.f, 1.f, 0.f, 0.f));

		XMMATRIX projectionMatrix = renderer_projection_matrix(desc->pSettings->projection);
		XMMATRIX viewProjectionMatrix = viewMatrix * projectionMatrix;

		// Render
		Offscreen& offscreen = *desc->pOffscreen;

		// Begin command list
		CommandList cmd = graphics_commandlist_begin();

		Viewport viewport = offscreen.GetViewport();
		graphics_set_viewports(&viewport, 1u, cmd);

		Scissor scissor = offscreen.GetScissor();
		graphics_set_scissors(&scissor, 1u, cmd);

		// Skybox
		//TODO: clear offscreen here :)

		// this is not good for performance - remember to do image barrier here
		graphics_image_clear(offscreen.renderTarget, GPUImageLayout_ShaderResource, GPUImageLayout_RenderTarget, { 0.f, 0.f, 0.f, 1.f }, 1.f, 0u, cmd);
		graphics_image_clear(offscreen.depthStencil, GPUImageLayout_DepthStencil, GPUImageLayout_DepthStencil, { 0.f, 0.f, 0.f, 1.f }, 1.f, 0u, cmd);

		// Sprite rendering
		{
			// TODO: Linear allocator
			EntityView<SpriteComponent> sprites(scene.ecs);
			std::vector<SpriteInstance> instances;

			for (SpriteComponent& sprite : sprites) {

				Transform trans = ecs_entity_transform_get(scene.ecs, sprite.entity);
				instances.emplace_back(trans.GetWorldMatrix(), sprite.sprite, sprite.color);

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

					vec3 pos = trans.GetWorldPosition();
					vec3 dir; // TODO: Light Direction

					pos = XMVector3Transform(pos, viewMatrix);

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

		// Offscreen layout: from render target to shader resource
		GPUBarrier barrier = GPUBarrier::Image(offscreen.renderTarget, GPUImageLayout_RenderTarget, GPUImageLayout_ShaderResource);
		graphics_barrier(&barrier, 1u, cmd);
	}

}