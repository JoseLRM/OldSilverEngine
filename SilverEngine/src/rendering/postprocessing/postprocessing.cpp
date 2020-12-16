#include "core.h"

#include "postprocessing_internal.h"

namespace sv {

	Result PostProcessing_internal::initialize()
	{
		svCheck(pp_blurInitialize());
		return Result_Success;
	}

	Result PostProcessing_internal::close()
	{
		svCheck(pp_blurClose());
		return Result_Success;
	}

}