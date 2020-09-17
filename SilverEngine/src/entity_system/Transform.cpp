#include "core.h"

#include "entity_system_internal.h"

#define parse() sv::EntityTransform* t = reinterpret_cast<sv::EntityTransform*>(trans)

namespace sv {

	Transform::Transform(Entity entity, void* transform, ECS* ecs)
		: entity(entity), trans(transform), pECS(ecs) {}

	const vec3& Transform::GetLocalPosition() const noexcept 
	{ 
		parse();
		return *(vec3*)& t->localPosition;
	}

	const vec3& Transform::GetLocalRotation() const noexcept 
	{ 
		parse();
		return *(vec3*)& t->localRotation;
	}

	const vec3& Transform::GetLocalScale() const noexcept 
	{ 
		parse();
		return *(vec3*)& t->localScale;
	}

	XMVECTOR Transform::GetLocalPositionDXV() const noexcept 
	{ 
		parse();
		return XMLoadFloat3(&t->localPosition);
	}

	XMVECTOR Transform::GetLocalRotationDXV() const noexcept 
	{ 
		parse();
		return XMLoadFloat3(&t->localRotation);
	}

	XMVECTOR Transform::GetLocalScaleDXV() const noexcept 
	{ 
		parse();
		return XMLoadFloat3(&t->localScale);
	}

	XMMATRIX Transform::GetLocalMatrix() const noexcept
	{
		parse();
		return XMMatrixScalingFromVector(GetLocalScaleDXV()) * XMMatrixRotationRollPitchYawFromVector(XMVectorSet(t->localRotation.x, t->localRotation.y, t->localRotation.z, 1.f))
			* XMMatrixTranslation(t->localPosition.x, t->localPosition.y, t->localPosition.z);
	}

	vec3 Transform::GetWorldPosition() noexcept
	{
		parse();
		if (t->modified) UpdateWorldMatrix();
		return *(vec3*)& t->worldMatrix._41;
	}
	vec3 Transform::GetWorldRotation() noexcept
	{
		parse();
		if (t->modified) UpdateWorldMatrix();
		return *(vec3*)& GetWorldRotationDXV();
	}
	vec3 Transform::GetWorldScale() noexcept
	{
		parse();
		if (t->modified) UpdateWorldMatrix();
		return vec3(((vec3*)& t->worldMatrix._11)->Mag(), ((vec3*)& t->worldMatrix._21)->Mag(), ((vec3*)& t->worldMatrix._31)->Mag());
	}
	XMVECTOR Transform::GetWorldPositionDXV() noexcept
	{
		parse();
		if (t->modified) UpdateWorldMatrix();

		vec3 position = GetWorldPosition();
		return XMVectorSet(position.x, position.y, position.z, 0.f);
	}
	XMVECTOR Transform::GetWorldRotationDXV() noexcept
	{
		parse();
		if (t->modified) UpdateWorldMatrix();
		XMVECTOR scale;
		XMVECTOR quatRotation;
		XMVECTOR rotation;
		float angle;
		XMVECTOR position;

		XMMatrixDecompose(&scale, &quatRotation, &position, XMLoadFloat4x4(&t->worldMatrix));
		XMQuaternionToAxisAngle(&rotation, &angle, quatRotation);

		return rotation;
	}
	XMVECTOR Transform::GetWorldScaleDXV() noexcept
	{
		parse();

		if (t->modified) UpdateWorldMatrix();
		XMVECTOR scale;
		XMVECTOR rotation;
		XMVECTOR position;

		XMMatrixDecompose(&scale, &rotation, &position, XMLoadFloat4x4(&t->worldMatrix));

		return XMVectorAbs(scale);
	}

	XMMATRIX Transform::GetWorldMatrix() noexcept
	{
		parse();

		if (t->modified) UpdateWorldMatrix();
		return XMLoadFloat4x4(&t->worldMatrix);
	}

	void Transform::SetPosition(const vec3& position) noexcept
	{
		Notify();

		parse();
		t->localPosition = *(XMFLOAT3*)& position;
	}
	void Transform::SetRotation(const vec3& rotation) noexcept
	{
		Notify();

		parse();
		t->localRotation = *(XMFLOAT3*)& rotation;
	}
	void Transform::SetScale(const vec3& scale) noexcept
	{
		parse();

		Notify();
		t->localScale = *(XMFLOAT3*)& scale;
	}

	void Transform::UpdateWorldMatrix()
	{
		parse();
		ECS_internal& ecs = *reinterpret_cast<ECS_internal*>(pECS);

		t->modified = false;

		XMMATRIX m = GetLocalMatrix();

		auto& list = ecs.entityData;
		EntityData& entityData = list[entity];
		Entity parent = entityData.parent;

		if (parent != SV_ENTITY_NULL) {
			Transform parentTransform(parent, &list.get_transform(parent), pECS);
			XMMATRIX mp = parentTransform.GetWorldMatrix();
			m = m * mp;
		}
		XMStoreFloat4x4(&t->worldMatrix, m);
	}

	void Transform::Notify()
	{
		parse();

		if (!t->modified) {

			t->modified = true;

			ECS_internal& ecs = *reinterpret_cast<ECS_internal*>(pECS);
			auto& list = ecs.entityData;
			EntityData& entityData = list[entity];

			if (entityData.childsCount == 0) return;

			auto& entities = ecs.entities;
			for (ui32 i = 0; i < entityData.childsCount; ++i) {
				EntityTransform& et = list.get_transform(entities[entityData.handleIndex + 1 + i]);
				et.modified = true;
			}

		}
	}

}