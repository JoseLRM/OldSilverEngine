#include "core.h"

#include "Scene.h"

namespace SV {

	Transform::Transform(SV::Entity entity, SV::Scene* pScene) 
		: entity(entity), m_pScene(pScene) {}
	void Transform::operator=(const Transform& other)
	{
		m_LocalPosition = other.m_LocalPosition;
		m_LocalRotation = other.m_LocalRotation;
		m_LocalScale = other.m_LocalScale;
		m_Modified = true;
	}
	void Transform::operator=(Transform&& other)
	{
		m_LocalPosition = other.m_LocalPosition;
		m_LocalRotation = other.m_LocalRotation;
		m_LocalScale = other.m_LocalScale;
		entity = other.entity;
		m_pScene = other.m_pScene;
		m_Modified = true;
	}

	XMMATRIX Transform::GetLocalMatrix() const noexcept
	{
		return XMMatrixScalingFromVector(GetLocalScaleDXV()) * XMMatrixRotationRollPitchYawFromVector(XMVectorSet(m_LocalRotation.x, m_LocalRotation.y, m_LocalRotation.z, 1.f))
			* XMMatrixTranslationFromVector(GetLocalPositionDXV());
	}

	vec3 Transform::GetWorldPosition() noexcept
	{
		if (m_Modified) UpdateWorldMatrix();
		return *(vec3*)& m_WorldMatrix._41;
	}
	vec3 Transform::GetWorldRotation() noexcept
	{
		if (m_Modified) UpdateWorldMatrix();
		return *(vec3*)& GetWorldRotationDXV();
	}
	vec3 Transform::GetWorldScale() noexcept
	{
		if (m_Modified) UpdateWorldMatrix();
		return vec3(((vec3*)& m_WorldMatrix._11)->Mag(), ((vec3*)& m_WorldMatrix._21)->Mag(), ((vec3*)& m_WorldMatrix._31)->Mag());
	}
	XMVECTOR Transform::GetWorldPositionDXV() noexcept
	{
		if (m_Modified) UpdateWorldMatrix();

		vec3 position = GetWorldPosition();
		return XMVectorSet(position.x, position.y, position.z, 0.f);
	}
	XMVECTOR Transform::GetWorldRotationDXV() noexcept
	{
		if (m_Modified) UpdateWorldMatrix();
		XMVECTOR scale;
		XMVECTOR quatRotation;
		XMVECTOR rotation;
		float angle;
		XMVECTOR position;

		XMMatrixDecompose(&scale, &quatRotation, &position, XMLoadFloat4x4(&m_WorldMatrix));
		XMQuaternionToAxisAngle(&rotation, &angle, quatRotation);

		return rotation;
	}
	XMVECTOR Transform::GetWorldScaleDXV() noexcept
	{
		if (m_Modified) UpdateWorldMatrix();
		XMVECTOR scale;
		XMVECTOR rotation;
		XMVECTOR position;

		XMMatrixDecompose(&scale, &rotation, &position, XMLoadFloat4x4(&m_WorldMatrix));

		return XMVectorAbs(scale);
	}

	XMMATRIX Transform::GetWorldMatrix() noexcept
	{
		if (m_Modified) UpdateWorldMatrix();
		return XMLoadFloat4x4(&m_WorldMatrix);
	}

	void Transform::SetPosition(const vec3& position) noexcept
	{
		m_Modified = true;
		m_LocalPosition = *(XMFLOAT3*)& position;
	}
	void Transform::SetRotation(const vec3& rotation) noexcept
	{
		m_Modified = true;
		m_LocalRotation = *(XMFLOAT3*)& rotation;
	}
	void Transform::SetScale(const vec3& scale) noexcept
	{
		m_Modified = true;
		m_LocalScale = *(XMFLOAT3*)& scale;
	}

	void Transform::UpdateWorldMatrix()
	{
		m_Modified = false;

		XMMATRIX m = GetLocalMatrix();

		auto& list = m_pScene->GetEntityDataList();
		EntityData& entityData = list[entity];
		Entity parent = entityData.parent;

		if (parent != SV_INVALID_ENTITY) {
			XMMATRIX mp = list[parent].transform.GetWorldMatrix();
			m = m * mp;
		}
		XMStoreFloat4x4(&m_WorldMatrix, m);

		// Set all the sons modified
		if (entityData.childsCount == 0) return;

		auto& entities = m_pScene->GetEntityList();
		for (ui32 i = 0; i < entityData.childsCount; ++i) {
			EntityData& ed = list[entities[entityData.handleIndex + 1 + i]];
			ed.transform.m_Modified = true;
		}

	}

}