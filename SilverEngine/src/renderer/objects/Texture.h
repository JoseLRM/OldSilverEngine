#pragma once

#include "graphics.h"

namespace sv {

	class Texture {
		GPUImage			m_Image;
		//TEMP
		Sampler				m_Sampler;

	public:
		Result CreateFromFile(const char* filePath);
		Result Destroy();

		inline GPUImage& GetImage() { return m_Image; }
		inline Sampler& GetSampler() { return m_Sampler; }

	};

}