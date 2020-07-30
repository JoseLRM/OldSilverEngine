#include "core.h"

#include "Scene.h"

using namespace _sv;

namespace sv {

	Transform::Transform(Entity entity, EntityTransform* transform, Scene* pScene)
		: entity(entity), trans(transform), scene(pScene) {}
	void Transform::operator=(const Transform& other)
	{
		*trans = *other.trans;
	}

	XMMATRIX Transform::GetLocalMatrix() const noexcept
	{
		return XMMatrixScalingFromVector(GetLocalScaleDXV()) * XMMatrixRotationRollPitchYawFromVector(XMVectorSet(trans->localRotation.x, trans->localRotation.y, trans->localRotation.z, 1.f))
			* XMMatrixTranslation(trans->localPosition.x, -trans->localPosition.y, trans->localPosition.z);
	}

	vec3 Transform::GetWorldPosition() noexcept
	{
		if (trans->modified) UpdateWorldMatrix();
		vec3 pos = *(vec3*)& trans->worldMatrix._41;
		pos.y = -pos.y;
		return pos;
	}
	vec3 Transform::GetWorldRotation() noexcept
	{
		if (trans->modified) UpdateWorldMatrix();
		return *(vec3*)& GetWorldRotationDXV();
	}
	vec3 Transform::GetWorldScale() noexcept
	{
		if (trans->modified) UpdateWorldMatrix();
		return vec3(((vec3*)& trans->worldMatrix._11)->Mag(), ((vec3*)& trans->worldMatrix._21)->Mag(), ((vec3*)& trans->worldMatrix._31)->Mag());
	}
	XMVECTOR Transform::GetWorldPositionDXV() noexcept
	{
		if (trans->modified) UpdateWorldMatrix();

		vec3 position = GetWorldPosition();
		return XMVectorSet(position.x, position.y, position.z, 0.f);
	}
	XMVECTOR Transform::GetWorldRotationDXV() noexcept
	{
		if (trans->modified) UpdateWorldMatrix();
		XMVECTOR scale;
		XMVECTOR quatRotation;
		XMVECTOR rotation;
		float angle;
		XMVECTOR position;

		XMMatrixDecompose(&scale, &quatRotation, &position, XMLoadFloat4x4(&trans->worldMatrix));
		XMQuaternionToAxisAngle(&rotation, &angle, quatRotation);

		return rotation;
	}
	XMVECTOR Transform::GetWorldScaleDXV() noexcept
	{
		if (trans->modified) UpdateWorldMatrix();
		XMVECTOR scale;
		XMVECTOR rotation;
		XMVECTOR position;

		XMMatrixDecompose(&scale, &rotation, &position, XMLoadFloat4x4(&trans->worldMatrix));

		return XMVectorAbs(scale);
	}

	XMMATRIX Transform::GetWorldMatrix() noexcept
	{
		if (trans->modified) UpdateWorldMatrix();
		return XMLoadFloat4x4(&trans->worldMatrix);
	}

	void Transform::SetPosition(const vec3& position) noexcept
	{
		trans->modified = true;
		trans->localPosition = *(XMFLOAT3*)& position;
	}
	void Transform::SetRotation(const vec3& rotation) noexcept
	{
		trans->modified = true;
		trans->localRotation = *(XMFLOAT3*)& rotation;
	}
	void Transform::SetScale(const vec3& scale) noexcept
	{
		trans->modified = true;
		trans->localScale = *(XMFLOAT3*)& scale;
	}

	void Transform::UpdateWorldMatrix()
	{
		trans->modified = false;

		XMMATRIX m = GetLocalMatrix();

		auto& list = scene->GetEntityDataList();
		EntityData& entityData = list[entity];
		Entity parent = entityData.parent;

		if (parent != SV_INVALID_ENTITY) {
			Transform parentTransform(parent, &list[parent].transform, scene);
			XMMATRIX mp = parentTransform.GetWorldMatrix();
			m = m * mp;
		}
		XMStoreFloat4x4(&trans->worldMatrix, m);

		// Set all the sons modified
		if (entityData.childsCount == 0) return;

		auto& entities = scene->GetEntityList();
		for (ui32 i = 0; i < entityData.childsCount; ++i) {
			EntityData& ed = list[entities[entityData.handleIndex + 1 + i]];
			ed.transform.modified = true;
		}

	}

}