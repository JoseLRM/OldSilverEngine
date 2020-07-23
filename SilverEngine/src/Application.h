#pragma once

#include "core.h"

namespace SV {

	class Application {

	public:
		Application();
		~Application();

		virtual void Initialize() {}
		virtual void Update(float dt) {}
		virtual void FixedUpdate() {}
		virtual void Render() {}
		virtual void Close() {}

	};

}