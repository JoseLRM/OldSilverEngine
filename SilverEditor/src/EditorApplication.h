#pragma once

#include "core.h"
#include "editor.h"

namespace sve {

	class EditorApplication : public sv::Application
	{
	public:

		void Initialize() override
		{
			editor_initialize();
		}

		void Update(float dt) override
		{
			editor_update(dt);
		}
		void Render() override
		{
			editor_render();
		}
		void Close() override
		{
			editor_close();
		}
	};

}