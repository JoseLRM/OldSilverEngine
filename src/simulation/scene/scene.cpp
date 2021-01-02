#include "core.h"

#include "scene_internal.h"

#include "utils/allocator.h"
#include "engine.h"

namespace sv {

	Result scene_asset_load_file(const char* filePath, void* pObj)
	{
		Scene* scene = new(pObj) Scene();
		return scene->deserialize(filePath);
	}

	Result scene_asset_destroy(void* pObj)
	{
		Scene* scene = reinterpret_cast<Scene*>(pObj);
		scene->destroy();
		scene->~Scene();
		return Result_Success;
	}

	Result scene_initialize()
	{
		// Register Scene Asset
		{
			const char* extensions[] = {
				"scene"
			};

			AssetRegisterTypeDesc desc;
			desc.name = "Scene";
			desc.pExtensions = extensions;
			desc.extensionsCount = 1u;
			desc.loadFileFn = scene_asset_load_file;
			desc.loadIDFn = nullptr;
			desc.createFn = nullptr;
			desc.destroyFn = scene_asset_destroy;
			desc.reloadFileFn = nullptr;
			desc.serializeFn = nullptr;
			desc.isUnusedFn = nullptr;
			desc.assetSize = sizeof(Scene);
			desc.unusedLifeTime = 5.f;

			svCheck(asset_register_type(&desc, nullptr));
		}

		// Register Component Names
		{
			ecs_component_register<NameComponent>("Name");
			ecs_component_register<CameraComponent>("Camera");
			ecs_component_register<SpriteComponent>("Sprite");
			ecs_component_register<AnimatedSpriteComponent>("Animated Sprite");
			ecs_component_register<MeshComponent>("Mesh");
			ecs_component_register<LightComponent>("Light");
			ecs_component_register<SkyComponent>("Sky");
			ecs_component_register<RigidBody2DComponent>("RigidBody 2D");
			ecs_component_register<BoxCollider2DComponent>("BoxCollider 2D");
			ecs_component_register<CircleCollider2DComponent>("CircleCollider 2D");
		}

		return Result_Success;
	}

	Result scene_close()
	{
		return Result_Success;
	}

	// SCENE

	Scene::Scene()
	{}

	Scene::~Scene()
	{
		destroy();
	}

	void Scene::create()
	{
		// Initialize Entity Component System
		ecs_create(&m_ECS);

		ecs_register<NameComponent>(m_ECS,
			[](BaseComponent* comp, ArchiveO& archive)
		{
			NameComponent* name = reinterpret_cast<NameComponent*>(comp);
			archive << name->name;
		},
			[](BaseComponent* comp, ArchiveI& archive)
		{
			new(comp) NameComponent();
			NameComponent* name = reinterpret_cast<NameComponent*>(comp);
			archive >> name->name;
		});

		// Rendering
		SceneRenderer::initECS(m_ECS);

		// Physics
		m_Physics.create(m_ECS);

		// Create main camera
		m_MainCamera = ecs_entity_create(m_ECS);
		ecs_component_add<NameComponent>(m_ECS, m_MainCamera, "Camera");
		Camera& camera = ecs_component_add<CameraComponent>(m_ECS, m_MainCamera)->camera;

		camera.setCameraType(CameraType_2D);
		camera.setResolution(1920u, 1080u);
		camera.activate();
		
	}

	void Scene::destroy()
	{
		ecs_destroy(m_ECS);
		m_ECS = nullptr;
		m_Physics.destroy();
		m_MainCamera = SV_ENTITY_NULL;
	}

	void Scene::clear()
	{
		ecs_clear(m_ECS);
		m_MainCamera = SV_ENTITY_NULL;
	}

	Result Scene::serialize(const char* filePath)
	{
		ArchiveO archive;

		svCheck(serialize(archive));

		// Save
		svCheck(archive.save_file(filePath, false));

		return Result_Success;
	}

