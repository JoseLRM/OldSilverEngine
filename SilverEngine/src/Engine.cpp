#include "core.h"

#include "Engine.h"

///////////////////Initialization Parameters////////////////////////////////

void SV_ENGINE_INITIALIZATION_DESC::SetDefault()
{
	executeInThisThread = true;
	windowDesc.x = 320;
	windowDesc.y = 180;
	windowDesc.width = 1280;
	windowDesc.height = 720;
	windowDesc.title = "SilverEngine";
}

////////////////////////////////////////////////////////////////

namespace SV {

	Engine::Engine() : m_EngineState(SV_ENGINE_STATE_NONE)
	{}
	Engine::~Engine()
	{}

	void Engine::Run(const SV_ENGINE_INITIALIZATION_DESC* d)
	{
		// Initialization Parameters
		SV_ENGINE_INITIALIZATION_DESC desc;
		if (!d) desc.SetDefault();
		else desc = *d;

		std::string nameString = "SilverEngine ";
		nameString += m_Version.ToString();

		try {
			// Initialize
			SV::LogI("Initializing %s", nameString.c_str());
			m_EngineState = SV_ENGINE_STATE_INITIALIZING;

			if (!m_Window.Initialize(&desc.windowDesc)) {
				SV::LogE("Can't initialize Window");
				return;
			}

			// Run
			SV::LogI("Running %s", nameString.c_str());
			m_EngineState = SV_ENGINE_STATE_RUNNING;

			while (m_Window.UpdateInput())
			{
				
			}

			// Close
			SV::LogI("Closing %s", nameString.c_str());
			m_EngineState = SV_ENGINE_STATE_CLOSING;
			
			if (!m_Window.Close()) {
				SV::LogE("Can't close Window");
			}
		}
		catch (...) {
			SV::LogE("Unknown Error");
		}

		m_EngineState = SV_ENGINE_STATE_NONE;
	}

}