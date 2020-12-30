#include "core.h"

#include "render_utils_internal.h"

namespace sv {

	static constexpr Time AUXIMG_UNUSED_GLOBAL_TIME = 7.f;
	static constexpr Time AUXIMG_UNUSED_FREE_TIME = 15.f;

	struct AuxImage {
		
		GPUImage* image;
		GPUImageLayout layout;
		Time lastTimeUsed;
	};

	struct AuxImageList {

		std::vector<AuxImage>	images;
		std::vector<u64>		ids;

	};

	static AuxImageList g_GlobalImages;
	static std::mutex g_GlobalMutex;

	static AuxImageList g_Images[GraphicsLimit_CommandList];

	SV_INLINE static size_t getImageIndex(const AuxImageList& list, u64 id)
	{
		const u64* it = list.ids.data();
		const u64* end = it + list.ids.size();

		while (it != end) {

			if (id == *it) {

				size_t index = it - list.ids.data();
				
				if (list.images[index].lastTimeUsed != 0.f)
					return index;
			}

			++it;
		}

		return SIZE_MAX;
	}

	SV_INLINE static void prepareImageToReturn(AuxImage& img, GPUImageLayout layout, CommandList cmd)
	{
		img.lastTimeUsed = 0.f;

		if (img.layout != layout) {
			
			GPUBarrier barrier;
			barrier = GPUBarrier::Image(img.image, img.layout, layout);
			graphics_barrier(&barrier, 1u, cmd);
			img.layout = layout;
		}
	}

	SV_INLINE static void destroyImage(GPUImage* image)
	{
		Result res = graphics_destroy(image);
		if (result_fail(res)) {
			SV_LOG_ERROR("Can't destroy a auxiliar image successfully. ErrorCode %u", res);
		}
	}

	Result auximg_initialize()
	{
		return Result_Success;
	}

	Result auximg_close()
	{
		foreach(i, GraphicsLimit_CommandList) {

			AuxImageList& list = g_Images[i];

			for (auto it = list.images.cbegin(); it != list.images.cend(); ++it) {
				
				// Make sure that all the images are poped
				SV_ASSERT(it->lastTimeUsed != 0.f);
				destroyImage(it->image);
			}

			list.ids.clear();
			list.images.clear();
		}

		{
			std::scoped_lock lock(g_GlobalMutex);

			for (auto it = g_GlobalImages.images.cbegin(); it != g_GlobalImages.images.cend(); ++it) {
				destroyImage(it->image);
			}

			g_GlobalImages.images.clear();
			g_GlobalImages.ids.clear();
		}

		return Result_Success;
	}

	void auximg_update(Time now)
	{
		// Move on unused Images from cmd to global
		foreach(i, GraphicsLimit_CommandList) {

			AuxImageList& list = g_Images[i];
			
			for (auto it = list.images.cbegin(); it != list.images.cend(); ) {
				
				// Make sure that all the images are poped
				SV_ASSERT(it->lastTimeUsed != 0.f);

				if (now - it->lastTimeUsed >= AUXIMG_UNUSED_GLOBAL_TIME) {
					
					size_t index = it - list.images.begin();

					{
						std::scoped_lock lock(g_GlobalMutex);

						g_GlobalImages.images.push_back(*it);
						g_GlobalImages.ids.push_back(list.ids[index]);
					}

					it = list.images.erase(it);
					list.ids.erase(list.ids.begin() + index);
				}
				else ++it;
			}
		}

		// Free unused images
		{
			std::scoped_lock lock(g_GlobalMutex);

			for (auto it = g_GlobalImages.images.cbegin(); it != g_GlobalImages.images.cend(); ) {

				if (now - it->lastTimeUsed >= AUXIMG_UNUSED_FREE_TIME) {
					
					// Destroy image
					Result res = graphics_destroy(it->image);
					if (result_fail(res)) {
						SV_LOG_ERROR("Can't destroy a auxiliar image successfully. ErrorCode %u", res);
					}

					// Remove from lists
					size_t index = it - g_GlobalImages.images.begin();

					it = g_GlobalImages.images.erase(it);
					g_GlobalImages.ids.erase(g_GlobalImages.ids.begin() + index);
				}
				else ++it;
			}
		}
	}

	u64	auximg_id(u32 width, u32 height, Format format, GPUImageTypeFlags imageType)
	{
		// TODO: More image data

		SV_ASSERT(u16(width) == width && u16(height) == height && u16(format) == format);
		return (u64(width) << 48U) + (u64(height) << 32U) + (u64(imageType) << 16U) + u64(format);
	}

	std::pair<GPUImage*, size_t> auximg_push(u64 id, GPUImageLayout layout, CommandList cmd)
	{
		size_t index;

		// Get from cmd list
		index = getImageIndex(g_Images[cmd], id);
		
		if (index != SIZE_MAX) {

			AuxImage& img = g_Images[cmd].images[index];
			
			prepareImageToReturn(img, layout, cmd);
			
			return std::make_pair(img.image, index);
		}
		
		// If not found get from global
		{
			std::scoped_lock lock(g_GlobalMutex);

			index = getImageIndex(g_GlobalImages, id);

			if (index != SIZE_MAX) {

				AuxImage img = g_GlobalImages.images[index];
				prepareImageToReturn(img, layout, cmd);

				g_GlobalImages.images.erase(g_GlobalImages.images.begin() + index);
				g_GlobalImages.ids.erase(g_GlobalImages.ids.begin() + index);

				index = g_Images[cmd].images.size();

				g_Images[cmd].images.push_back(img);
				g_Images[cmd].ids.push_back(id);

				return std::make_pair(img.image, index);
			}
		}

		// If not exist, create new one and add to cmd list
		{
			AuxImage newImg;

			Format format = Format(id & u64(u16_max));
			GPUImageType imageType = GPUImageType((id >> 16u) & u64(u16_max));
			u32 height = u32((id >> 32u) & u64(u16_max));
			u32 width = u32((id >> 48u) & u64(u16_max));

			GPUImageDesc desc;
			desc.pData = nullptr;
			desc.size = 0u;
			desc.format = format;
			desc.layout = layout;
			desc.type = imageType;
			desc.usage = ResourceUsage_Static;
			desc.CPUAccess = CPUAccess_None;
			desc.dimension = 2u;
			desc.width = width;
			desc.height = height;
			desc.depth = 1u;
			desc.layers = 1u;

			Result res = graphics_image_create(&desc, &newImg.image);

			if (result_okay(res)) {
				
				graphics_name_set(newImg.image, "AuxiliarImage");

				newImg.layout = layout;
				newImg.lastTimeUsed = 0.f;

				// Save it in the list
				size_t index = g_Images[cmd].images.size();

				g_Images[cmd].images.push_back(newImg);
				g_Images[cmd].ids.push_back(id);

				return std::make_pair(newImg.image, index);
			}
			else {
				SV_LOG_ERROR("Can't create a auxiliar image. ErrorCode %u", res);
				return {nullptr, 0u};
			}
		}
	}

	void auximg_pop(size_t index, GPUImageLayout layout, CommandList cmd)
	{
		AuxImageList& list = g_Images[cmd];
		SV_ASSERT(list.images.size() > index);

		AuxImage& img = list.images[index];
		img.lastTimeUsed = timer_now();
		img.layout = layout;
	}

}