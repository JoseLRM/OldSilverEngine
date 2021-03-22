#include "SilverEngine/core.h"

#include "SilverEngine/dev.h"

namespace sv {

	constexpr f32 VPADDING = 5.f;
	constexpr f32 SEPARATOR = 30.f;
	constexpr f32 COMP_ITEM_HEIGHT = 30.f;

	struct EditorWindowInfo {
		f32 yoff;
	};

	static List<EditorWindowInfo> window_info;

	bool egui_begin()
	{
		gui_begin(dev.gui, f32(window_width_get(engine.window)), f32(window_height_get(engine.window)));
		
		

		return true;
	}

	void egui_end()
	{
		SV_ASSERT(window_info.empty());
		gui_end(dev.gui);
	}

	bool egui_begin_window(const char* title)
	{
		if (gui_begin_window(dev.gui, title)) {
			
			EditorWindowInfo& info = window_info.emplace_back();
			info.yoff = 0.f;

			return true;
		}

		return false;
	}

	void egui_end_window()
	{
		window_info.pop_back();
		gui_end_window(dev.gui);
	}

	void egui_header(const char* text, u64 id)
	{
		constexpr f32 HEIGHT = 10.f;

		EditorWindowInfo& info = window_info.back();

		gui_text(dev.gui, text, id, GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + 40.f));
		info.yoff += 40.f + SEPARATOR + VPADDING;
	}

	void egui_transform(Entity entity)
	{
		constexpr f32 TRANSFORM_HEIGHT = 30.f;

		EditorWindowInfo& info = window_info.back();

		// TODO: euler rotation
		Transform trans = get_entity_transform(engine.scene, entity);
		v3_f32 position = trans.getLocalPosition();
		v3_f32 scale = trans.getLocalScale();

		GuiContainerStyle container_style;
		container_style.color = Color::White(0u);

		foreach(i, 2u) {

			gui_push_id(dev.gui, i + 0x23549abf);

			v3_f32* values;

			switch (i) {
			case 0:
				values = &position;
				break;
			case 1:
				values = &scale;
				break;
			}

			// TODO: Hot color
			GuiButtonStyle x_style;
			x_style.color = { 229u, 25u, 25u, 255u };

			GuiButtonStyle y_style;
			y_style.color = { 51u, 204u, 51u, 255u };

			GuiButtonStyle z_style;
			z_style.color = { 13u, 25u, 229u, 255u };

			constexpr f32 EXTERN_PADDING = 0.03f;
			constexpr f32 INTERN_PADDING = 0.07f;

			constexpr f32 ELEMENT_WIDTH = (1.f - EXTERN_PADDING * 2.f - INTERN_PADDING * 2.f) / 3.f;

			// X
			{
				gui_begin_container(dev.gui, 1u,
					GuiCoord::Relative(0.5f - ELEMENT_WIDTH * 1.5f - INTERN_PADDING),
					GuiCoord::Relative(0.5f - ELEMENT_WIDTH * 0.5f - INTERN_PADDING),
					GuiCoord::IPixel(info.yoff),
					GuiCoord::IPixel(info.yoff + TRANSFORM_HEIGHT),
					container_style
				);

				if (gui_button(dev.gui, "X", 0u, GuiCoord::Relative(0.f), GuiCoord::Relative(0.25f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), x_style)) {

					if (i == 1u)
						values->x = 1.f;
					else
						values->x = 0.f;
				}

				gui_drag_f32(dev.gui, &values->x, 0.1f, 1u, GuiCoord::Relative(0.25f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));

				gui_end_container(dev.gui);
			}

			// Y
			{
				gui_begin_container(dev.gui, 2u,
					GuiCoord::Relative(0.5f - ELEMENT_WIDTH * 0.5f),
					GuiCoord::Relative(0.5f + ELEMENT_WIDTH * 0.5f),
					GuiCoord::IPixel(info.yoff),
					GuiCoord::IPixel(info.yoff + TRANSFORM_HEIGHT),
					container_style
				);

				if (gui_button(dev.gui, "Y", 0u, GuiCoord::Relative(0.f), GuiCoord::Relative(0.25f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), y_style)) {

					if (i == 1u)
						values->y = 1.f;
					else
						values->y = 0.f;
				}

				gui_drag_f32(dev.gui, &values->y, 0.1f, 1u, GuiCoord::Relative(0.25f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));

				gui_end_container(dev.gui);
			}

			// Z
			{
				gui_begin_container(dev.gui, 3u,
					GuiCoord::Relative(0.5f + ELEMENT_WIDTH * 0.5f + INTERN_PADDING),
					GuiCoord::Relative(0.5f + ELEMENT_WIDTH * 1.5f + INTERN_PADDING),
					GuiCoord::IPixel(info.yoff),
					GuiCoord::IPixel(info.yoff + TRANSFORM_HEIGHT),
					container_style
				);

				if (gui_button(dev.gui, "Z", 0u, GuiCoord::Relative(0.f), GuiCoord::Relative(0.25f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), z_style)) {

					if (i == 1u)
						values->z = 1.f;
					else
						values->z = 0.f;
				}

				gui_drag_f32(dev.gui, &values->z, 0.1f, 1u, GuiCoord::Relative(0.25f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));

				gui_end_container(dev.gui);
			}

			gui_pop_id(dev.gui);

			info.yoff += TRANSFORM_HEIGHT + VPADDING;
		}

		info.yoff += SEPARATOR;

		trans.setPosition(position);
		trans.setScale(scale);
	}

	bool egui_button(const char* text, u64 id)
	{
		constexpr f32 HEIGHT = 10.f;

		EditorWindowInfo& info = window_info.back();

		bool res = gui_button(dev.gui, text, id, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + HEIGHT));
		info.yoff += HEIGHT + VPADDING;
		return res;
	}

	bool egui_begin_component(Entity entity, CompID comp_id, bool* remove)
	{
		GuiContainerStyle container_style;
		container_style.color = Color::Red(100u);

		GuiLabelStyle text_style;
		text_style.text_color = Color::Black();
		text_style.text_alignment = TextAlignment_Left;

		GuiCheckboxStyle checkbox_style;
		checkbox_style.active_box = GuiBox::Triangle(Color::Black(), true, 0.5f);
		checkbox_style.inactive_box = GuiBox::Triangle(Color::Black(), false, 0.5f);
		checkbox_style.color = Color::Black(0u);

		EditorWindowInfo& info = window_info.back();

		u64 id = u64("SHOW COMPONENT") ^ u64((u64(comp_id) << 32u) + entity);
		gui_push_id(dev.gui, id);

		*remove = false;

		gui_begin_container(dev.gui, 0u, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + 40.f), container_style);

		gui_text(dev.gui, get_component_name(comp_id), 1u, GuiCoord::Pixel(35.f), GuiCoord::IPixel(10.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), text_style);

		if (gui_begin_popup(dev.gui, GuiPopupTrigger_LastWidget, MouseButton_Right, 2u)) {

			*remove = gui_button(dev.gui, "Remove", 0u, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(5.f), GuiCoord::IPixel(25.f));

			gui_end_popup(dev.gui);
		}

		bool show = gui_checkbox(dev.gui, 3u, GuiCoord::Pixel(0.f), GuiCoord::Aspect(), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), checkbox_style);

		gui_end_container(dev.gui);

		info.yoff += 40.f + VPADDING;

		if (show) {

			gui_push_id(dev.gui, "SHOW COMPONENT");
		}
		else {

			gui_pop_id(dev.gui);
		}

		return show;
	}

	void egui_end_component()
	{
		gui_pop_id(dev.gui, 2u);
	}
	
	void egui_comp_color(const char* text, u64 id, Color* pcolor)
	{
		EditorWindowInfo& info = window_info.back();

		gui_push_id(dev.gui, id);

		gui_text(dev.gui, text, 0u, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.5f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + COMP_ITEM_HEIGHT));
		gui_button(dev.gui, "TODO", 1u, GuiCoord::Relative(0.55f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + COMP_ITEM_HEIGHT));
		info.yoff += COMP_ITEM_HEIGHT + VPADDING;

		gui_pop_id(dev.gui);
	}

	void egui_comp_texture(const char* text, u64 id, TextureAsset* texture)
	{
		EditorWindowInfo& info = window_info.back();

		gui_push_id(dev.gui, id);

		gui_text(dev.gui, text, 0u, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.5f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + COMP_ITEM_HEIGHT));
		gui_button(dev.gui, "TODO", 1u, GuiCoord::Relative(0.55f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + COMP_ITEM_HEIGHT));
		info.yoff += COMP_ITEM_HEIGHT + VPADDING;

		gui_pop_id(dev.gui);
	}

	bool egui_comp_bool(const char* text, u64 id, bool* value)
	{
		EditorWindowInfo& info = window_info.back();

		gui_push_id(dev.gui, id);

		gui_text(dev.gui, text, 0u, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.5f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + COMP_ITEM_HEIGHT));

		bool res = gui_checkbox(dev.gui, value, 1u, GuiCoord::Relative(0.55f), GuiCoord::Aspect(), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + COMP_ITEM_HEIGHT));

		gui_pop_id(dev.gui);
		info.yoff += COMP_ITEM_HEIGHT + VPADDING;

		return res;
	}

	bool egui_comp_drag_f32(const char* text, u64 id, f32* value, f32 adv, f32 min, f32 max)
	{
		EditorWindowInfo& info = window_info.back();

		gui_push_id(dev.gui, id);

		gui_text(dev.gui, text, 0u, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.5f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + COMP_ITEM_HEIGHT));

		bool res = gui_drag_f32(dev.gui, value, adv, min, max, 1u, GuiCoord::Relative(0.55f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + COMP_ITEM_HEIGHT));

		gui_pop_id(dev.gui);
		info.yoff += COMP_ITEM_HEIGHT + VPADDING;

		return res;
	}

}
