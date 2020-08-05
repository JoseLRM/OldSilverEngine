#pragma once

#include "renderer_desc.h"
#include "..//scene/Scene.h"

namespace _sv {

	bool renderer_layer_initialize();
	bool renderer_layer_close();
	void renderer_layer_begin();
	void renderer_layer_end();
	void renderer_layer_prepare_scene();
	void renderer_layer_render(sv::CommandList cmd);

}

namespace sv {

	RenderLayerID renderer_layer_create(i16 sortValue, SV_REND_SORT_MODE sortMode, bool transparent);
	i16 renderer_layer_get_sort_value(RenderLayerID renderLayer);
	SV_REND_SORT_MODE renderer_layer_get_sort_mode(RenderLayerID renderLayer);
	bool renderer_layer_get_transparent(RenderLayerID renderLayer);
	void renderer_layer_set_sort_value(i16 value, RenderLayerID renderLayer);
	void renderer_layer_set_sort_mode(SV_REND_SORT_MODE mode, RenderLayerID renderLayer);
	void renderer_layer_set_transparent(bool transparent, RenderLayerID renderLayer);
	void renderer_layer_destroy(RenderLayerID renderLayer);

}