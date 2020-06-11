#pragma once

namespace SV {

	class Engine;
	class Graphics;
	class Renderer;
	class Window;
	class Input;
	class Application;

	class EngineDevice {
		Engine* m_pEngine = nullptr;
	public:
		inline void SetEngine(Engine* pEngine) noexcept { m_pEngine = pEngine; }
		inline Engine& GetEngine() const noexcept { return *m_pEngine; }

		Graphics& GetGraphics() const noexcept;
		Renderer& GetRenderer() const noexcept;
		Window& GetWindow() const noexcept;
		Input& GetInput() const noexcept;
		Application& GetApplication() const noexcept;

	};

}