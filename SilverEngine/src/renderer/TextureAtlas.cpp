#include "core.h"

#include "TextureAtlas.h"

using namespace _sv;

namespace sv {

	bool TextureAtlas::CreateFromFile(const char* filePath, bool linearFilter, SV_GFX_ADDRESS_MODE addressMode)
	{
		svCheck(Destroy());

		m_DrawData = std::make_unique<TextureAtlas_DrawData>();

		// Get file data
		void* data;
		ui32 width;
		ui32 height;
		svCheck(utils_loader_image(filePath, &data, &width, &height));

		// Create Image
		{
			SV_GFX_IMAGE_DESC desc;

			desc.pData = data;
			desc.size = width * height * 4u;
			desc.format = SV_GFX_FORMAT_R8G8B8A8_UNORM;
			desc.layout = SV_GFX_IMAGE_LAYOUT_SHADER_RESOUCE;
			desc.type = SV_GFX_IMAGE_TYPE_SHADER_RESOURCE;
			desc.usage = SV_GFX_USAGE_STATIC;
			desc.CPUAccess = SV_GFX_CPU_ACCESS_NONE;
			desc.dimension = 2u;
			desc.width = width;
			desc.height = height;
			desc.depth = 1u;
			desc.layers = 1u;
			
			svCheck(graphics_image_create(&desc, m_DrawData->image));
		}
		// Create Sampler
		{
			SV_GFX_SAMPLER_DESC desc;

			desc.addressModeU = addressMode;
			desc.addressModeV = addressMode;
			desc.addressModeW = addressMode;
			desc.minFilter = linearFilter ? SV_GFX_FILTER_LINEAR : SV_GFX_FILTER_NEAREST;
			desc.magFilter = desc.minFilter;

			svCheck(graphics_sampler_create(&desc, m_DrawData->sampler));
		}

		return true;
	}

	bool TextureAtlas::Destroy()
	{
		if (m_DrawData.get()) {
			svCheck(graphics_destroy(m_DrawData->image));
			svCheck(graphics_destroy(m_DrawData->sampler));
			m_DrawData->sprites.clear();
		}
		return true;
	}

	Sprite TextureAtlas::AddSprite(float x, float y, float w, float h)
	{
		SV_ASSERT(m_DrawData.get() != nullptr);
		Sprite res = { m_DrawData.get(), ui32(m_DrawData->sprites.size()) };
		m_DrawData->sprites.emplace_back(x, y, x + w, y + h);
		return res;
	}

	vec4 TextureAtlas::operator[](ui32 ID) const noexcept
	{
		SV_ASSERT(m_DrawData.get() != nullptr);
		return m_DrawData->sprites[ID];
	}

}