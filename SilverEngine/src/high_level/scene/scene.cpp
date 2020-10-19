#include "core.h"

#include "scene_internal.h"
#include "main/engine.h"

#include "utils/allocator.h"

namespace sv {

	// CAMERA

	Camera::Camera()
	{
	}

	Camera::~Camera()
	{
		clear();
	}

	Camera::Camera(const Camera& other)
	{
		m_Active = other.m_Active;
		m_Projection = other.m_Projection;

		vec2u res = other.getResolution();
		if (res.x != 0u && res.y != 0u) 
		{
			Result r = setResolution(res.x, res.y);
			SV_ASSERT(result_okay(r));
		}
	}

	Camera::Camera(Camera&& other) noexcept
	{
		m_Active = other.m_Active;
		m_OffscreenRT = other.m_OffscreenRT;
		m_OffscreenDS = other.m_OffscreenDS;
		m_Projection = other.m_Projection;

		other.m_Active = false;
		other.m_OffscreenRT = nullptr;
		other.m_OffscreenDS = nullptr;
	}

	void Camera::clear()
	{
		sv::Result res = graphics_destroy(m_OffscreenRT);
		SV_ASSERT(result_okay(res));
		m_OffscreenRT = nullptr;
		res = graphics_destroy(m_OffscreenDS);
		SV_ASSERT(result_okay(res));
		m_OffscreenDS = nullptr;
	}

	Result Camera::serialize(ArchiveO& archive)
	{
		archive << m_Active << m_Projection << getResolution();
		return Result_Success;
	}

	Result Camera::deserialize(ArchiveI& archive)
	{
		vec2u res;
		archive >> m_Active >> m_Projection >> res;
		if (res.x != 0u && res.y != 0u) svCheck(setResolution(res.x, res.y));

		return Result_Success;
	}

	bool Camera::isActive() const noexcept
	{
		return m_Active && m_OffscreenRT != nullptr;
	}

	Viewport Camera::getViewport() const noexcept
	{
		vec2u res = getResolution();
		return { 0.f, 0.f, float(res.x), float(res.y), 0.f, 1.f };
	}

	Scissor Camera::getScissor() const noexcept
	{
		vec2u res = getResolution();
		return { 0.f, 0.f, float(res.x), float(res.y) };
	}

	void Camera::adjust(ui32 width, ui32 height) noexcept
	{
		adjust(float(width) / float(height));
	}

	void Camera::adjust(float aspect) noexcept
	{
		if (m_Projection.width / m_Projection.height == aspect) return;
		vec2f res = { m_Projection.width, m_Projection.height };
		float mag = res.length();
		res.x = aspect;
		res.y = 1.f;
		res.normalize();
		res *= mag;
		m_Projection.width = res.x;
		m_Projection.height = res.y;
		m_Projection.modified = true;
	}

	float Camera::getProjectionLength() const noexcept
	{
		return math_sqrt(m_Projection.width * m_Projection.width + m_Projection.height * m_Projection.height);
	}

	void Camera::setProjectionLength(float length) noexcept
	{
		float currentLength = getProjectionLength();
		if (currentLength == length) return;
		m_Projection.width = m_Projection.width / currentLength * length;
		m_Projection.height = m_Projection.height / currentLength * length;
		m_Projection.modified = true;
	}

	void Camera::setWidth(float width) noexcept
	{
		m_Projection.width = width;
		m_Projection.modified = true;
	}

	void Camera::setHeight(float height) noexcept
	{
		m_Projection.height = height;
		m_Projection.modified = true;
	}

	void Camera::setNear(float near) noexcept
	{
		m_Projection.near = near;
		m_Projection.modified = true;
	}

	void Camera::setFar(float far) noexcept
	{
		m_Projection.far = far;
		m_Projection.modified = true;
	}

	void Camera::setProjectionType(ProjectionType type) noexcept
	{
		if (m_Projection.type != type) {
			m_Projection.type = type;
			m_Projection.modified = true;
		}
	}

	const XMMATRIX& Camera::getProjectionMatrix() noexcept
	{
		if (m_Projection.modified) {
			m_Projection.modified = false;

			switch (m_Projection.type)
			{
			case ProjectionType_Orthographic:
				m_Projection.matrix = XMMatrixOrthographicLH(m_Projection.width, m_Projection.height, m_Projection.near, m_Projection.far);
				break;

			case ProjectionType_Perspective:
				m_Projection.matrix = XMMatrixPerspectiveLH(m_Projection.width, m_Projection.height, m_Projection.near, m_Projection.far);
				break;

			default:
				m_Projection.matrix = XMMatrixIdentity();
				break;
			}
		}

		return m_Projection.matrix;
	}

