#pragma once

#include "core.h"

namespace sve {

	class ImGuiDevice {
	protected:
		ImGuiContext* m_Ctx = nullptr;

	public:
		virtual bool Initialize() = 0;
		virtual bool Close() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual ImTextureID ParseImage(sv::Image& image) = 0;

		inline ImGuiContext* GetCtx() const noexcept { return m_Ctx; }

	};

	std::unique_ptr<ImGuiDevice> device_create();

}