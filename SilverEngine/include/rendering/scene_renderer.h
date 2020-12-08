#pragma once

#include "core.h"
#include "simulation/asset_system.h"
#include "simulation/entity_system.h"
#include "utils/io.h"
#include "rendering/material_system.h"
#include "simulation/sprite_animator.h"
#include "simulation/model.h"

namespace sv {

	enum CameraType : ui32 {
		CameraType_2D,
		CameraType_3D
	};

	// This class only contains information about projection and some images used during rendering (offscreen)
	class Camera {
	public:
		Camera();
		~Camera();
		Camera(const Camera& other);
		Camera(Camera&& other) noexcept;

		void clear();

		Result serialize(ArchiveO& archive);
		Result deserialize(ArchiveI& archive);

		inline void activate() noexcept { m_Active = true; }
		inline void deactivate() noexcept { m_Active = false; }
		bool isActive() const noexcept; //TODO: while implementing check if the resolution is defined

		inline CameraType	getCameraType() const noexcept { return m_Type; }
		void				setCameraType(CameraType type) noexcept;


		// Projection

		inline float			getWidth() const noexcept { return m_Projection.width; }
		inline float			getHeight() const noexcept { return m_Projection.height; }
		inline float			getNear() const noexcept { return m_Projection.near; }
		inline float			getFar() const noexcept { return m_Projection.far; }

		void setWidth(float width) noexcept;
		void setHeight(float height) noexcept;
		void setNear(float near) noexcept;
		void setFar(float far) noexcept;

		Viewport	getViewport() const noexcept;
		Scissor		getScissor() const noexcept;

		void adjust(ui32 width, ui32 height) noexcept;
		void adjust(float aspect) noexcept;

		float	getProjectionLength() const noexcept;
		void	setProjectionLength(float length) noexcept;



		const XMMATRIX& getProjectionMatrix() noexcept;

		// Resolution

		Result	setResolution(ui32 width, ui32 height);
		ui32	getResolutionWidth() const noexcept;
		ui32	getResolutionHeight() const noexcept;
		vec2u	getResolution() const noexcept;

		// Offscreen getters

		inline GPUImage* getOffscreenRT() const noexcept { return m_OffscreenRT; }
		inline GPUImage* getOffscreenDS() const noexcept { return m_OffscreenDS; }

	private:
		GPUImage* m_OffscreenRT = nullptr;
		GPUImage* m_OffscreenDS = nullptr;

		struct {
			float width = 1.f;
			float height = 1.f;
			float near = -1000.f;
			float far = 1000.f;
			XMMATRIX matrix = XMMatrixIdentity();
			bool modified = true;
		} m_Projection;

		CameraType m_Type = CameraType_2D;

		bool m_Active = true;

	};

	struct RenderLayer2D {

		std::string name;
		bool frustumTest;
		i32 sortValue;

	};

	struct RenderLayer3D {

		std::string name;

	};

	struct SceneRenderer {

		SceneRenderer() = delete;

		static void initECS(ECS* ecs);
		
		// Draw calls

		static void draw(ECS* ecs, Entity mainCamera, bool present = true);
		static void drawCamera(ECS* ecs, Camera* pCamera, const vec3f& position, const vec4f& directionQuat);

		// RenderLayers 2D

		static constexpr ui32 RENDER_LAYER_COUNT = 16u;

		static RenderLayer2D renderLayers2D[RENDER_LAYER_COUNT];
		static RenderLayer3D renderLayers3D[RENDER_LAYER_COUNT];

	};

	// Sprite Component

	struct SpriteComponent : public Component<SpriteComponent> {

		MaterialAsset material;
		Sprite sprite;
		Color color = Color::White();
		ui32 renderLayer = 0u;

		SpriteComponent() {}
		SpriteComponent(Color col) : color(col) {}
		SpriteComponent(Sprite& spr) : sprite(spr) {}
		SpriteComponent(Sprite& spr, Color col) : sprite(spr), color(col) {}

	};

	// Animated Sprite Component

	struct AnimatedSpriteComponent : public Component<AnimatedSpriteComponent> {

		MaterialAsset material;
		AnimatedSprite sprite;
		Color color = Color::White();
		ui32 renderLayer = 0u;

		AnimatedSpriteComponent() {}
		AnimatedSpriteComponent(Color col) : color(col) {}
		AnimatedSpriteComponent(AnimatedSprite& spr) : sprite(spr) {}
		AnimatedSpriteComponent(AnimatedSprite& spr, Color col) : sprite(spr), color(col) {}

	};

	// Mesh Component

	struct MeshComponent : public Component<MeshComponent> {

		MaterialAsset	material;
		MeshAsset		mesh;
		ui32			renderLayer = 0u;

		MeshComponent() = default;
		MeshComponent(MeshAssetType assetType) { mesh.loadFromID(assetType); }

	};

	// Light Component

	struct LightComponent : public Component<LightComponent> {

	};

	// Camera Component

	struct CameraComponent : public Component<CameraComponent> {
		Camera camera;

		CameraComponent() = default;
		CameraComponent(ui32 width, ui32 height) { camera.setResolution(width, height); }

	};

}