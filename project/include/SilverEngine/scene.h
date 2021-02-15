#pragma once

#include "SilverEngine/entity_system.h"
#include "SilverEngine/graphics.h"
#include "SilverEngine/mesh.h"

#define SV_DEFINE_COMPONENT(name) struct name : public sv::Component<name> 

namespace sv {

    SV_DEFINE_COMPONENT(SpriteComponent) {

	TextureAsset texture;
	v4_f32 texcoords = { 0.f, 0.f, 1.f, 1.f };
	u32 layer = 0u;
	
    };

    enum ProjectionType : u32 {
	ProjectionType_Clip,
	ProjectionType_Orthographic,
	ProjectionType_Perspective,
    };

    struct CameraBuffer {

	XMMATRIX	view_matrix = XMMatrixIdentity();
	XMMATRIX	projection_matrix = XMMatrixIdentity();
	v3_f32		position;
	v4_f32		rotation;
	
	GPUBuffer* buffer = nullptr;
	
    };
    
    SV_DEFINE_COMPONENT(CameraComponent) {
	
	f32 near = -1000.f;
	f32 far = 1000.f;
	
	ProjectionType	projection_type = ProjectionType_Orthographic;

	CameraBuffer _buffer;
	
    };

    SV_DEFINE_COMPONENT(MeshComponent) {
	// TODO
    };

    SV_DEFINE_COMPONENT(PointLightComponent) {
	Color3f color = Color3f::White();
	f32 intensity = 1.f;
	f32 range = 5.f;
	f32 smoothness = 0.5f;
    };

    SV_DEFINE_COMPONENT(DirectionLightComponent) {
	Color3f color = Color3f::White();
	f32 intensity = 1.f;
	v3_f32 direction = { 0.f, 0.f, 1.f };
    };

    SV_DEFINE_COMPONENT(SpotLightComponent) {
	Color3f color = Color3f::White();
	f32 intensity = 1.f;
	v3_f32 direction = { 0.f, 0.f, 1.f };
	f32 range = 5.f;
	f32 smoothness = 0.5f;
    };
    
}
