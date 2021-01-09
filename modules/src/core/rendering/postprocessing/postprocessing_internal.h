#pragma once

#include "core/rendering/postprocessing.h"
#include "core/rendering/render_utils.h"

namespace sv {

	struct PostProcessing_internal {

		static Result initialize();
		static Result close();

	};

	Result pp_bloomInitialize();
	Result pp_bloomClose();

	Result pp_toneMappingInitialize();
	Result pp_toneMappingClose();

	extern Shader* g_VS_DefPostProcessing;
	extern Shader* g_PS_DefPostProcessing;

}