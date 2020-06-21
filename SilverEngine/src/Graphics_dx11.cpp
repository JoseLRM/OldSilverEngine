#include "core.h"

#include "Graphics_dx11.h"
#include "Window.h"
#include "Engine.h"

#define dxAssert(x) if((x) != 0) SV_THROW("DirectX11 Exception!!", #x);
#define dxCheck(x) if((x) != 0) return false

inline SV::Graphics_dx11& ParseDevice(SV::Graphics& device)
{
	return *reinterpret_cast<SV::Graphics_dx11*>(&device);
}

// SV format to DX11 format
constexpr D3D11_USAGE ParseUsage(SV_GFX_USAGE usage)
{
	switch (usage)
	{
	case SV_GFX_USAGE_DEFAULT:
		return D3D11_USAGE_DEFAULT;
	case SV_GFX_USAGE_STATIC:
		return D3D11_USAGE_IMMUTABLE;
	case SV_GFX_USAGE_DYNAMIC:
		return D3D11_USAGE_DYNAMIC;
	case SV_GFX_USAGE_STAGING:
		return D3D11_USAGE_STAGING;
	default:
		SV::LogE("Invalid GFXUsage '%u'", usage);
		return D3D11_USAGE_DEFAULT;
	}
}
constexpr DXGI_FORMAT ParseFormat(SV_GFX_FORMAT format)
{
	return (DXGI_FORMAT)format;
}
constexpr D3D11_PRIMITIVE_TOPOLOGY ParseTopology(SV_GFX_TOPOLOGY topology)
{
	switch (topology)
	{
	case SV_GFX_TOPOLOGY_POINTS:
		return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	case SV_GFX_TOPOLOGY_LINES:
		return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	case SV_GFX_TOPOLOGY_LINESTRIP:
		return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case SV_GFX_TOPOLOGY_TRIANGLES:
		return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case SV_GFX_TOPOLOGY_TRIANGLESTRIP:
		return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	case SV_GFX_TOPOLOGY_UNDEFINED:
	default:
		SV::LogE("Undefined Topology '%u'", topology);
		return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}
}
constexpr D3D11_TEXTURE_ADDRESS_MODE ParseAddressMode(SV_GFX_TEXTURE_ADDRESS_MODE mode)
{
	return (D3D11_TEXTURE_ADDRESS_MODE)mode;
}
constexpr D3D11_FILTER ParseTextureFilter(SV_GFX_TEXTURE_FILTER filter)
{
	return (D3D11_FILTER)filter;
}
constexpr DXGI_MODE_SCALING ParseScalingMode(SV_GFX_MODE_SCALING mode)
{
	return (DXGI_MODE_SCALING)mode;
}
constexpr DXGI_MODE_SCANLINE_ORDER ParseScanlineOrder(SV_GFX_MODE_SCANLINE_ORDER order)
{
	return (DXGI_MODE_SCANLINE_ORDER)order;
}

constexpr D3D11_BLEND ParseBlendOption(SV_GFX_BLEND opt) {
	return (D3D11_BLEND)opt;
}
constexpr D3D11_BLEND_OP ParseBlendOperation(SV_GFX_BLEND_OP op) {
	return (D3D11_BLEND_OP)op;
}

constexpr ui32 ParseCPUAccess(ui8 cpuAccess)
{
	return ((cpuAccess & SV_GFX_CPU_ACCESS_WRITE) ? D3D11_CPU_ACCESS_WRITE : 0u) | ((cpuAccess & SV_GFX_CPU_ACCESS_READ) ? D3D11_CPU_ACCESS_READ : 0u);
}

constexpr D3D11_COMPARISON_FUNC ParseComparisonFn(SV_GFX_COMPARISON_FN comp)
{
	return (D3D11_COMPARISON_FUNC)comp;
}
constexpr D3D11_STENCIL_OP ParseStencilOp(SV_GFX_STENCIL_OP op)
{
	return (D3D11_STENCIL_OP)op;
}

namespace SV {

	void Graphics_dx11::CreateBackBuffer(const SV::Adapter::OutputMode& outputMode, ui32 width, ui32 height)
	{
		D3D11_RENDER_TARGET_VIEW_DESC backBufferDesc;
		backBufferDesc.Format = ParseFormat(outputMode.format);
		backBufferDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		backBufferDesc.Texture2D.MipSlice = 0u;

		mainFrameBuffer.Set(_AllocateFrameBuffer());

		FrameBuffer_dx11* backBuffer = reinterpret_cast<FrameBuffer_dx11*>(mainFrameBuffer.Get());

		backBuffer->m_Width = width;
		backBuffer->m_Height = height;
		backBuffer->m_Format = outputMode.format;

		dxAssert(swapChain->GetBuffer(0u, __uuidof(ID3D11Resource), &backBuffer->m_Resource));
		dxAssert(device->CreateRenderTargetView(backBuffer->m_Resource.Get(), &backBufferDesc, &backBuffer->m_RenderTargetView));
	}

	bool Graphics_dx11::_Initialize(const SV_GRAPHICS_INITIALIZATION_DESC& desc)
	{
		Window& window = GetEngine().GetWindow();

		// Get OutputMode
		const SV::Adapter::OutputMode& outputMode = GetAdapter().modes[GetOutputModeID()];

		// Create swap chain
		HWND windowHandle = reinterpret_cast<HWND>(window.GetWindowHandle());

		DXGI_SWAP_CHAIN_DESC swapChainDescriptor;
		svZeroMemory(&swapChainDescriptor, sizeof(DXGI_SWAP_CHAIN_DESC));
		swapChainDescriptor.BufferCount = 1;
		swapChainDescriptor.BufferDesc.Format = ParseFormat(outputMode.format);
#ifdef SV_IMGUI
		swapChainDescriptor.BufferDesc.Height = 0;
		swapChainDescriptor.BufferDesc.Width = 0;
#else
		swapChainDescriptor.BufferDesc.Height = outputMode.height;
		swapChainDescriptor.BufferDesc.Width = outputMode.width;
#endif
		swapChainDescriptor.BufferDesc.RefreshRate.Denominator = outputMode.refreshRate.denominator;
		swapChainDescriptor.BufferDesc.RefreshRate.Numerator = outputMode.refreshRate.numerator;
		swapChainDescriptor.BufferDesc.Scaling = ParseScalingMode(outputMode.scaling);
		swapChainDescriptor.BufferDesc.ScanlineOrdering = ParseScanlineOrder(outputMode.scanlineOrdering);
		swapChainDescriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDescriptor.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		swapChainDescriptor.OutputWindow = windowHandle;
		swapChainDescriptor.SampleDesc.Count = 1;
		swapChainDescriptor.SampleDesc.Quality = 0;
		swapChainDescriptor.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDescriptor.Windowed = TRUE;

		bool debugMode = false;

#ifdef SV_DEBUG
		debugMode = true;
#endif

		dxAssert(D3D11CreateDeviceAndSwapChain(
			NULL,
			D3D_DRIVER_TYPE_HARDWARE,
			NULL,
			debugMode ? D3D11_CREATE_DEVICE_DEBUG : 0u,
			NULL,
			0,
			D3D11_SDK_VERSION,
			&swapChainDescriptor,
			&swapChain,
			&device,
			NULL,
			&immediateContext
		));

#ifdef SV_IMGUI
		CreateBackBuffer(outputMode, window.GetWidth(), window.GetHeight());
#else
		CreateBackBuffer(outputMode, outputMode.width, outputMode.height);
#endif

		return true;
	}

