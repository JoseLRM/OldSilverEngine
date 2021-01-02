#include "core.h"

#include "rendering/scene_renderer.h"

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
		m_Offscreen = other.m_Offscreen;
		m_Projection = other.m_Projection;

		other.m_Active = false;
		other.m_Offscreen = nullptr;
	}

	void Camera::clear()
	{
		sv::Result res = graphics_destroy(m_Offscreen);
		SV_ASSERT(result_okay(res));
		m_Offscreen = nullptr;
		SV_ASSERT(result_okay(res));
	}

	Result Camera::serialize(ArchiveO& archive)
	{
		archive << m_Active << m_Projection << m_Type << getResolution() << m_PP;
		return Result_Success;
	}

	Result Camera::deserialize(ArchiveI& archive)
	{
		vec2u res;
		archive >> m_Active >> m_Projection >> m_Type >> res >> m_PP;
		if (res.x != 0u && res.y != 0u) svCheck(setResolution(res.x, res.y));

		return Result_Success;
	}

	bool Camera::isActive() const noexcept
	{
		return m_Active && m_Offscreen != nullptr;
	}

	Viewport Camera::getViewport() const noexcept
	{
		vec2u res = getResolution();
		return { 0.f, 0.f, float(res.x), float(res.y), 0.f, 1.f };
	}

	Scissor Camera::getScissor() const noexcept
	{
		vec2u res = getResolution();
		return { 0u, 0u, res.x, res.y };
	}

	void Camera::adjust(u32 width, u32 height) noexcept
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

	void Camera::setCameraType(CameraType type) noexcept
	{
		if (m_Type != type) {
			m_Type = type;

			switch (m_Type)
			{
			case sv::CameraType_2D:
				m_Projection.width = 10.f;
				m_Projection.height = 10.f;
				m_Projection.near = -1000.f;
				m_Projection.far = 1000.f;
				break;

			case sv::CameraType_3D:
				m_Projection.width = 0.1f;
				m_Projection.height = 0.1f;
				m_Projection.near = 0.1f;
				m_Projection.far = 1000.f;
				break;
			}
			
			m_Projection.matrix = XMMatrixIdentity();
			m_Projection.modified = true;
		}
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

	const XMMATRIX& Camera::getProjectionMatrix() noexcept
	{
		if (m_Projection.modified) {
			m_Projection.modified = false;

			switch (m_Type)
			{
			case CameraType_2D:
				m_Projection.matrix = XMMatrixOrthographicLH(m_Projection.width, m_Projection.height, m_Projection.near, m_Projection.far);
				break;

			case CameraType_3D:
				m_Projection.matrix = XMMatrixPerspectiveLH(m_Projection.width, m_Projection.height, m_Projection.near, m_Projection.far);
				break;

			default:
				m_Projection.matrix = XMMatrixIdentity();
				break;
			}
		}

		return m_Projection.matrix;
	}

	Result Camera::setResolution(u32 width, u32 height)
	{
		if (width == 0u || height == 0u || width > 10000u || height > 10000u) return Result_InvalidUsage;
		vec2u oldRes = getResolution();

		if (oldRes.x == width && oldRes.y == height) return Result_Success;

		// TODO: Resize function
		{
			if (m_Offscreen) {
				svCheck(graphics_destroy(m_Offscreen));
				m_CameraBuffer.clear();
				m_Offscreen = nullptr;
			}

			GPUImageDesc desc;
			desc.pData = nullptr;
			desc.format = OFFSCREEN_FORMAT;
			desc.layout = GPUImageLayout_RenderTarget;
			desc.type = GPUImageType_RenderTarget | GPUImageType_ShaderResource;
			desc.usage = ResourceUsage_Static;
			desc.CPUAccess = CPUAccess_None;
			desc.dimension = 2u;
			desc.width = width;
			desc.height = height;
			desc.depth = 1u;
			desc.layers = 1u;

			svCheck(graphics_image_create(&desc, &m_Offscreen));
		}

		return Result_Success;
	}

	u32 Camera::getResolutionWidth() const noexcept
	{
		if (m_Offscreen) {
			return graphics_image_get_width(m_Offscreen);
		}
		return 0u;
	}

	u32 Camera::getResolutionHeight() const noexcept
	{
		if (m_Offscreen) {
			return graphics_image_get_height(m_Offscreen);
		}
		return 0u;
	}

	vec2u Camera::getResolution() const noexcept
	{
		if (m_Offscreen) {
			return { graphics_image_get_width(m_Offscreen), graphics_image_get_height(m_Offscreen) };
		}
		return { 0u, 0u };
	}

	Result Camera::updateCameraBuffer(const vec3f& position, const vec4f& rotation, const XMMATRIX& viewMatrix, CommandList cmd)
	{
		m_CameraBuffer.viewMatrix = viewMatrix;
		m_CameraBuffer.projectionMatrix = getProjectionMatrix();
		m_CameraBuffer.position = position;
		m_CameraBuffer.direction = rotation;
		
		m_CameraBuffer.updateGPU(cmd);
		return Result();
	}

}