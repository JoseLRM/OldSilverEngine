#pragma once

#include "core.h"
#include "simulation/asset_system.h"
#include "simulation/entity_system.h"
#include "utils/io.h"
#include "utils/allocator.h"
#include "rendering/material_system.h"
#include "simulation/sprite_animator.h"
#include "simulation/model.h"
#include "rendering/render_utils.h"

namespace sv {

	enum CameraType : u32 {
		CameraType_2D,
		CameraType_3D
	};

	struct CameraBloomData {
		f32 threshold = 0.9f;
		u32 blurIterations = 1u;
		f32 blurRange = 10.f;
		bool enabled = false;
	};

	struct CameraToneMappingData {
		f32 exposure = 0.8f;
		bool enabled = false;
	};

	// This class only contains information about projection, postprocessing and offscreen image
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

		void adjust(u32 width, u32 height) noexcept;
		void adjust(float aspect) noexcept;

		float	getProjectionLength() const noexcept;
		void	setProjectionLength(float length) noexcept;

		const XMMATRIX& getProjectionMatrix() noexcept;

		// Resolution

		Result	setResolution(u32 width, u32 height);
		u32		getResolutionWidth() const noexcept;
		u32		getResolutionHeight() const noexcept;
		vec2u	getResolution() const noexcept;

		// CameraBuffer

		SV_INLINE CameraBuffer& getCameraBuffer() noexcept { return m_CameraBuffer; }
		Result updateCameraBuffer(const vec3f& position, const vec4f& rotation, const XMMATRIX& viewMatrix, CommandList cmd);

		// Offscreen getters

		inline GPUImage* getOffscreen() const noexcept { return m_Offscreen; }

		// Get PP Data

		inline CameraBloomData& getBloom() noexcept { return m_PP.bloom; };
		inline const CameraBloomData& getBloom() const noexcept { return m_PP.bloom; };

		inline CameraToneMappingData& getToneMapping() noexcept { return m_PP.toneMapping; };
		inline const CameraToneMappingData& getToneMapping() const noexcept { return m_PP.toneMapping; };

	private:
		GPUImage* m_Offscreen = nullptr;
		CameraBuffer m_CameraBuffer;

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

		struct {

			CameraBloomData bloom;
			CameraToneMappingData toneMapping;

		} m_PP;
	};

	struct RenderLayer2D {

		std::string		name;
		i32				sortValue;
		float			lightMult;
		float			ambientMult;
		bool			frustumTest;
		bool			enabled;

	};

	struct RenderLayer3D {

		std::string name;

	};

	struct LightSceneData {

		FrameList<LightInstance>	lights;
		Color3f						ambientLight;

	};

	struct SceneRenderer {

		SceneRenderer() = delete;

		static void initECS(ECS* ecs);
		
		/*
			It only sort the renderLayerOrder2D array but is necesary to sort 2D properly
		*/
		static void prepareRenderLayers2D();

		/*
			Clear the camera offscreen
		*/
		static void clearScreen(Camera& camera, Color4f color, CommandList cmd);
		static void clearGBuffer(GBuffer& gBuffer, Color4f color, f32 depth, u32 stencil, CommandList cmd);

		/*
			Prepare the lights instances and do shadow mapping
		*/
		static void processLighting(ECS* ecs, LightSceneData& lightData);

		// 2D Layer Rendering

		static void drawSprites2D(ECS* ecs, Camera& camera, GBuffer& gBuffer, LightSceneData& lightData, const vec3f& position, const vec4f& direction, u32 renderLayerIndex, CommandList cmd);

		// 3D Rendering
		static void drawSprites3D(ECS* ecs, Camera& camera, GBuffer& gBuffer, LightSceneData& lightData, const vec3f& position, const vec4f& direction, CommandList cmd);
		static void drawMeshes3D(ECS* ecs, Camera& camera, GBuffer& gBuffer, LightSceneData& lightData, const vec3f& position, const vec4f& direction, CommandList cmd);

		// PostProcessing

		static void doPostProcessing(Camera& camera, GBuffer& gBuffer, CommandList cmd);

		// Presents the image to the swapchain
		static void present(GPUImage* image);

		// RenderLayers 2D
		static RenderLayer2D	renderLayers2D[RENDERLAYER_COUNT];
		static u32				renderLayerOrder2D[RENDERLAYER_COUNT];

		// RenderLayers 3D
		static RenderLayer3D	renderLayers3D[RENDERLAYER_COUNT];

	};

	// Sprite Component

	struct SpriteComponent : public Component<SpriteComponent> {

		MaterialAsset material;
		Sprite sprite;
		Color color = Color::White();
		u32 renderLayer = 0u;

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
		u32 renderLayer = 0u;

		AnimatedSpriteComponent() {}
		AnimatedSpriteComponent(Color col) : color(col) {}
		AnimatedSpriteComponent(AnimatedSprite& spr) : sprite(spr) {}
		AnimatedSpriteComponent(AnimatedSprite& spr, Color col) : sprite(spr), color(col) {}

	};

	// Mesh Component

	struct MeshComponent : public Component<MeshComponent> {

		MaterialAsset	material;
		MeshAsset		mesh;
		u32			renderLayer = 0u;

		MeshComponent() = default;
		MeshComponent(MeshAssetType assetType) { mesh.loadFromID(assetType); }

	};

	// Light Component

	struct LightComponent : public Component<LightComponent> {
		
		LightType lightType;
		Color3f color;
		float intensity;
		
		union {
			struct {
				float range;
				float smoothness;
			} point;
		};

		LightComponent() : lightType(LightType_Point), color(Color3f::White()), intensity(0.7f), point({ 1.f, 0.6f }) {}

	};

	// Sky Component

	struct SkyComponent : public Component<SkyComponent> {
		Color3f ambient = Color3f::White();
	};

	// Camera Component

	struct CameraComponent : public Component<CameraComponent> {
		Camera camera;

		CameraComponent() = default;
		CameraComponent(u32 width, u32 height) { camera.setResolution(width, height); }

	};

}