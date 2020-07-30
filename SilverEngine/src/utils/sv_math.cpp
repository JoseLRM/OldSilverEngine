#include "core.h"

namespace sv {

	void math_quaternion_to_euler(sv::vec4* q, sv::vec3* e)
	{
		// roll (x-axis rotation)
		float sinr_cosp = 2.f * (q->w * q->x + q->y * q->z);
		float cosr_cosp = 1.f - 2.f * (q->x * q->x + q->y * q->y);
		e->x = std::atan2(sinr_cosp, cosr_cosp);

		// pitch (y-axis rotation)
		float sinp = 2.f * (q->w * q->y - q->z * q->x);
		if (std::abs(sinp) >= 1.f)
			e->y = std::copysign(PI / 2.f, sinp); // use 90 degrees if out of range
		else
			e->y = std::asin(sinp);

		// yaw (z-axis rotation)
		float siny_cosp = 2.f * (q->w * q->z + q->x * q->y);
		float cosy_cosp = 1.f - 2.f * (q->y * q->y + q->z * q->z);
		e->z = std::atan2(siny_cosp, cosy_cosp);
	}

	XMMATRIX math_matrix_view(vec3 position, vec3 rotation)
	{
		XMVECTOR direction = XMVectorSet(0.f, 0.f, 1.f, 0.f);

		direction = XMVector3Transform(direction, XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, 0.f));

		const auto target = XMVECTOR(position) + direction;
		return XMMatrixLookAtLH(position, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
	}

}