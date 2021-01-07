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

		/*
			It only sort the renderLayerOrder2D array but is necesary to sort 2D properly
		*/
		static void prepareRenderLayers2D();

		/*
			Clear the camera offscreen
		*/
		static void clearGBuffer(GBuffer& gBuffer, Color4f color, f32 depth, u32 stencil, CommandList cmd);

		/*
			Prepare the lights instances and do shadow mapping
		*/
		static void processLighting(ECS* ecs, LightSceneData& lightData);

		// 2D Layer Rendering

		static void drawSprites2D(ECS* ecs, CameraData& cameraData, LightSceneData& lightData, u32 renderLayerIndex, CommandList cmd);
		static void drawParticles2D(ECS* ecs, CameraData& cameraData, LightSceneData& lightData, u32 renderLayerIndex, CommandList cmd);

		// 3D Rendering
		static void drawSprites3D(ECS* ecs, CameraData& cameraData, LightSceneData& lightData, CommandList cmd);
		static void drawMeshes3D(ECS* ecs, CameraData& cameraData, LightSceneData& lightData, CommandList cmd);

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

		void serialize(ArchiveO& file);
		void deserialize(ArchiveI& file);

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

		void serialize(ArchiveO& file);
		void deserialize(ArchiveI& file);

	};

	// Mesh Component

	struct MeshComponent : public Component<MeshComponent> {

		MaterialAsset	material;
		MeshAsset		mesh;
		u32			renderLayer = 0u;

		MeshComponent() = default;
		MeshComponent(MeshAssetType assetType) { mesh.loadFromID(assetType); }

		void serialize(ArchiveO& file);
		void deserialize(ArchiveI& file);

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

		void serialize(ArchiveO& file);
		void deserialize(ArchiveI& file);

	};

	// Sky Component

	struct SkyComponent : public Component<SkyComponent> {
		Color3f ambient = Color3f::White();

		void serialize(ArchiveO& file);
		void deserialize(ArchiveI& file);
	};

	// Camera Component

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

	struct CameraComponent : public Component<CameraComponent> {
		
		bool				enabled = true;
		GBuffer				gBuffer;
		CameraProjection	projection;

		struct {
			CameraBloomData	bloom;
			CameraToneMappingData toneMapping;
		};

		CameraComponent() {}
		CameraComponent(u32 width, u32 height) { gBuffer.create(width, height); }

		void serialize(ArchiveO& file);
		void deserialize(ArchiveI& file);
	};

}