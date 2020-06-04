#include "core.h"

#include "Engine.h"
#include "TaskSystem.h"

///////////////////Initialization Parameters////////////////////////////////

void SV_ENGINE_INITIALIZATION_DESC::SetDefault()
{
	executeInThisThread = true;
	windowDesc.showConsole = true;
	windowDesc.x = 320;
	windowDesc.y = 180;
	windowDesc.width = 1280;
	windowDesc.height = 720;
	windowDesc.title = "SilverEngine";
}

////////////////////////////////////////////////////////////////

ui32 g_EngineInstanceIDCount = 0;
std::mutex g_ReserveInstanceMutex;

ui32 ReserveInstance()
{
	std::lock_guard<std::mutex> lock(g_ReserveInstanceMutex);
	return ++g_EngineInstanceIDCount;
}

namespace SV {
	bool Initialize()
	{
		SV::task::_internal::Initialize(2);
		if(!SV::_internal::RegisterWindowClass()) return false;
		return true;
	}
	bool Close()
	{
		SV::task::_internal::Close();
		return true;
	}
}

namespace SV {

	Engine::Engine() : m_EngineState(SV_ENGINE_STATE_NONE)
	{
		m_Window.SetEngine(this);
		m_Input.SetEngine(this);
	}
	Engine::~Engine()
	{}

	void Engine::Run(const SV_ENGINE_INITIALIZATION_DESC* d)
	{
		// Initialization Parameters
		SV_ENGINE_INITIALIZATION_DESC desc;
		if (!d) desc.SetDefault();
		else desc = *d;

		// Set Instance ID
		m_InstanceID = ReserveInstance();

		// Run
		if (desc.executeInThisThread) _Run(desc);
		else {
			SV::task::Execute([this, &desc]() {
				_Run(desc);
			}, nullptr, true);
		}
	}
	void Engine::_Run(SV_ENGINE_INITIALIZATION_DESC& desc)
	{
		std::string nameString = "SilverEngine ";
		nameString += m_Version.ToString();

		try {
			// Initialize
			SV::LogI("Initializing %s", nameString.c_str());
			m_EngineState = SV_ENGINE_STATE_INITIALIZING;
			if (!Initialize(desc)) return;
			

			// Run
			SV::LogI("Running %s", nameString.c_str());
			m_EngineState = SV_ENGINE_STATE_RUNNING;
			Loop();

			// Close
			SV::LogI("Closing %s", nameString.c_str());
			m_EngineState = SV_ENGINE_STATE_CLOSING;
			if(!Close()) return;
			
		}
		catch (std::exception e) {
			SV::LogE("STD Exception: '%s'", e.what());
		}
		catch (int i) {
			SV::LogE("Unknown Error: %i", i);
		}
		catch (...) {
			SV::LogE("Unknown Error");
		}

		m_EngineState = SV_ENGINE_STATE_NONE;
	}

	bool Engine::Initialize(SV_ENGINE_INITIALIZATION_DESC& desc)
	{
		if (!m_Window.Initialize(&desc.windowDesc)) {
			SV::LogE("Can't initialize Window");
			return false;
		}

		return true;
	}
	void Engine::Loop()
	{
		while (m_Window.UpdateInput())
		{
			if (m_Input.IsMousePressed(SV_MOUSE_CENTER)) SV::Log("All right");
		}
	}
	bool Engine::Close()
	{
		if (!m_Window.Close()) {
			SV::LogE("Can't close Window");
		}

		return true;
	}

}