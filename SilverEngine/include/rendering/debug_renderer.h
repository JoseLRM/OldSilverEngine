#pragma once

#include "platform/graphics.h"

namespace sv {

	struct DebugRenderer {

		~DebugRenderer();

		Result create();
		Result destroy();

		void reset();
		void render(GPUImage* renderTarget, const Viewport& viewport, const Scissor& scissor, const XMMATRIX& viewProjectionMatrix, CommandList cmd);

		// Draw calls

		void drawQuad(const XMMATRIX& matrix, Color color);
		void drawLine(const vec3f& p0, const vec3f& p1, Color color);
		void drawEllipse(const XMMATRIX& matrix, Color color);
		void drawSprite(const XMMATRIX& matrix, Color color, GPUImage* image);

		// Helper draw calls

		void drawQuad(const vec3f& position, const vec2f& size, Color color);
		void drawQuad(const vec3f& position, const vec2f& size, const vec3f& rotation, Color color);
		void drawQuad(const vec3f& position, const vec2f& size, const vec4f& rotationQuat, Color color);

		void drawEllipse(const vec3f& position, const vec2f& size, Color color);
		void drawEllipse(const vec3f& position, const vec2f& size, const vec3f& rotation, Color color);
		void drawEllipse(const vec3f& position, const vec2f& size, const vec4f& rotationQuat, Color color);

		void drawSprite(const vec3f& position, const vec2f& size, Color color, GPUImage* image);
		void drawSprite(const vec3f& position, const vec2f& size, const vec3f& rotation, Color color, GPUImage* image);
		void drawSprite(const vec3f& position, const vec2f& size, const vec4f& rotationQuat, Color color, GPUImage* image);

		// Attributes

		// Line rasterization width (pixels)
		void	setlinewidth(f32 lineWidth);
		f32		getlinewidth();

		// Quad and ellipse stroke: 1.f renders normally, 0.01f renders thin stroke around
		void	setStroke(f32 stroke);
		f32		getStroke();

		// Sprite texCoords
		void	setTexcoord(const vec4f& texCoord);
		vec4f	getTexcoord();

		// Sprite sampler
		void setSamplerDefault();
		void setSampler(Sampler* sampler);

		// High level draw calls

		void drawOrthographicGrip(const vec2f& position, const vec2f& size, float gridSize, Color color);

	private:
		void* pInternal;

	};

}