	Result Camera::setResolution(ui32 width, ui32 height)
	{
		if (width == 0u || height == 0u || width > 10000u || height > 10000u) return Result_InvalidUsage;
		vec2u oldRes = getResolution();

		if (oldRes.x == width && oldRes.y == height) return Result_Success;

		// TODO: Resize function
		{
			if (m_OffscreenRT) {
				svCheck(graphics_destroy(m_OffscreenRT));
				m_OffscreenRT = nullptr;
			}
			if (m_OffscreenDS) {
				svCheck(graphics_destroy(m_OffscreenDS));
				m_OffscreenDS = nullptr;
			}

			GPUImageDesc desc;
			desc.pData = nullptr;
			desc.format = Format_R8G8B8A8_SRGB;
			desc.layout = GPUImageLayout_RenderTarget;
			desc.type = GPUImageType_RenderTarget | GPUImageType_ShaderResource;
			desc.usage = ResourceUsage_Static;
			desc.CPUAccess = CPUAccess_None;
			desc.dimension = 2u;
			desc.width = width;
			desc.height = height;
			desc.depth = 1u;
			desc.layers = 1u;

			svCheck(graphics_image_create(&desc, &m_OffscreenRT));

			desc.format = Format_D24_UNORM_S8_UINT;
			desc.layout = GPUImageLayout_DepthStencil;
			desc.type = GPUImageType_DepthStencil;

			svCheck(graphics_image_create(&desc, &m_OffscreenDS));
		}

		return Result_Success;
	}

	ui32 Camera::getResolutionWidth() const noexcept
	{
		if (m_OffscreenRT) {
			return graphics_image_get_width(m_OffscreenRT);
		}
		return 0u;
	}

	ui32 Camera::getResolutionHeight() const noexcept
	{
		if (m_OffscreenRT) {
			return graphics_image_get_height(m_OffscreenRT);
		}
		return 0u;
	}

	vec2u Camera::getResolution() const noexcept
	{
		if (m_OffscreenRT) {
			return { graphics_image_get_width(m_OffscreenRT), graphics_image_get_height(m_OffscreenRT) };
		}
		return { 0u, 0u };
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

		ecs_register<NameComponent>(m_ECS, "NameComponent", scene_component_serialize_NameComponent, scene_component_deserialize_NameComponent);
		ecs_register<SpriteComponent>(m_ECS, "SpriteComponent", scene_component_serialize_SpriteComponent, scene_component_deserialize_SpriteComponent);
		ecs_register<CameraComponent>(m_ECS, "CameraComponent", scene_component_serialize_CameraComponent, scene_component_deserialize_CameraComponent);
		ecs_register<RigidBody2DComponent>(m_ECS, "RigidBody2DComponent", scene_component_serialize_RigidBody2DComponent, scene_component_deserialize_RigidBody2DComponent);

		// Create main camera
		m_MainCamera = ecs_entity_create(m_ECS);
		ecs_component_add<NameComponent>(m_ECS, m_MainCamera, "Camera");
		Camera& camera = ecs_component_add<CameraComponent>(m_ECS, m_MainCamera)->camera;
		camera.setProjectionType(ProjectionType_Orthographic);
		camera.setProjectionLength(10.f);
		camera.setResolution(1080u, 720u);
		camera.activate();

		// Rendering
		createRendering();

		// Physics
		createPhysics();
	}

	void Scene::destroy()
	{
		ecs_destroy(m_ECS);
		m_ECS = nullptr;
		m_MainCamera = SV_ENTITY_NULL;
		destroyRendering();
		destroyPhysics();
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
		// Version
		{
			Version version;
			archive >> version;

			if (version < SCENE_MINIMUM_SUPPORTED_VERSION) return Result_UnsupportedVersion;
		}

		svCheck(ecs_deserialize(m_ECS, archive));

		return Result_Success;
	}

	void Scene::setMainCamera(Entity camera)
	{
		SV_ASSERT(ecs_entity_exist(m_ECS, camera) && ecs_component_get<CameraComponent>(m_ECS, camera));
		m_MainCamera = camera;
	}

	Entity Scene::getMainCamera()
	{
		return m_MainCamera;
	}

}