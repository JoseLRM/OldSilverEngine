#include "core.h"

#include "Engine.h"

#include "Graphics_dx11.h"

///////////////////Initialization Parameters////////////////////////////////
void SV_ENGINE_INITIALIZATION_DESC::SetDefault()
{
	executeInThisThread = true;
	windowDesc.x = 320;
	windowDesc.y = 180;
	windowDesc.width = 1280;
	windowDesc.height = 720;
	windowDesc.title = "SilverEngine";

	rendererDesc.windowAttachment.enabled = true;
	rendererDesc.windowAttachment.resolution = 1920u;
}

/////////////////////Static Initialization///////////////////////////////////////////
void SV_ENGINE_STATIC_INITIALIZATION_DESC::SetDefault()
{
	showConsole = true;
	minThreadsCount = 2u;
}
namespace SV {
	bool Initialize(const SV_ENGINE_STATIC_INITIALIZATION_DESC* d)
	{
		SV_ENGINE_STATIC_INITIALIZATION_DESC desc;
		if (d) desc = *d;
		else desc.SetDefault();

		if (!SV::User::_internal::Initialize()) {
			SV::LogE("Can't initialize User Data");
		}

		if (desc.showConsole) SV::ShowConsole();
		else SV::HideConsole();

		SV::Timer::_internal::Initialize();

		SV::Task::_internal::Initialize(desc.minThreadsCount);
		if (!SV::_internal::RegisterWindowClass()) return false;
		return true;
	}
	bool Close()
	{
		SV::Task::_internal::Close();
		return true;
	}
}

///////////////////////////////////////////////////////////////////////////////////////

ui32 g_EngineInstanceIDCount = 0;
std::mutex g_ReserveInstanceMutex;

ui32 ReserveInstance()
{
	std::lock_guard<std::mutex> lock(g_ReserveInstanceMutex);
	return ++g_EngineInstanceIDCount;
}

namespace SV {

	Engine::Engine() : m_EngineState(SV_ENGINE_STATE_NONE), m_Application(nullptr)
	{
		m_Window.SetEngine(this);
		m_Renderer.SetEngine(this);
		m_Input.SetEngine(this);
	}
	Engine::~Engine()
	{}

	void Engine::Run(Application* app, const SV_ENGINE_INITIALIZATION_DESC* d)
	{
		SV_ASSERT(app);
		m_Application = app;
		m_Application->SetEngine(this);

		// Initialization Parameters
		SV_ENGINE_INITIALIZATION_DESC desc;
		if (!d) desc.SetDefault();
		else desc = *d;

		// Set Instance ID
		m_InstanceID = ReserveInstance();

		// Run
		if (desc.executeInThisThread) _Run(desc);
		else {
			SV::Task::Execute([this, &desc]() {
				_Run(desc);
			}, nullptr, true);
		}
	}
	void Engine::_Run(SV_ENGINE_INITIALIZATION_DESC& desc)
	{
		m_Name = "SilverEngine ";
		m_Name += m_Version.ToString();

		try {
			// Initialize
			m_EngineState = SV_ENGINE_STATE_INITIALIZING;
			if (!Initialize(desc)) return;

			// Run
			m_EngineState = SV_ENGINE_STATE_RUNNING;
			Loop();

			// Close
			m_EngineState = SV_ENGINE_STATE_CLOSING;
			if(!Close()) return;
			
		}
		catch (SV::_internal::Exception e) {
			SV::LogE("%s: '%s'\nFile: '%s', Line: %u", e.type.c_str(), e.desc.c_str(), e.file.c_str(), e.line);
			Exit(false);
		}
		catch (std::exception e) {
			SV::LogE("STD Exception: '%s'", e.what());
			Exit(false);
		}
		catch (int i) {
			SV::LogE("Unknown Error: %i", i);
			Exit(false);
		}
		catch (...) {
			SV::LogE("Unknown Error");
			Exit(false);
		}

		m_EngineState = SV_ENGINE_STATE_NONE;
	}

	bool Engine::Initialize(SV_ENGINE_INITIALIZATION_DESC& desc)
	{
		SV::LogSeparator();
		SV::LogI("Initializing %s", m_Name.c_str());

		// WINDOW
		if (!m_Window.Initialize(desc.windowDesc)) {
			SV::LogE("Can't initialize Window");
			return false;
		}

		// GRAPHICS
		m_Graphics = std::make_unique<SV::DirectX11Device>();
		m_Graphics->SetEngine(this);
		if (!m_Graphics->Initialize(desc.graphicsDesc)) {
			SV::LogE("Can't initialize Graphics");
			return false;
		}

		// RENDERER
		if (!m_Renderer.Initialize(desc.rendererDesc)) {
			SV::LogE("Can't initialize Renderer");
			return false;
		}

#ifdef SV_IMGUI
		SVImGui::_internal::Initialize(m_Window, GetGraphics());
#endif

		// APPLICATION
		m_Application->Initialize();

		return true;
	}
	void Engine::Loop()
	{
		SV::LogSeparator();
		SV::LogI("Running %s", m_Name.c_str());

		Time lastTime;
		Time fixedUpdateTime;
		Time fixedUpdateFrameRate = 1 / 60.f;

		ui32 frameCount = 0;
		Time frameCountTime = 0.f;

		while (m_Window.UpdateInput() && m_EngineState == SV_ENGINE_STATE_RUNNING)
		{
			Time now = Timer::Now();

			Time deltaTime = lastTime.TimeSince(now);
			lastTime = now;

			fixedUpdateTime = fixedUpdateTime + deltaTime;

			// FPS Count
			frameCountTime = frameCountTime + deltaTime;
			if (frameCountTime >= 1.f) {
				frameCountTime = frameCountTime - 1.f;
				SV::LogI("FPS = %u", frameCount);
				frameCount = 0u;
			}
			frameCount++;

			// Begin
			m_Graphics->BeginFrame();
			m_Renderer.BeginFrame();

			// Updating
			m_Application->Update(deltaTime);
			if (fixedUpdateTime >= fixedUpdateFrameRate) {
				m_Application->FixedUpdate();
				fixedUpdateTime = fixedUpdateTime - fixedUpdateFrameRate;
			}

			// Rendering
			m_Application->Render();
			m_Renderer.Render();

			m_Renderer.EndFrame();
			m_Graphics->EndFrame();
		}
	}
	bool Engine::Close()
	{
		SV::LogSeparator();
		SV::LogI("Closing %s", m_Name.c_str());

		// APPLICATION
		m_Application->Close();
		m_Application = nullptr;

		// RENDERER
		if (!m_Renderer.Close()) {
			SV::LogE("Can't close Renderer");
		}

#ifdef SV_IMGUI
		SVImGui::_internal::Close();
#endif

		// WINDOW
		if (!m_Window.Close()) {
			SV::LogE("Can't close Window");
		}

		// GRAPHICS
		if (!m_Graphics->Close()) {
			SV::LogE("Can't close Graphics");
		}
		m_Graphics.reset();

		return true;
	}
	void Engine::Exit(bool endFrame)
	{
		if (m_EngineState == SV_ENGINE_STATE_NONE || m_EngineState == SV_ENGINE_STATE_CLOSING) exit(1);
		else {
			m_EngineState = SV_ENGINE_STATE_CLOSING;
			if (!endFrame) {
				if (!Close()) {
					SV::LogE("Can't close %s", m_Name.c_str());
				}
			}
		}
	}

}