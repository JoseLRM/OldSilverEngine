#include "utils/math.h"

namespace sv {

    // TEMP
    XMMATRIX math_matrix_view(const v3_f32& position, const v4_f32& directionQuat)
    {
	XMVECTOR direction, pos, target;
		
	direction = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	pos = position.getDX();

	direction = XMVector3Transform(direction, XMMatrixRotationQuaternion(directionQuat.get_dx()));

	target = XMVectorAdd(pos, direction);
	return XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
    }
    
}
