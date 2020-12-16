#pragma once

#include "rendering/postprocessing.h"

namespace sv {

	struct PostProcessing_internal {

		static Result initialize();
		static Result close();

	};


	Result pp_blurInitialize();
	Result pp_blurClose();

}