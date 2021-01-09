#pragma once

#include "core/graphics.h"

namespace sv {

	struct DebugRenderer {

		~DebugRenderer();

		Result create();
		Result destroy();

		void reset();
		void render(GPUImage* renderTarget, const Viewport& viewport, const Scissor& scissor, const XMMATRIX& viewProjectionMatrix, CommandList cmd);

		// Draw calls

		void drawQuad(const XMMATRIX& matrix, Color color);
		void drawLine(const v3_f32& p0, const v3_f32& p1, Color color);
		void drawEllipse(const XMMATRIX& matrix, Color color);
		void drawSprite(const XMMATRIX& matrix, Color color, GPUImage* image);

		// Helper draw calls

		void drawQuad(const v3_f32& position, const v2_f32& size, Color color);
		void drawQuad(const v3_f32& position, const v2_f32& size, const v3_f32& rotation, Color color);
		void drawQuad(const v3_f32& position, const v2_f32& size, const v4_f32& rotationQuat, Color color);

		void drawEllipse(const v3_f32& position, const v2_f32& size, Color color);
		void drawEllipse(const v3_f32& position, const v2_f32& size, const v3_f32& rotation, Color color);
		void drawEllipse(const v3_f32& position, const v2_f32& size, const v4_f32& rotationQuat, Color color);

		void drawSprite(const v3_f32& position, const v2_f32& size, Color color, GPUImage* image);
		void drawSprite(const v3_f32& position, const v2_f32& size, const v3_f32& rotation, Color color, GPUImage* image);
		void drawSprite(const v3_f32& position, const v2_f32& size, const v4_f32& rotationQuat, Color color, GPUImage* image);

		// Attributes

		// Line rasterization width (pixels)
		void	setlinewidth(f32 lineWidth);
		f32		getlinewidth();

		// Quad and ellipse stroke: 1.f renders normally, 0.01f renders thin stroke around
		void	setStroke(f32 stroke);
		f32		getStroke();

		// Sprite texCoords
		void	setTexcoord(const v4_f32& texCoord);
		v4_f32	getTexcoord();

		// Sprite sampler
		void setSamplerDefault();
		void setSampler(Sampler* sampler);

		// High level draw calls

		void drawOrthographicGrip(const v2_f32& position, const v2_f32& size, float gridSize, Color color);

	private:
		void* pInternal;

	};

}