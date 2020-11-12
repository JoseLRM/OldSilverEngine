#include "core.h"

#include "scene_internal.h"

#include "utils/allocator.h"

namespace sv {

	Result scene_asset_create(const char* filePath, void* pObj)
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
			desc.createFn = scene_asset_create;
			desc.destroyFn = scene_asset_destroy;
			desc.recreateFn = nullptr;
			desc.isUnusedFn = nullptr;
			desc.assetSize = sizeof(Scene);
			desc.unusedLifeTime = 5.f;

			svCheck(asset_register_type(&desc, nullptr));
		}
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

	void Scene::create(SceneType sceneType)
	{
		m_SceneType = sceneType;

		// Initialize Entity Component System
		ecs_create(&m_ECS);

		ecs_register<NameComponent>(m_ECS, "Name",
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
		m_Renderer.create(m_ECS);

		// Physics
		m_Physics.create(m_ECS);

		// Create main camera
		m_MainCamera = ecs_entity_create(m_ECS);
		ecs_component_add<NameComponent>(m_ECS, m_MainCamera, "Camera");
		Camera& camera = ecs_component_add<CameraComponent>(m_ECS, m_MainCamera)->camera;

		switch (sceneType)
		{
		case sv::SceneType_2D:
		{
			camera.setProjectionType(ProjectionType_Orthographic);
			camera.setProjectionLength(10.f);
			camera.setResolution(1920u, 1080u);
			camera.setNear(-1000.f);
			camera.setFar(1000.f);
			camera.activate();
		}
			break;
		case sv::SceneType_3D:
		{
			camera.setProjectionType(ProjectionType_Perspective);
			camera.setProjectionLength(0.1f);
			camera.setResolution(1920u, 1080u);
			camera.setNear(0.2f);
			camera.setFar(100000.f);
			camera.activate();
		}
			break;
		}
		
	}

	void Scene::destroy()
	{
		if (m_SceneType == SceneType_Invalid) return;
		m_SceneType = SceneType_Invalid;

		ecs_destroy(m_ECS);
		m_ECS = nullptr;
		m_Renderer.destroy();
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

		archive << m_SceneType;

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

		// Scene Type
		archive >> m_SceneType;

		if (m_SceneType != SceneType_2D && m_SceneType != SceneType_3D) return Result_InvalidFormat;

		// Create the scene
		create(m_SceneType);

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

	void Scene::draw(bool present)
	{
		m_Renderer.draw(m_ECS, m_MainCamera, present);
	}

	void Scene::drawCamera(Camera* pCamera, const vec3f& position, const vec4f& directionQuat)
	{
		m_Renderer.drawCamera2D(m_ECS, pCamera, position, directionQuat);
	}

	Result SceneAsset::createFile(const char* filePath, SceneType sceneType)
	{
		Scene scene;
		scene.create(sceneType);
		svCheck(scene.serialize(filePath));
		return asset_refresh();
	}

}