	bool Graphics_dx11::_Close()
	{
		// Release Deferred Contexts
		for (ui32 i = 0; i < SV_GFX_COMMAND_LIST_COUNT; ++i)
		{
			deferredContext[i].Reset();
		}

		ReleaseFrameBuffer(mainFrameBuffer);
		immediateContext.Reset();
		swapChain.Reset();
		device.Reset();


		return true;
	}

	void Graphics_dx11::Present()
	{
		// Execute CommandLists
		ui32 cmdID = 0u;
		ComPtr<ID3D11CommandList> cmd;
		while (activeCommandLists.pop(cmdID)) {
			deferredContext[cmdID]->FinishCommandList(FALSE, &cmd);
			immediateContext->ExecuteCommandList(cmd.Get(), TRUE);
			freeCommandLists.push(cmdID);
		}

#ifdef SV_IMGUI		
		FrameBuffer_dx11* backBuffer = reinterpret_cast<FrameBuffer_dx11*>(mainFrameBuffer.Get());
		immediateContext->OMSetRenderTargets(1, backBuffer->m_RenderTargetView.GetAddressOf(), nullptr);
		SVImGui::_internal::EndFrame();
#endif

		swapChain->Present(0u, 0u);

		// Reset state
		for(ui32 i = 0; i < SV_GFX_COMMAND_LIST_COUNT; ++i) 
			viewports[i].Reset();
	}

	CommandList Graphics_dx11::BeginCommandList()
	{
		ui32 ID = 0;

		if (!freeCommandLists.pop(ID)) {
			ID = commandListCount.fetch_add(1);
			SV_ASSERT(ID < SV_GFX_COMMAND_LIST_COUNT);
			dxAssert(device->CreateDeferredContext(0u, &deferredContext[ID]));
		}
		activeCommandLists.push(ID);

		return ID;
	}

	void Graphics_dx11::SetViewport(ui32 slot, float x, float y, float w, float h, float n, float f, SV::CommandList cmd)
	{
		D3D11_VIEWPORT viewport;
		viewport.TopLeftX = x;
		viewport.TopLeftY = y;
		viewport.Width = w;
		viewport.Height = h;
		viewport.MinDepth = n;
		viewport.MaxDepth = f;

		viewports[cmd].SetViewport(slot, viewport);
	}

