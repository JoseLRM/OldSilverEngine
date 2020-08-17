#include "core.h"
#include "TextureAtlas.h"

namespace sv {

	bool TextureAtlas::CreateFromFile(const char* filePath, bool linearFilter, SamplerAddressMode addressMode)
	{
		// Get file data
		void* data;
		ui32 width;
		ui32 height;
		svCheck(utils_loader_image(filePath, &data, &width, &height));

		// Create Image
		{
			GPUImageDesc desc;

			desc.pData = data;
			desc.size = width * height * 4u;
			desc.format = Format_R8G8B8A8_UNORM;
			desc.layout = GPUImageLayout_ShaderResource;
			desc.type = GPUImageType_ShaderResource;
			desc.usage = ResourceUsage_Static;
			desc.CPUAccess = CPUAccess_None;
			desc.dimension = 2u;
			desc.width = width;
			desc.height = height;
			desc.depth = 1u;
			desc.layers = 1u;

			svCheck(graphics_image_create(&desc, m_Image));
		}
		// Create Sampler
		{
			SamplerDesc desc;

			desc.addressModeU = addressMode;
			desc.addressModeV = addressMode;
			desc.addressModeW = addressMode;
			desc.minFilter = linearFilter ? SamplerFilter_Linear : SamplerFilter_Nearest;
			desc.magFilter = desc.minFilter;

			svCheck(graphics_sampler_create(&desc, m_Sampler));
		}

		return true;
	}

	bool TextureAtlas::Destroy()
	{
		svCheck(graphics_destroy(m_Image));
		svCheck(graphics_destroy(m_Sampler));
		m_Sprites.clear();
		return true;
	}

	Sprite TextureAtlas::AddSprite(float x, float y, float w, float h)
	{
		Sprite res = { this, ui32(m_Sprites.size()) };
		m_Sprites.emplace_back(x, y, x + w, y + h);
		return res;
	}

}