	Result Scene::serialize(ArchiveO& archive)
	{
		archive << engine_version_get();

		svCheck(ecs_serialize(m_ECS, archive));
		archive << m_MainCamera;

		return Result_Success;
	}

	Result Scene::deserialize(const char* filePath)
	{
		ArchiveI archive;

		svCheck(archive.open_file(filePath));
		svCheck(deserialize(archive));

		return Result_Success;
	}

	Result Scene::deserialize(ArchiveI& archive)
	{
		destroy();

		// Version
		{
			Version version;
			archive >> version;

			if (version < SCENE_MINIMUM_SUPPORTED_VERSION) return Result_UnsupportedVersion;
		}

		// Create the scene
		create();

		svCheck(ecs_deserialize(m_ECS, archive));

		archive >> m_MainCamera;

		if (!ecs_entity_exist(m_ECS, m_MainCamera) || ecs_component_get<CameraComponent>(m_ECS, m_MainCamera) == nullptr) {
			m_MainCamera = SV_ENTITY_NULL;
			return Result_InvalidFormat;
		}

		return Result_Success;
	}

	void Scene::setMainCamera(Entity camera)
	{
		if (!ecs_entity_exist(m_ECS, camera) || ecs_component_get<CameraComponent>(m_ECS, camera) == nullptr) m_MainCamera = SV_ENTITY_NULL;
		else m_MainCamera = camera;
	}

	void Scene::physicsSimulate(float dt)
	{
		m_Physics.simulate(dt, m_ECS);
	}

	Entity Scene::getMainCamera()
	{
		return m_MainCamera;
	}

	void Scene::draw()
	{
		// This should call only in 2D mode
		SceneRenderer::prepareRenderLayers2D();
		
		SceneRenderer::processLighting(m_ECS, lightData);

		EntityView<CameraComponent> cameras(m_ECS);

		for (CameraComponent& cam : cameras) {

			if (cam.camera.isActive()) {
				
				Transform trans = ecs_entity_transform_get(m_ECS, cam.entity);
				vec3f position = trans.getWorldPosition();
				vec4f rotation = trans.getWorldRotation();

				// Begin CommandList
				CommandList cmd = graphics_commandlist_begin();

				graphics_event_begin("Scene Rendering", cmd);

				// Update CameraBuffer
				cam.camera.updateCameraBuffer(position, rotation, math_matrix_view(position, rotation), cmd);

				// Clear Buffers
				SceneRenderer::clearScreen(cam.camera, { 0.f, 0.f, 0.f, 1.f }, cmd);
				
				if (gBuffer.diffuse == nullptr)
					// TODO: Should manage this better xd
					gBuffer.create(1920u, 1080u);
				else {
					SceneRenderer::clearGBuffer(gBuffer, { 0.f, 0.f, 0.f, 1.f }, 1.f, 0u, cmd);
				}

				// Draw Render Components
				graphics_viewport_set(cam.camera.getOffscreen(), 0u, cmd);
				graphics_scissor_set(cam.camera.getOffscreen(), 0u, cmd);

				switch (cam.camera.getCameraType())
				{
				case CameraType_2D:
					for (u32 i = 0u; i < RENDERLAYER_COUNT; ++i) {
						SceneRenderer::drawSprites2D(m_ECS, cam.camera, gBuffer, lightData, position, rotation, i, cmd);
					}
					break;

				case CameraType_3D:
					SceneRenderer::drawMeshes3D(m_ECS, cam.camera, gBuffer, lightData, position, rotation, cmd);
					SceneRenderer::drawSprites3D(m_ECS, cam.camera, gBuffer, lightData, position, rotation, cmd);
					break;
				}

				graphics_event_end(cmd);
			}
		}
	}

	Result SceneAsset::createFile(const char* filePath)
	{
		Scene scene;
		scene.create();
		svCheck(scene.serialize(filePath));
		return asset_refresh();
	}

}