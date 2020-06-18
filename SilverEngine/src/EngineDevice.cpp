#include "core.h"

#include "Engine.h"

namespace SV {

	Graphics& EngineDevice::GetGraphics() const noexcept { return m_pEngine->GetGraphics(); }
	Renderer& EngineDevice::GetRenderer() const noexcept { return m_pEngine->GetRenderer(); }
	Window& EngineDevice::GetWindow() const noexcept { return m_pEngine->GetWindow(); }
	Input& EngineDevice::GetInput() const noexcept { return m_pEngine->GetInput(); }
	StateManager& EngineDevice::GetStateManager() const noexcept { return m_pEngine->GetStateManager(); }
	Application& EngineDevice::GetApplication() const noexcept { return m_pEngine->GetApplication(); }

}