	void Graphics_dx11::ResizeBackBuffer(ui32 width, ui32 height)
	{
		if (!swapChain.Get()) return;

		// Get OutputMode
		const SV::Adapter::OutputMode& outputMode = GetAdapter().modes[GetOutputModeID()];

		ReleaseFrameBuffer(mainFrameBuffer);

		dxAssert(swapChain->ResizeBuffers(1, width, height, ParseFormat(outputMode.format), DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

		CreateBackBuffer(outputMode, width, height);
	}

	void Graphics_dx11::SetTopology(SV_GFX_TOPOLOGY topology, CommandList cmd)
	{
		deferredContext[cmd]->IASetPrimitiveTopology(ParseTopology(topology));
	}

	void Graphics_dx11::EnableFullscreen()
	{
		swapChain->SetFullscreenState(TRUE, nullptr);
	}
	void Graphics_dx11::DisableFullscreen()
	{
		swapChain->SetFullscreenState(FALSE, nullptr);
	}

	SV::FrameBuffer& Graphics_dx11::GetMainFrameBuffer()
	{
		return mainFrameBuffer;
	}

	void Graphics_dx11::Draw(ui32 vertexCount, ui32 startVertex, CommandList cmd)
	{
		viewports[cmd].Bind(deferredContext[cmd].Get());
		deferredContext[cmd]->Draw(vertexCount, startVertex);
	}
	void Graphics_dx11::DrawIndexed(ui32 indexCount, ui32 startIndex, ui32 startVertex, CommandList cmd)
	{
		viewports[cmd].Bind(deferredContext[cmd].Get());
		deferredContext[cmd]->DrawIndexed(indexCount, startIndex, startVertex);
	}
	void Graphics_dx11::DrawInstanced(ui32 verticesPerInstance, ui32 instances, ui32 startVertex, ui32 startInstance, CommandList cmd)
	{
		viewports[cmd].Bind(deferredContext[cmd].Get());
		deferredContext[cmd]->DrawInstanced(verticesPerInstance, instances, startVertex, startInstance);
	}
	void Graphics_dx11::DrawIndexedInstanced(ui32 indicesPerInstance, ui32 instances, ui32 startIndex, ui32 startVertex, ui32 startInstance, CommandList cmd)
	{
		viewports[cmd].Bind(deferredContext[cmd].Get());
		deferredContext[cmd]->DrawIndexedInstanced(indicesPerInstance, instances, startIndex, startVertex, startInstance);
	}


	void Graphics_dx11::_ResetState(CommandList cmd)
	{
		deferredContext[cmd]->ClearState();
	}

	///////////////////////////////VERTEX BUFFER///////////////////////////////////
	void UpdateBuffer(Graphics_dx11& dx11, ID3D11Resource* res, SV_GFX_USAGE usage, ui32 bufferSize, void* data, ui32 size, CommandList cmd)
	{
		SV_ASSERT(usage != SV_GFX_USAGE_STATIC && bufferSize >= size);

		if (usage == SV_GFX_USAGE_DYNAMIC) {
			D3D11_MAPPED_SUBRESOURCE map;
			dx11.deferredContext[cmd]->Map(res, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &map);

			memcpy(map.pData, data, (size == 0u) ? bufferSize : size);

			dx11.deferredContext[cmd]->Unmap(res, 0u);
		}
		else {
			dx11.deferredContext[cmd]->UpdateSubresource(res, 0, nullptr, data, 0, 0);
		}
	}

	bool Graphics_dx11::_CreateVertexBuffer(void* data, VertexBuffer& vb)
	{
		VertexBuffer_dx11& buffer = ToInternal(vb);

		D3D11_BUFFER_DESC desc;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.ByteWidth = buffer.GetSize();
		desc.CPUAccessFlags = ParseCPUAccess(buffer.GetCPUAccess());
		desc.MiscFlags = 0u;
		desc.StructureByteStride = 0u;
		desc.Usage = ParseUsage(buffer.GetUsage());

		if (data) {
			D3D11_SUBRESOURCE_DATA sdata;
			sdata.pSysMem = data;
			dxCheck(device->CreateBuffer(&desc, &sdata, buffer.m_Buffer.GetAddressOf()));
		}
		else {
			dxCheck(device->CreateBuffer(&desc, nullptr, buffer.m_Buffer.GetAddressOf()));
		}

		return true;
	}
	void Graphics_dx11::_ReleaseVertexBuffer(VertexBuffer& vb)
	{
		VertexBuffer_dx11& buffer = ToInternal(vb);
		buffer.m_Buffer.Reset();
	}
	void Graphics_dx11::_UpdateVertexBuffer(void* data, ui32 size, VertexBuffer& vb, CommandList cmd)
	{
		VertexBuffer_dx11& buffer = ToInternal(vb);
		UpdateBuffer(*this, buffer.m_Buffer.Get(), buffer.GetUsage(), buffer.GetSize(), data, size, cmd);
	}
	void Graphics_dx11::_BindVertexBuffer(ui32 slot, ui32 stride, ui32 offset, VertexBuffer& vb, CommandList cmd)
	{
		VertexBuffer_dx11& buffer = ToInternal(vb);
		deferredContext[cmd]->IASetVertexBuffers(slot, 1, buffer.m_Buffer.GetAddressOf(), &stride, &offset);
	}
	void Graphics_dx11::_BindVertexBuffers(ui32 slot, ui32 count, const ui32* strides, const ui32* offsets, VertexBuffer** vbs, CommandList cmd)
	{
		ID3D11Buffer* buffers[SV_GFX_VERTEX_BUFFER_COUNT];

		for (ui32 i = 0; i < count; ++i) {
			VertexBuffer_dx11& buffer = ToInternal(*vbs[i]);
			buffers[i] = buffer.m_Buffer.Get();
		}

		deferredContext[cmd]->IASetVertexBuffers(slot, count, buffers, strides, offsets);
	}
	void Graphics_dx11::_UnbindVertexBuffer(ui32 slot, CommandList cmd)
	{
		ID3D11Buffer* null[1] = { nullptr };
		const UINT stride = 1u;
		const UINT offset = 0u;
		deferredContext[cmd]->IASetVertexBuffers(slot, 1, null, &stride, &offset);
	}
	void Graphics_dx11::_UnbindVertexBuffers(CommandList cmd)
	{
		ID3D11Buffer* buffers[SV_GFX_VERTEX_BUFFER_COUNT];
		svZeroMemory(buffers, sizeof(ID3D11Buffer*) * SV_GFX_VERTEX_BUFFER_COUNT);
		deferredContext[cmd]->IASetVertexBuffers(0, SV_GFX_VERTEX_BUFFER_COUNT, buffers, (const UINT*)buffers, (const UINT*)buffers);
	}
	///////////////////////////////INDEX BUFFER///////////////////////////////////
	bool Graphics_dx11::_CreateIndexBuffer(void* data, IndexBuffer& ib)
	{
		IndexBuffer_dx11& buffer = ToInternal(ib);

		D3D11_BUFFER_DESC desc;
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		desc.ByteWidth = buffer.GetSize();
		desc.CPUAccessFlags = ParseCPUAccess(buffer.GetCPUAccess());
		desc.MiscFlags = 0u;
		desc.StructureByteStride = 0u;
		desc.Usage = ParseUsage(buffer.GetUsage());

		if (data) {
			D3D11_SUBRESOURCE_DATA sdata;
			sdata.pSysMem = data;
			dxCheck(device->CreateBuffer(&desc, &sdata, buffer.m_Buffer.GetAddressOf()));
		}
		else {
			dxCheck(device->CreateBuffer(&desc, nullptr, buffer.m_Buffer.GetAddressOf()));
		}

		return true;
	}
	void Graphics_dx11::_ReleaseIndexBuffer(IndexBuffer& ib)
	{
		IndexBuffer_dx11& buffer = ToInternal(ib);
		buffer.m_Buffer.Reset();
	}
	void Graphics_dx11::_UpdateIndexBuffer(void* data, ui32 size, IndexBuffer& ib, CommandList cmd)
	{
		IndexBuffer_dx11& buffer = ToInternal(ib);
		UpdateBuffer(*this, buffer.m_Buffer.Get(), buffer.GetUsage(), buffer.GetSize(), data, size, cmd);
	}
	void Graphics_dx11::_BindIndexBuffer(SV_GFX_FORMAT format, ui32 offset, IndexBuffer& ib, CommandList cmd) 
	{
		IndexBuffer_dx11& buffer = ToInternal(ib);
		deferredContext[cmd]->IASetIndexBuffer(buffer.m_Buffer.Get(), ParseFormat(format), offset);
	}
	void Graphics_dx11::_UnbindIndexBuffer(CommandList cmd)
	{
		deferredContext[cmd]->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0u);
	}

	///////////////////////////////CONSTANT BUFFER///////////////////////////////////
	bool Graphics_dx11::_CreateConstantBuffer(void* data, ConstantBuffer& cb)
	{
		ConstantBuffer_dx11& buffer = ToInternal(cb);

		D3D11_BUFFER_DESC desc;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.ByteWidth = buffer.GetSize();
		desc.CPUAccessFlags = ParseCPUAccess(buffer.GetCPUAccess());
		desc.MiscFlags = 0u;
		desc.StructureByteStride = 0u;
		desc.Usage = ParseUsage(buffer.GetUsage());

		if (data) {
			D3D11_SUBRESOURCE_DATA sdata;
			sdata.pSysMem = data;
			dxCheck(device->CreateBuffer(&desc, &sdata, buffer.m_Buffer.GetAddressOf()));
		}
		else {
			dxCheck(device->CreateBuffer(&desc, nullptr, buffer.m_Buffer.GetAddressOf()));
		}

		return true;
	}
	void Graphics_dx11::_ReleaseConstantBuffer(ConstantBuffer& cb)
	{
		ConstantBuffer_dx11& buffer = ToInternal(cb);
		buffer.m_Buffer.Reset();
	}
	void Graphics_dx11::_UpdateConstantBuffer(void* data, ui32 size, ConstantBuffer& cb, CommandList cmd)
	{
		ConstantBuffer_dx11& buffer = ToInternal(cb);
		UpdateBuffer(*this, buffer.m_Buffer.Get(), buffer.GetUsage(), buffer.GetSize(), data, size, cmd);
	}
	void Graphics_dx11::_BindConstantBuffer(ui32 slot, SV_GFX_SHADER_TYPE type, ConstantBuffer& cb, CommandList cmd)
	{
		ConstantBuffer_dx11& buffer = ToInternal(cb);
		switch (type)
		{
		case SV_GFX_SHADER_TYPE_VERTEX:
			deferredContext[cmd]->VSSetConstantBuffers(slot, 1, buffer.m_Buffer.GetAddressOf());
			break;
		case SV_GFX_SHADER_TYPE_PIXEL:
			deferredContext[cmd]->PSSetConstantBuffers(slot, 1, buffer.m_Buffer.GetAddressOf());
			break;
		case SV_GFX_SHADER_TYPE_GEOMETRY:
			deferredContext[cmd]->GSSetConstantBuffers(slot, 1, buffer.m_Buffer.GetAddressOf());
			break;
		}
	}
	void Graphics_dx11::_BindConstantBuffers(ui32 slot, SV_GFX_SHADER_TYPE type, ui32 count, ConstantBuffer** cbs, CommandList cmd)
	{
		ID3D11Buffer* buffers[SV_GFX_CONSTANT_BUFFER_COUNT];
		for (ui32 i = 0; i < count; ++i) {
			ConstantBuffer_dx11& buffer = ToInternal(*cbs[i]);
			buffers[i] = buffer.m_Buffer.Get();
		}

		switch (type)
		{
		case SV_GFX_SHADER_TYPE_VERTEX:
			deferredContext[cmd]->VSSetConstantBuffers(slot, count, buffers);
			break;
		case SV_GFX_SHADER_TYPE_PIXEL:
			deferredContext[cmd]->PSSetConstantBuffers(slot, count, buffers);
			break;
		case SV_GFX_SHADER_TYPE_GEOMETRY:
			deferredContext[cmd]->GSSetConstantBuffers(slot, count, buffers);
			break;
		}
	}
	void Graphics_dx11::_UnbindConstantBuffer(ui32 slot, SV_GFX_SHADER_TYPE type, CommandList cmd)
	{
		ID3D11Buffer* null[1] = { nullptr };
		switch (type)
		{
		case SV_GFX_SHADER_TYPE_VERTEX:
			deferredContext[cmd]->VSSetConstantBuffers(slot, 1, null);
			break;
		case SV_GFX_SHADER_TYPE_PIXEL:
			deferredContext[cmd]->PSSetConstantBuffers(slot, 1, null);
			break;
		case SV_GFX_SHADER_TYPE_GEOMETRY:
			deferredContext[cmd]->GSSetConstantBuffers(slot, 1, null);
			break;
		}
	}
	void Graphics_dx11::_UnbindConstantBuffers(SV_GFX_SHADER_TYPE type, CommandList cmd)
	{
		ID3D11Buffer* buffers[SV_GFX_CONSTANT_BUFFER_COUNT];
		svZeroMemory(buffers, sizeof(ID3D11Buffer) * SV_GFX_CONSTANT_BUFFER_COUNT);

		switch (type)
		{
		case SV_GFX_SHADER_TYPE_VERTEX:
			deferredContext[cmd]->VSSetConstantBuffers(0u, SV_GFX_CONSTANT_BUFFER_COUNT, buffers);
			break;
		case SV_GFX_SHADER_TYPE_PIXEL:
			deferredContext[cmd]->PSSetConstantBuffers(0u, SV_GFX_CONSTANT_BUFFER_COUNT, buffers);
			break;
		case SV_GFX_SHADER_TYPE_GEOMETRY:
			deferredContext[cmd]->GSSetConstantBuffers(0u, SV_GFX_CONSTANT_BUFFER_COUNT, buffers);
			break;
		}
	}
	void Graphics_dx11::_UnbindConstantBuffers(CommandList cmd)
	{
		_UnbindConstantBuffers(SV_GFX_SHADER_TYPE_VERTEX, cmd);
		_UnbindConstantBuffers(SV_GFX_SHADER_TYPE_PIXEL, cmd);
		_UnbindConstantBuffers(SV_GFX_SHADER_TYPE_GEOMETRY, cmd);
	}

	///////////////////////////////FRAME BUFFER///////////////////////////////////
	bool Graphics_dx11::_CreateFrameBuffer(FrameBuffer& fb)
	{
		FrameBuffer_dx11& buffer = ToInternal(fb);
		// Render target view desc
		{
			D3D11_RENDER_TARGET_VIEW_DESC desc;
			desc.Format = ParseFormat(buffer.GetFormat());
			desc.Texture2D.MipSlice = 0u;
			desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

			UINT bindFlags = D3D11_BIND_RENDER_TARGET;
			if (buffer.HasTextureUsage()) bindFlags |= D3D11_BIND_SHADER_RESOURCE;

			// Resource desc
			D3D11_TEXTURE2D_DESC resDesc;
			resDesc.ArraySize = 1u;
			resDesc.BindFlags = bindFlags;
			resDesc.CPUAccessFlags = 0u;
			resDesc.Format = ParseFormat(buffer.GetFormat());
			resDesc.Width = buffer.GetWidth();
			resDesc.Height = buffer.GetHeight();
			resDesc.MipLevels = 1u;
			resDesc.MiscFlags = 0u;
			resDesc.SampleDesc.Count = 1u;
			resDesc.SampleDesc.Quality = 0u;
			resDesc.Usage = D3D11_USAGE_DEFAULT;

			dxCheck(device->CreateTexture2D(&resDesc, nullptr, &buffer.m_Resource));
			dxCheck(device->CreateRenderTargetView(buffer.m_Resource.Get(), &desc, &buffer.m_RenderTargetView));
		}

		// Shader resouce
		if (buffer.HasTextureUsage()) {
			D3D11_SHADER_RESOURCE_VIEW_DESC desc;
			svZeroMemory(&desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
			desc.Format = ParseFormat(buffer.GetFormat());
			desc.Texture2D.MipLevels = 1u;
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

			dxCheck(device->CreateShaderResourceView(buffer.m_Resource.Get(), &desc, &buffer.m_ShaderResouceView));
		}

		return true;
	}
	void Graphics_dx11::_ReleaseFrameBuffer(FrameBuffer& fb)
	{
		FrameBuffer_dx11& buffer = ToInternal(fb);
		buffer.m_RenderTargetView.Reset();
		buffer.m_Resource.Reset();
		buffer.m_ShaderResouceView.Reset();
	}
	void Graphics_dx11::_ClearFrameBuffer(SV::Color4f c, FrameBuffer& fb, CommandList cmd)
	{
		const FLOAT color[4] = {
			c.x, c.y, c.z, c.w
		};
		FrameBuffer_dx11& buffer = ToInternal(fb);
		deferredContext[cmd]->ClearRenderTargetView(buffer.m_RenderTargetView.Get(), color);
	}
	void Graphics_dx11::_BindFrameBuffer(FrameBuffer& fb, Texture* dsv, CommandList cmd)
	{
		FrameBuffer_dx11& buffer = ToInternal(fb);
		if (dsv) {
			Texture_dx11& DSV = ToInternal(*dsv);
			deferredContext[cmd]->OMSetRenderTargets(1, buffer.m_RenderTargetView.GetAddressOf(), DSV.m_DepthStencilView.Get());
		}
		else deferredContext[cmd]->OMSetRenderTargets(1, buffer.m_RenderTargetView.GetAddressOf(), nullptr);
	}
	void Graphics_dx11::_BindFrameBuffers(ui32 count, FrameBuffer** fbs, Texture* dsv, CommandList cmd)
	{
		ID3D11RenderTargetView* renderTargets[SV_GFX_RENDER_TARGET_VIEW_COUNT];
		for (ui32 i = 0; i < SV_GFX_RENDER_TARGET_VIEW_COUNT; ++i) {
			FrameBuffer_dx11& buffer = ToInternal(*fbs[i]);
			renderTargets[i] = buffer.m_RenderTargetView.Get();
		}

		if (dsv) {
			Texture_dx11& DSV = ToInternal(*dsv);
			deferredContext[cmd]->OMSetRenderTargets(count, renderTargets, DSV.m_DepthStencilView.Get());
		}
		else deferredContext[cmd]->OMSetRenderTargets(count, renderTargets, nullptr);
	}
	void Graphics_dx11::_BindFrameBufferAsTexture(ui32 slot, SV_GFX_SHADER_TYPE type, FrameBuffer& fb, CommandList cmd)
	{
		FrameBuffer_dx11& buffer = ToInternal(fb);
		switch (type) {
		case SV_GFX_SHADER_TYPE_VERTEX:
			deferredContext[cmd]->VSSetShaderResources(slot, 1, buffer.m_ShaderResouceView.GetAddressOf());
			break;
		case SV_GFX_SHADER_TYPE_PIXEL:
			deferredContext[cmd]->PSSetShaderResources(slot, 1, buffer.m_ShaderResouceView.GetAddressOf());
			break;
		case SV_GFX_SHADER_TYPE_GEOMETRY:
			deferredContext[cmd]->GSSetShaderResources(slot, 1, buffer.m_ShaderResouceView.GetAddressOf());
			break;
		}
	}
	void Graphics_dx11::_UnbindFrameBuffers(CommandList cmd)
	{
		ID3D11RenderTargetView* nullRTV = nullptr;
		deferredContext[cmd]->OMSetRenderTargets(1, &nullRTV, nullptr);
	}

	/////////////////////////////////SHADER//////////////////////////////////////////
	inline ID3D11VertexShader* ToVS(void* ptr)
	{
		return reinterpret_cast<ID3D11VertexShader*>(ptr);
	}
	inline ID3D11PixelShader* ToPS(void* ptr)
	{
		return reinterpret_cast<ID3D11PixelShader*>(ptr);
	}
	inline ID3D11GeometryShader* ToGS(void* ptr)
	{
		return reinterpret_cast<ID3D11GeometryShader*>(ptr);
	}

	bool Graphics_dx11::_CreateShader(const char* filePath, Shader& s)
	{
		std::wstring wFilePath = SV::Utils::ToWString(filePath);

		Shader_dx11& shader = ToInternal(s);

		// Read File
		dxCheck(D3DReadFileToBlob(wFilePath.c_str(), &shader.m_Blob));

		// Create Shader
		switch (shader.GetType())
		{
		case SV_GFX_SHADER_TYPE_VERTEX:
			dxCheck(device->CreateVertexShader(shader.m_Blob->GetBufferPointer(), shader.m_Blob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11VertexShader * *>(&shader.m_Shader)));
			break;
		case SV_GFX_SHADER_TYPE_PIXEL:
			dxCheck(device->CreatePixelShader(shader.m_Blob->GetBufferPointer(), shader.m_Blob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11PixelShader * *>(&shader.m_Shader)));
			break;
		case SV_GFX_SHADER_TYPE_GEOMETRY:
			dxCheck(device->CreateGeometryShader(shader.m_Blob->GetBufferPointer(), shader.m_Blob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11GeometryShader * *>(&shader.m_Shader)));
			break;
		case SV_GFX_SHADER_TYPE_UNDEFINED:
		default:
			SV::LogE("Invalid Shader Type '%u'", shader.GetType());
			return false;
		}

		if (shader.GetType() != SV_GFX_SHADER_TYPE_VERTEX) shader.m_Blob.Reset();

		return true;
	}
	void Graphics_dx11::_ReleaseShader(Shader& s)
	{
		Shader_dx11& shader = ToInternal(s);

		if (shader.m_Shader) {
			switch (shader.GetType()) {
			case SV_GFX_SHADER_TYPE_VERTEX:
			{
				auto vs = ToVS(shader.m_Shader);
				vs->Release();
				break;
			}
			case SV_GFX_SHADER_TYPE_PIXEL:
			{
				auto ps = ToPS(shader.m_Shader);
				ps->Release();
				break;
			}
			case SV_GFX_SHADER_TYPE_GEOMETRY:
			{
				auto gs = ToGS(shader.m_Shader);
				gs->Release();
				break;
			}
			}
		}
		shader.m_Blob.Reset();
	}
	void Graphics_dx11::_BindShader(Shader& s, CommandList cmd)
	{
		Shader_dx11& shader = ToInternal(s);

		switch (shader.GetType()) {
		case SV_GFX_SHADER_TYPE_VERTEX:
		{
			auto vs = ToVS(shader.m_Shader);

			deferredContext[cmd]->VSSetShader(vs, nullptr, 0u);

			break;
		}
		case SV_GFX_SHADER_TYPE_PIXEL:
		{
			auto ps = ToPS(shader.m_Shader);

			deferredContext[cmd]->PSSetShader(ps, nullptr, 0u);

			break;
		}
		case SV_GFX_SHADER_TYPE_GEOMETRY:
		{
			auto gs = ToGS(shader.m_Shader);

			deferredContext[cmd]->GSSetShader(gs, nullptr, 0u);

			break;
		}
		}
	}
	void Graphics_dx11::_UnbindShader(SV_GFX_SHADER_TYPE type, CommandList cmd)
	{
		switch (type) {
		case SV_GFX_SHADER_TYPE_VERTEX:
			deferredContext[cmd]->VSSetShader(nullptr, nullptr, 0u);
			break;
		case SV_GFX_SHADER_TYPE_PIXEL:
			deferredContext[cmd]->PSSetShader(nullptr, nullptr, 0u);
			break;
		case SV_GFX_SHADER_TYPE_GEOMETRY:
			deferredContext[cmd]->GSSetShader(nullptr, nullptr, 0u);
			break;
		}
	}

	/////////////////////////////////INPUT LAYOUT//////////////////////////////////////////
	bool Graphics_dx11::_CreateInputLayout(const SV_GFX_INPUT_ELEMENT_DESC* d, ui32 count, const Shader& vs, InputLayout& il)
	{
		SV_ASSERT(count < 32);

#ifdef SV_DEBUG
		if (vs.IsValid() && vs->GetType() != SV_GFX_SHADER_TYPE_VERTEX) {
			SV::LogE("To create an input layout must have a vertex shader");
			return false;
		}
#endif

		D3D11_INPUT_ELEMENT_DESC desc[32];

		for (ui32 i = 0; i < count; ++i) {
			desc[i].AlignedByteOffset = d[i].AlignedByteOffset;
			desc[i].Format = ParseFormat(d[i].Format);
			desc[i].InputSlot = d[i].InputSlot;
			desc[i].InputSlotClass = (d[i].InstancedData) ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;
			desc[i].InstanceDataStepRate = d[i].InstanceDataStepRate;
			desc[i].SemanticIndex = d[i].SemanticIndex;
			desc[i].SemanticName = d[i].SemanticName;
		}

		InputLayout_dx11& inputLayout = ToInternal(il);
		Shader_dx11& shader = ToInternal(const_cast<Shader&>(vs));

		dxCheck(device->CreateInputLayout(desc, count, shader.m_Blob->GetBufferPointer(), shader.m_Blob->GetBufferSize(), &inputLayout.m_InputLayout));

		return true;
	}
	void Graphics_dx11::_ReleaseInputLayout(InputLayout& il)
	{
		InputLayout_dx11& inputLayout = ToInternal(il);
		inputLayout.m_InputLayout.Reset();
	}
	void Graphics_dx11::_BindInputLayout(InputLayout& il, CommandList cmd)
	{
		InputLayout_dx11& inputLayout = ToInternal(il);
		deferredContext[cmd]->IASetInputLayout(inputLayout.m_InputLayout.Get());
	}
	void Graphics_dx11::_UnbindInputLayout(CommandList cmd)
	{
		deferredContext[cmd]->IASetInputLayout(nullptr);
	}

	/////////////////////////////////TEXTURE//////////////////////////////////////////
	bool Graphics_dx11::_CreateTexture(void* data, Texture& t)
	{
		Texture_dx11& texture = ToInternal(t);

		// Resource
		{
			D3D11_TEXTURE2D_DESC desc = {};
			desc.ArraySize = 1u;
			desc.BindFlags = texture.IsDepthStencilView() ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = ParseCPUAccess(texture.GetCPUAccess());
			desc.Format = ParseFormat(texture.GetFormat());
			desc.Width = texture.GetWidth();
			desc.Height = texture.GetHeight();
			desc.MipLevels = 1u;
			desc.MiscFlags = 0u;
			desc.SampleDesc.Count = 1u;
			desc.SampleDesc.Quality = 0u;
			desc.Usage = ParseUsage(texture.GetUsage());

			if (data) {
				D3D11_SUBRESOURCE_DATA sdata;
				sdata.pSysMem = data;
				sdata.SysMemPitch = texture.GetWidth() * SV::GetFormatSize(texture.GetFormat());

				dxCheck(device->CreateTexture2D(&desc, &sdata, &texture.m_Texture));
			}
			else {
				dxCheck(device->CreateTexture2D(&desc, nullptr, &texture.m_Texture));
			}
		}

		// Depth stencil view
		if (texture.IsDepthStencilView()) {
			D3D11_DEPTH_STENCIL_VIEW_DESC desc;
			desc.Flags = 0u;
			desc.Format = ParseFormat(texture.GetFormat());
			desc.Texture2D.MipSlice = 0u;
			desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

			dxCheck(device->CreateDepthStencilView(texture.m_Texture.Get(), &desc, &texture.m_DepthStencilView));
		}
		else {
			// Shader resource view
			D3D11_SHADER_RESOURCE_VIEW_DESC desc;
			svZeroMemory(&desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
			desc.Format = ParseFormat(texture.GetFormat());
			desc.Texture2D.MipLevels = 1u;
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

			dxCheck(device->CreateShaderResourceView(texture.m_Texture.Get(), &desc, &texture.m_ShaderResouceView));
		}

		return true;
	}
	void Graphics_dx11::_ReleaseTexture(Texture& t)
	{
		Texture_dx11& texture = ToInternal(t);
		texture.m_Texture.Reset();
		texture.m_ShaderResouceView.Reset();
		texture.m_DepthStencilView.Reset();
	}
	void Graphics_dx11::_UpdateTexture(void* data, ui32 size, Texture& t, CommandList cmd)
	{
		Texture_dx11& texture = ToInternal(t);
		UpdateBuffer(*this, texture.m_Texture.Get(), texture.GetUsage(), texture.GetSize(), data, size, cmd);
	}
	void Graphics_dx11::_ClearTextureDSV(float depth, ui8 stencil, Texture& t, CommandList cmd)
	{
		Texture_dx11& texture = ToInternal(t);
		deferredContext[cmd]->ClearDepthStencilView(texture.m_DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, stencil);
	}
	void Graphics_dx11::_BindTexture(ui32 slot, SV_GFX_SHADER_TYPE type, Texture& t, CommandList cmd)
	{
		Texture_dx11& texture = ToInternal(t);
		switch (type)
		{
		case SV_GFX_SHADER_TYPE_VERTEX:
			deferredContext[cmd]->VSSetShaderResources(slot, 1u, texture.m_ShaderResouceView.GetAddressOf());
			break;
		case SV_GFX_SHADER_TYPE_PIXEL:
			deferredContext[cmd]->PSSetShaderResources(slot, 1u, texture.m_ShaderResouceView.GetAddressOf());
			break;
		case SV_GFX_SHADER_TYPE_GEOMETRY:
			deferredContext[cmd]->GSSetShaderResources(slot, 1u, texture.m_ShaderResouceView.GetAddressOf());
			break;
		}
	}
	void Graphics_dx11::_BindTextures(ui32 slot, SV_GFX_SHADER_TYPE type, ui32 count, Texture** ts, CommandList cmd)
	{
		ID3D11ShaderResourceView* textures[SV_GFX_TEXTURE_COUNT];

		for (ui32 i = 0; i < count; ++i) {
			Texture_dx11& texture = ToInternal(*ts[i]);
			textures[i] = texture.m_ShaderResouceView.Get();
		}

		switch (type)
		{
		case SV_GFX_SHADER_TYPE_VERTEX:
			deferredContext[cmd]->VSSetShaderResources(slot, count, textures);
			break;
		case SV_GFX_SHADER_TYPE_PIXEL:
			deferredContext[cmd]->PSSetShaderResources(slot, count, textures);
			break;
		case SV_GFX_SHADER_TYPE_GEOMETRY:
			deferredContext[cmd]->GSSetShaderResources(slot, count, textures);
			break;
		}
	}
	void Graphics_dx11::_UnbindTexture(ui32 slot, SV_GFX_SHADER_TYPE type, CommandList cmd)
	{
		ID3D11ShaderResourceView* null[1] = { nullptr };
		switch (type)
		{
		case SV_GFX_SHADER_TYPE_VERTEX:
			deferredContext[cmd]->VSSetShaderResources(slot, 1u, null);
			break;
		case SV_GFX_SHADER_TYPE_PIXEL:
			deferredContext[cmd]->PSSetShaderResources(slot, 1u, null);
			break;
		case SV_GFX_SHADER_TYPE_GEOMETRY:
			deferredContext[cmd]->GSSetShaderResources(slot, 1u, null);
			break;
		}
	}
	void Graphics_dx11::_UnbindTextures(SV_GFX_SHADER_TYPE type, CommandList cmd)
	{
		ID3D11ShaderResourceView* textures[SV_GFX_TEXTURE_COUNT];
		svZeroMemory(textures, sizeof(ID3D11ShaderResourceView) * SV_GFX_TEXTURE_COUNT);

		switch (type)
		{
		case SV_GFX_SHADER_TYPE_VERTEX:
			deferredContext[cmd]->VSSetShaderResources(0u, SV_GFX_TEXTURE_COUNT, textures);
			break;
		case SV_GFX_SHADER_TYPE_PIXEL:
			deferredContext[cmd]->PSSetShaderResources(0u, SV_GFX_TEXTURE_COUNT, textures);
			break;
		case SV_GFX_SHADER_TYPE_GEOMETRY:
			deferredContext[cmd]->GSSetShaderResources(0u, SV_GFX_TEXTURE_COUNT, textures);
			break;
		}
	}
	void Graphics_dx11::_UnbindTextures(CommandList cmd)
	{
		_UnbindTextures(SV_GFX_SHADER_TYPE_VERTEX, cmd);
		_UnbindTextures(SV_GFX_SHADER_TYPE_PIXEL, cmd);
		_UnbindTextures(SV_GFX_SHADER_TYPE_GEOMETRY, cmd);
	}

	/////////////////////////////////SAMPLER//////////////////////////////////////////
	bool Graphics_dx11::_CreateSampler(SV_GFX_TEXTURE_ADDRESS_MODE addressMode, SV_GFX_TEXTURE_FILTER filter, Sampler& s)
	{
		D3D11_SAMPLER_DESC desc;
		svZeroMemory(&desc, sizeof(D3D11_SAMPLER_DESC));
		desc.AddressU = ParseAddressMode(addressMode);
		desc.AddressV = ParseAddressMode(addressMode);
		desc.AddressW = ParseAddressMode(addressMode);
		desc.Filter = ParseTextureFilter(filter);

		Sampler_dx11& sampler = ToInternal(s);

		dxCheck(device->CreateSamplerState(&desc, &sampler.m_SamplerState));

		return true;
	}
	void Graphics_dx11::_ReleaseSampler(Sampler& s)
	{
		Sampler_dx11& sampler = ToInternal(s);
		sampler.m_SamplerState.Reset();
	}
	void Graphics_dx11::_BindSampler(ui32 slot, SV_GFX_SHADER_TYPE type, Sampler& s, CommandList cmd)
	{
		Sampler_dx11& sampler = ToInternal(s);
		switch (type) {
		case SV_GFX_SHADER_TYPE_VERTEX:
			deferredContext[cmd]->VSSetSamplers(slot, 1u, sampler.m_SamplerState.GetAddressOf());
			break;
		case SV_GFX_SHADER_TYPE_PIXEL:
			deferredContext[cmd]->PSSetSamplers(slot, 1u, sampler.m_SamplerState.GetAddressOf());
			break;
		case SV_GFX_SHADER_TYPE_GEOMETRY:
			deferredContext[cmd]->GSSetSamplers(slot, 1u, sampler.m_SamplerState.GetAddressOf());
			break;
		}
	}
	void Graphics_dx11::_BindSamplers(ui32 slot, SV_GFX_SHADER_TYPE type, ui32 count, Sampler** ss, CommandList cmd)
	{
		ID3D11SamplerState* samplers[SV_GFX_SAMPLER_COUNT];

		for (ui32 i = 0; i < count; ++i) {
			Sampler_dx11& sampler = ToInternal(*ss[i]);
			samplers[i] = sampler.m_SamplerState.Get();
		}

		switch (type) {
		case SV_GFX_SHADER_TYPE_VERTEX:
			deferredContext[cmd]->VSSetSamplers(slot, count, samplers);
			break;
		case SV_GFX_SHADER_TYPE_PIXEL:
			deferredContext[cmd]->PSSetSamplers(slot, count, samplers);
			break;
		case SV_GFX_SHADER_TYPE_GEOMETRY:
			deferredContext[cmd]->GSSetSamplers(slot, count, samplers);
			break;
		}
	}
	void Graphics_dx11::_UnbindSampler(ui32 slot, SV_GFX_SHADER_TYPE type, CommandList cmd)
	{
		ID3D11SamplerState* null[1] = { nullptr };
		switch (type) {
		case SV_GFX_SHADER_TYPE_VERTEX:
			deferredContext[cmd]->VSSetSamplers(slot, 1u, null);
			break;
		case SV_GFX_SHADER_TYPE_PIXEL:
			deferredContext[cmd]->PSSetSamplers(slot, 1u, null);
			break;
		case SV_GFX_SHADER_TYPE_GEOMETRY:
			deferredContext[cmd]->GSSetSamplers(slot, 1u, null);
			break;
		}
	}
	void Graphics_dx11::_UnbindSamplers(SV_GFX_SHADER_TYPE type, CommandList cmd)
	{
		ID3D11SamplerState* samplers[SV_GFX_SAMPLER_COUNT];
		svZeroMemory(samplers, sizeof(ID3D11SamplerState*) * SV_GFX_SAMPLER_COUNT);

		switch (type) {
		case SV_GFX_SHADER_TYPE_VERTEX:
			deferredContext[cmd]->VSSetSamplers(0u, SV_GFX_SAMPLER_COUNT, samplers);
			break;
		case SV_GFX_SHADER_TYPE_PIXEL:
			deferredContext[cmd]->PSSetSamplers(0u, SV_GFX_SAMPLER_COUNT, samplers);
			break;
		case SV_GFX_SHADER_TYPE_GEOMETRY:
			deferredContext[cmd]->GSSetSamplers(0u, SV_GFX_SAMPLER_COUNT, samplers);
			break;
		}
	}
	void Graphics_dx11::_UnbindSamplers(CommandList cmd)
	{
		_UnbindSamplers(SV_GFX_SHADER_TYPE_VERTEX, cmd);
		_UnbindSamplers(SV_GFX_SHADER_TYPE_PIXEL, cmd);
		_UnbindSamplers(SV_GFX_SHADER_TYPE_GEOMETRY, cmd);
	}

	/////////////////////////////////BLEND STATE///////////////////////////////////////////////
	bool Graphics_dx11::_CreateBlendState(BlendState& bs)
	{
		D3D11_BLEND_DESC desc;
		const SV_GFX_BLEND_STATE_DESC& d = bs->GetDesc();

		desc.AlphaToCoverageEnable = FALSE;
		desc.IndependentBlendEnable = d.independentRenderTarget ? TRUE : FALSE;

		for (ui8 i = 0; i < 8; ++i) {
			auto& rt0 = desc.RenderTarget[i];
			auto& rt1 = d.renderTargetDesc[i];

			rt0.BlendEnable = rt1.enabled ? TRUE : FALSE;
			rt0.SrcBlend = ParseBlendOption(rt1.src);
			rt0.SrcBlendAlpha = ParseBlendOption(rt1.srcAlpha);
			rt0.DestBlend = ParseBlendOption(rt1.dest);
			rt0.DestBlendAlpha = ParseBlendOption(rt1.destAlpha);
			rt0.BlendOp = ParseBlendOperation(rt1.op);
			rt0.BlendOpAlpha = ParseBlendOperation(rt1.opAlpha);
			rt0.RenderTargetWriteMask = rt1.writeMask;
		}

		BlendState_dx11& blendState = ToInternal(bs);
		dxCheck(device->CreateBlendState(&desc, &blendState.m_BlendState));

		return true;
	}
	void Graphics_dx11::_ReleaseBlendState(BlendState& bs)
	{
		BlendState_dx11& blendState = ToInternal(bs);
		blendState.m_BlendState.Reset();
	}
	void Graphics_dx11::_BindBlendState(ui32 sampleMask, const float* blendFactors, BlendState& bs, CommandList cmd)
	{
		BlendState_dx11& blendState = ToInternal(bs);
		deferredContext[cmd]->OMSetBlendState(blendState.m_BlendState.Get(), blendFactors, sampleMask);
	}
	void Graphics_dx11::_UnbindBlendState(CommandList cmd)
	{
		deferredContext[cmd]->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	}

	/////////////////////////////////DEPTHSTENCIL STATE///////////////////////////////////////////////
	bool Graphics_dx11::_CreateDepthStencilState(DepthStencilState& dss)
	{
		D3D11_DEPTH_STENCIL_DESC desc;
		const SV_GFX_DEPTH_STENCIL_STATE_DESC& d = dss->GetDesc();

		desc.DepthEnable = d.depthEnable ? TRUE : FALSE;
		desc.DepthFunc = ParseComparisonFn(d.depthFunction);
		desc.DepthWriteMask = d.depthWriteMask ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
		desc.StencilEnable = d.stencilEnable ? TRUE : FALSE;
		desc.StencilReadMask = d.stencilReadMask;
		desc.StencilWriteMask = d.stencilWriteMask;
		desc.FrontFace.StencilDepthFailOp = ParseStencilOp(d.frontFaceOp.stencilDepthFailOp);
		desc.FrontFace.StencilFailOp = ParseStencilOp(d.frontFaceOp.stencilFailOp);
		desc.FrontFace.StencilPassOp = ParseStencilOp(d.frontFaceOp.stencilPassOp);
		desc.FrontFace.StencilFunc = ParseComparisonFn(d.frontFaceOp.stencilFunction);
		desc.BackFace.StencilDepthFailOp = ParseStencilOp(d.backFaceOp.stencilDepthFailOp);
		desc.BackFace.StencilFailOp = ParseStencilOp(d.backFaceOp.stencilFailOp);
		desc.BackFace.StencilPassOp = ParseStencilOp(d.backFaceOp.stencilPassOp);
		desc.BackFace.StencilFunc = ParseComparisonFn(d.backFaceOp.stencilFunction);

		DepthStencilState_dx11& DSS = ToInternal(dss);

		dxCheck(device->CreateDepthStencilState(&desc, &DSS.m_DepthStencilState));

		return true;
	}
	void Graphics_dx11::_ReleaseDepthStencilState(DepthStencilState& dss)
	{
		DepthStencilState_dx11& DSS = ToInternal(dss);
		DSS.m_DepthStencilState.Reset();
	}
	void Graphics_dx11::_BindDepthStencilState(ui32 stencilRef, DepthStencilState& dss, CommandList cmd)
	{
		DepthStencilState_dx11& DSS = ToInternal(dss);
		deferredContext[cmd]->OMSetDepthStencilState(DSS.m_DepthStencilState.Get(), stencilRef);
	}
	void Graphics_dx11::_UnbindDepthStencilState(CommandList cmd)
	{
		deferredContext[cmd]->OMSetDepthStencilState(nullptr, 0u);
	}

}