#include "core.h"

#include "scene_internal.h"

#define Parse(x) reinterpret_cast<EntityTransform*>(x)

namespace sv {

	Transform::Transform(Entity entity, void* transform)
		: entity(entity), trans(transform) {}

	const vec3& Transform::GetLocalPosition() const noexcept 
	{ 
		return *(vec3*)& Parse(trans)->localPosition;
	}

	const vec3& Transform::GetLocalRotation() const noexcept 
	{ 
		return *(vec3*)& Parse(trans)->localRotation;
	}

	const vec3& Transform::GetLocalScale() const noexcept 
	{ 
		return *(vec3*)& Parse(trans)->localScale;
	}

	XMVECTOR Transform::GetLocalPositionDXV() const noexcept 
	{ 
		return XMLoadFloat3(&Parse(trans)->localPosition);
	}

	XMVECTOR Transform::GetLocalRotationDXV() const noexcept 
	{ 
		return XMLoadFloat3(&Parse(trans)->localRotation);
	}

	XMVECTOR Transform::GetLocalScaleDXV() const noexcept 
	{ 
		return XMLoadFloat3(&Parse(trans)->localScale);
	}

	XMMATRIX Transform::GetLocalMatrix() const noexcept
	{
		EntityTransform* t = Parse(trans);
		return XMMatrixScalingFromVector(GetLocalScaleDXV()) * XMMatrixRotationRollPitchYawFromVector(XMVectorSet(t->localRotation.x, t->localRotation.y, t->localRotation.z, 1.f))
			* XMMatrixTranslation(t->localPosition.x, t->localPosition.y, t->localPosition.z);
	}

	vec3 Transform::GetWorldPosition() noexcept
	{
		if (Parse(trans)->modified) UpdateWorldMatrix();
		return *(vec3*)& Parse(trans)->worldMatrix._41;
	}
	vec3 Transform::GetWorldRotation() noexcept
	{
		if (Parse(trans)->modified) UpdateWorldMatrix();
		return *(vec3*)& GetWorldRotationDXV();
	}
	vec3 Transform::GetWorldScale() noexcept
	{
		EntityTransform* t = Parse(trans);
		if (t->modified) UpdateWorldMatrix();
		return vec3(((vec3*)& t->worldMatrix._11)->Mag(), ((vec3*)& t->worldMatrix._21)->Mag(), ((vec3*)& t->worldMatrix._31)->Mag());
	}
	XMVECTOR Transform::GetWorldPositionDXV() noexcept
	{
		if (Parse(trans)->modified) UpdateWorldMatrix();

		vec3 position = GetWorldPosition();
		return XMVectorSet(position.x, position.y, position.z, 0.f);
	}
	XMVECTOR Transform::GetWorldRotationDXV() noexcept
	{
		if (Parse(trans)->modified) UpdateWorldMatrix();
		XMVECTOR scale;
		XMVECTOR quatRotation;
		XMVECTOR rotation;
		float angle;
		XMVECTOR position;

		XMMatrixDecompose(&scale, &quatRotation, &position, XMLoadFloat4x4(&Parse(trans)->worldMatrix));
		XMQuaternionToAxisAngle(&rotation, &angle, quatRotation);

		return rotation;
	}
	XMVECTOR Transform::GetWorldScaleDXV() noexcept
	{
		if (Parse(trans)->modified) UpdateWorldMatrix();
		XMVECTOR scale;
		XMVECTOR rotation;
		XMVECTOR position;

		XMMatrixDecompose(&scale, &rotation, &position, XMLoadFloat4x4(&Parse(trans)->worldMatrix));

		return XMVectorAbs(scale);
	}

	XMMATRIX Transform::GetWorldMatrix() noexcept
	{
		if (Parse(trans)->modified) UpdateWorldMatrix();
		return XMLoadFloat4x4(&Parse(trans)->worldMatrix);
	}

	void Transform::SetPosition(const vec3& position) noexcept
	{
		Parse(trans)->modified = true;
		Parse(trans)->localPosition = *(XMFLOAT3*)& position;
	}
	void Transform::SetRotation(const vec3& rotation) noexcept
	{
		Parse(trans)->modified = true;
		Parse(trans)->localRotation = *(XMFLOAT3*)& rotation;
	}
	void Transform::SetScale(const vec3& scale) noexcept
	{
		Parse(trans)->modified = true;
		Parse(trans)->localScale = *(XMFLOAT3*)& scale;
	}

	void Transform::SetWorldTransformPRS(const vec3& position, const vec3& rotation, const vec3& scale) noexcept
	{
	}
	void Transform::SetWorldTransformPR(const vec3& position, const vec3& rotation) noexcept
	{
		Entity parent = scene_ecs_entity_get_parent(entity);
		Parse(trans)->modified = true;

		if (parent == SV_INVALID_ENTITY) {
			Parse(trans)->localPosition = *reinterpret_cast<const XMFLOAT3*>(&position);
			Parse(trans)->localRotation = *reinterpret_cast<const XMFLOAT3*>(&rotation);
		}
	}

	void Transform::UpdateWorldMatrix()
	{
		EntityTransform* t = Parse(trans);
		t->modified = false;
		t->wakePhysics = true;

		XMMATRIX m = GetLocalMatrix();

		auto& list = scene_ecs_get_entity_data();
		EntityData& entityData = list[entity];
		Entity parent = entityData.parent;

		if (parent != SV_INVALID_ENTITY) {
			Transform parentTransform(parent, &list[parent].transform);
			XMMATRIX mp = parentTransform.GetWorldMatrix();
			m = m * mp;
		}
		XMStoreFloat4x4(&t->worldMatrix, m);

		// Set all the sons modified
		if (entityData.childsCount == 0) return;

		auto& entities = scene_ecs_get_entities();
		for (ui32 i = 0; i < entityData.childsCount; ++i) {
			EntityData& ed = list[entities[entityData.handleIndex + 1 + i]];
			ed.transform.modified = true;
		}

	}

}