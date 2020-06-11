#include "core.h"

#include "Graphics_dx11.h"
#include "Window.h"
#include "Engine.h"

#define dxAssert(x) if((x) != 0) SV_THROW("DirectX11 Exception!!", #x);
#define dxCheck(x) if((x) != 0) return false

inline SV::DirectX11Device& ParseDevice(SV::Graphics& device)
{
	return *reinterpret_cast<SV::DirectX11Device*>(&device);
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

namespace SV {

	bool DirectX11Device::_Initialize(const SV_GRAPHICS_INITIALIZATION_DESC& desc)
	{
		Window& window = GetEngine().GetWindow();

		// Create swap chain
		HWND windowHandle = reinterpret_cast<HWND>(window.GetWindowHandle());

		DXGI_SWAP_CHAIN_DESC swapChainDescriptor;
		svZeroMemory(&swapChainDescriptor, sizeof(DXGI_SWAP_CHAIN_DESC));
		swapChainDescriptor.BufferCount = 1;
		swapChainDescriptor.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDescriptor.BufferDesc.Height = 0;
		swapChainDescriptor.BufferDesc.Width = 0;
		swapChainDescriptor.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDescriptor.BufferDesc.RefreshRate.Numerator = 1;
		swapChainDescriptor.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDescriptor.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
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

		D3D11_RENDER_TARGET_VIEW_DESC backBufferDesc;
		backBufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		backBufferDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		backBufferDesc.Texture2D.MipSlice = 0u;

		ValidateFrameBuffer(&mainFrameBuffer);

		FrameBuffer_dx11* backBuffer = reinterpret_cast<FrameBuffer_dx11*>(mainFrameBuffer.Get());

		backBuffer->m_Width = 1280;
		backBuffer->m_Height = 720;
		backBuffer->m_Format = SV_GFX_FORMAT_B8G8R8A8_UNORM;

		dxAssert(swapChain->GetBuffer(0u, __uuidof(ID3D11Resource), &backBuffer->GetResource()));
		dxAssert(device->CreateRenderTargetView(backBuffer->GetResource().Get(), &backBufferDesc, &backBuffer->GetRTV()));

		return true;
	}

	bool DirectX11Device::_Close()
	{
		// Release Deferred Contexts
		for (ui32 i = 0; i < SV_GFX_COMMAND_LIST_COUNT; ++i)
		{
			deferredContext[i].Reset();
		}

		mainFrameBuffer->Release();
		immediateContext.Reset();
		swapChain.Reset();
		device.Reset();


		return true;
	}

	void DirectX11Device::Present()
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

		for (ui32 i = 0; i < SV_GFX_COMMAND_LIST_COUNT; ++i) {
			stateManager[i].Reset();
		}
	}

	CommandList DirectX11Device::BeginCommandList()
	{
		ui32 ID = 0;

		if (!freeCommandLists.pop(ID)) {
			ID = commandListCount.fetch_add(1);
			SV_ASSERT(ID < SV_GFX_COMMAND_LIST_COUNT);
			dxAssert(device->CreateDeferredContext(0u, &deferredContext[ID]));
		}
		activeCommandLists.push(ID);

		return CommandList(this, ID);
	}

	void DirectX11Device::SetViewport(ui32 slot, float x, float y, float w, float h, float n, float f, SV::CommandList& cmd)
	{
		stateManager[cmd.GetID()].SetViewport({x, y, w, h, n, f}, slot);
	}

	void DirectX11Device::SetTopology(SV_GFX_TOPOLOGY topology, CommandList& cmd)
	{
		deferredContext[cmd.GetID()]->IASetPrimitiveTopology(ParseTopology(topology));
	}

	SV::FrameBuffer& DirectX11Device::GetMainFrameBuffer()
	{
		return mainFrameBuffer;
	}

	void DirectX11Device::Draw(ui32 vertexCount, ui32 startVertex, CommandList& cmd)
	{
		stateManager[cmd.GetID()].BindState(deferredContext[cmd.GetID()]);
		deferredContext[cmd.GetID()]->Draw(vertexCount, startVertex);
	}
	void DirectX11Device::DrawIndexed(ui32 indexCount, ui32 startIndex, ui32 startVertex, CommandList& cmd)
	{
		stateManager[cmd.GetID()].BindState(deferredContext[cmd.GetID()]);
		deferredContext[cmd.GetID()]->DrawIndexed(indexCount, startIndex, startVertex);
	}
	void DirectX11Device::DrawInstanced(ui32 verticesPerInstance, ui32 instances, ui32 startVertex, ui32 startInstance, CommandList& cmd)
	{
		stateManager[cmd.GetID()].BindState(deferredContext[cmd.GetID()]);
		deferredContext[cmd.GetID()]->DrawInstanced(verticesPerInstance, instances, startVertex, startInstance);
	}
	void DirectX11Device::DrawIndexedInstanced(ui32 indicesPerInstance, ui32 instances, ui32 startIndex, ui32 startVertex, ui32 startInstance, CommandList& cmd)
	{
		stateManager[cmd.GetID()].BindState(deferredContext[cmd.GetID()]);
		deferredContext[cmd.GetID()]->DrawIndexedInstanced(indicesPerInstance, instances, startIndex, startVertex, startInstance);
	}

	///////////////////////////////VERTEX BUFFER///////////////////////////////////
	void UpdateBuffer(ID3D11Resource* res, SV_GFX_USAGE usage, ui32 bufferSize, void* data, ui32 size, CommandList& cmd)
	{
		SV_ASSERT(usage != SV_GFX_USAGE_STATIC && bufferSize >= size);

		DirectX11Device& dx11 = ParseDevice(cmd.GetDevice());

		if (usage == SV_GFX_USAGE_DYNAMIC) {
			D3D11_MAPPED_SUBRESOURCE map;
			dx11.deferredContext[cmd.GetID()]->Map(res, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &map);

			memcpy(map.pData, data, (size == 0u) ? bufferSize : size);

			dx11.deferredContext[cmd.GetID()]->Unmap(res, 0u);
		}
		else {
			dx11.deferredContext[cmd.GetID()]->UpdateSubresource(res, 0, nullptr, data, 0, 0);
		}
	}

	bool VertexBuffer_dx11::_Create(ui32 size, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, void* data, Graphics& d)
	{
		D3D11_BUFFER_DESC desc;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.ByteWidth = size;
		desc.CPUAccessFlags = (CPUWriteAccess ? D3D11_CPU_ACCESS_WRITE : 0u) | (CPUReadAccess ? D3D11_CPU_ACCESS_READ : 0u);
		desc.MiscFlags = 0u;
		desc.StructureByteStride = 0u;
		desc.Usage = ParseUsage(usage);

		DirectX11Device& dx11 = ParseDevice(d);

		if (data) {
			D3D11_SUBRESOURCE_DATA sdata;
			sdata.pSysMem = data;
			dxCheck(dx11.device->CreateBuffer(&desc, &sdata, m_Buffer.GetAddressOf()));
		}
		else {
			dxCheck(dx11.device->CreateBuffer(&desc, nullptr, m_Buffer.GetAddressOf()));
		}

		return true;
	}
	void VertexBuffer_dx11::_Release()
	{
		m_Buffer.Reset();
	}
	void VertexBuffer_dx11::_Update(void* data, ui32 size, CommandList& cmd)
	{
		UpdateBuffer(m_Buffer.Get(), m_Usage, m_Size, data, size, cmd);
	}
	void VertexBuffer_dx11::_Bind(ui32 slot, ui32 stride, ui32 offset, CommandList& cmd)
	{
		SV_ASSERT(slot < SV_GFX_VERTEX_BUFFER_COUNT);
		DirectX11Device& dx11 = ParseDevice(cmd.GetDevice());
		dx11.stateManager[cmd.GetID()].BindVB(m_Buffer.Get(), slot, stride, offset);
	}
	void VertexBuffer_dx11::_Unbind(CommandList& cmd)
	{
		DirectX11Device& dx11 = ParseDevice(cmd.GetDevice());
		dx11.stateManager[cmd.GetID()].UnbindVB(m_LastSlot);
	}

	///////////////////////////////INDEX BUFFER///////////////////////////////////
	bool IndexBuffer_dx11::_Create(ui32 size, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, void* data, Graphics& d)
	{
		D3D11_BUFFER_DESC desc;
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		desc.ByteWidth = size;
		desc.CPUAccessFlags = (CPUWriteAccess ? D3D11_CPU_ACCESS_WRITE : 0u) | (CPUReadAccess ? D3D11_CPU_ACCESS_READ : 0u);
		desc.MiscFlags = 0u;
		desc.StructureByteStride = 0u;
		desc.Usage = ParseUsage(usage);

		DirectX11Device& dx11 = ParseDevice(d);

		if (data) {
			D3D11_SUBRESOURCE_DATA sdata;
			sdata.pSysMem = data;
			dxCheck(dx11.device->CreateBuffer(&desc, &sdata, m_Buffer.GetAddressOf()));
		}
		else {
			dxCheck(dx11.device->CreateBuffer(&desc, nullptr, m_Buffer.GetAddressOf()));
		}

		return true;
	}
	void IndexBuffer_dx11::_Release()
	{
		m_Buffer.Reset();
	}
	void IndexBuffer_dx11::_Bind(SV_GFX_FORMAT format, ui32 offset, CommandList& cmd)
	{
		DirectX11Device& dx11 = ParseDevice(cmd.GetDevice());
		dx11.deferredContext[cmd.GetID()]->IASetIndexBuffer(m_Buffer.Get(), ParseFormat(format), offset);
	}
	void IndexBuffer_dx11::_Unbind(CommandList& cmd)
	{
		DirectX11Device& dx11 = ParseDevice(cmd.GetDevice());
		dx11.deferredContext[cmd.GetID()]->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32G32B32A32_FLOAT, 0u);
	}

	///////////////////////////////CONSTANT BUFFER///////////////////////////////////
	bool ConstantBuffer_dx11::_Create(ui32 size, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, void* data, Graphics& d)
	{
		D3D11_BUFFER_DESC desc;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.ByteWidth = size;
		desc.CPUAccessFlags = (CPUWriteAccess ? D3D11_CPU_ACCESS_WRITE : 0u) | (CPUReadAccess ? D3D11_CPU_ACCESS_READ : 0u);
		desc.MiscFlags = 0u;
		desc.StructureByteStride = 0u;
		desc.Usage = ParseUsage(usage);

		DirectX11Device& dx11 = ParseDevice(d);

		if (data) {
			D3D11_SUBRESOURCE_DATA sdata;
			sdata.pSysMem = data;
			dxCheck(dx11.device->CreateBuffer(&desc, &sdata, m_Buffer.GetAddressOf()));
		}
		else {
			dxCheck(dx11.device->CreateBuffer(&desc, nullptr, m_Buffer.GetAddressOf()));
		}

		return true;
	}
	void ConstantBuffer_dx11::_Release()
	{
		m_Buffer.Reset();
	}
	void ConstantBuffer_dx11::_Bind(ui32 slot, SV_GFX_SHADER_TYPE type, CommandList& cmd)
	{
		SV_ASSERT(slot < SV_GFX_CONSTANT_BUFFER_COUNT);
		DirectX11Device& dx11 = ParseDevice(cmd.GetDevice());
		dx11.stateManager[cmd.GetID()].BindCB(m_Buffer.Get(), type, slot);
	}
	void ConstantBuffer_dx11::_Unbind(SV_GFX_SHADER_TYPE type, CommandList& cmd)
	{
		DirectX11Device& dx11 = ParseDevice(cmd.GetDevice());
		dx11.stateManager[cmd.GetID()].UnbindCB(type, m_LastSlot[type]);
	}

	///////////////////////////////FRAME BUFFER///////////////////////////////////

	bool FrameBuffer_dx11::_Create(ui32 width, ui32 height, SV_GFX_FORMAT format, bool textureUsage, SV::Graphics& d)
	{
		DirectX11Device& dx11 = ParseDevice(d);
		// Render target view desc
		{
			D3D11_RENDER_TARGET_VIEW_DESC desc;
			desc.Format = ParseFormat(format);
			desc.Texture2D.MipSlice = 0u;
			desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

			UINT bindFlags = D3D11_BIND_RENDER_TARGET;
			if (textureUsage) bindFlags |= D3D11_BIND_SHADER_RESOURCE;

			// Resource desc
			D3D11_TEXTURE2D_DESC resDesc;
			resDesc.ArraySize = 1u;
			resDesc.BindFlags = bindFlags;
			resDesc.CPUAccessFlags = 0u;
			resDesc.Format = ParseFormat(format);
			resDesc.Width = width;
			resDesc.Height = height;
			resDesc.MipLevels = 1u;
			resDesc.MiscFlags = 0u;
			resDesc.SampleDesc.Count = 1u;
			resDesc.SampleDesc.Quality = 0u;
			resDesc.Usage = D3D11_USAGE_DEFAULT;

			dxCheck(dx11.device->CreateTexture2D(&resDesc, nullptr, &m_Resource));
			dxCheck(dx11.device->CreateRenderTargetView(m_Resource.Get(), &desc, &m_RenderTargetView));
		}

		// Shader resouce
		if (textureUsage) {
			D3D11_SHADER_RESOURCE_VIEW_DESC desc;
			svZeroMemory(&desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
			desc.Format = ParseFormat(format);
			desc.Texture2D.MipLevels = 1u;
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

			dxCheck(dx11.device->CreateShaderResourceView(m_Resource.Get(), &desc, &m_ShaderResouceView));
		}
		
		return true;
	}
	void FrameBuffer_dx11::_Release()
	{
		m_RenderTargetView.Reset();
		m_Resource.Reset();
		m_ShaderResouceView.Reset();
	}
	bool FrameBuffer_dx11::_Resize(ui32 width, ui32 height, SV::Graphics& d)
	{
		if (width == m_Width && height == m_Height) return true;
		Release();
		return Create(width, height, m_Format, m_TextureUsage, d);
	}
	void FrameBuffer_dx11::_Clear(SV::Color4f c, CommandList& cmd)
	{
		const FLOAT color[4] = {
			c.x, c.y, c.z, c.w
		};
		DirectX11Device& dx11 = ParseDevice(cmd.GetDevice());
		dx11.deferredContext[cmd.GetID()]->ClearRenderTargetView(m_RenderTargetView.Get(), color);
	}
	void FrameBuffer_dx11::_Bind(ui32 slot, CommandList& cmd)
	{
		SV_ASSERT(slot < SV_GFX_RENDER_TARGET_VIEW_COUNT);
		DirectX11Device& dx11 = ParseDevice(cmd.GetDevice());
		dx11.stateManager[cmd.GetID()].BindRTV(m_RenderTargetView.Get(), slot);
	}
	void FrameBuffer_dx11::_Unbind(CommandList& cmd)
	{
		DirectX11Device& dx11 = ParseDevice(cmd.GetDevice());
		dx11.stateManager[cmd.GetID()].UnbindRTV(m_LastSlot);
	}

	void FrameBuffer_dx11::_BindAsTexture(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd)
	{
		DirectX11Device& dx11 = ParseDevice(cmd.GetDevice());
		dx11.stateManager[cmd.GetID()].BindTex(m_ShaderResouceView.Get(), type, slot);
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

	bool Shader_dx11::_Create(SV_GFX_SHADER_TYPE type, const char* filePath, SV::Graphics& device)
	{
		std::wstring wFilePath = SV::Utils::ToWString(filePath);

		// Read File
		dxCheck(D3DReadFileToBlob(wFilePath.c_str(), &m_Blob));

		// Create Shader
		DirectX11Device& dx11 = ParseDevice(device);

		switch (type)
		{
		case SV_GFX_SHADER_TYPE_VERTEX:
			dxCheck(dx11.device->CreateVertexShader(m_Blob->GetBufferPointer(), m_Blob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11VertexShader**>(&m_Shader)));
			break;
		case SV_GFX_SHADER_TYPE_PIXEL:
			dxCheck(dx11.device->CreatePixelShader(m_Blob->GetBufferPointer(), m_Blob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11PixelShader**>(&m_Shader)));
			break;
		case SV_GFX_SHADER_TYPE_GEOMETRY:
			dxCheck(dx11.device->CreateGeometryShader(m_Blob->GetBufferPointer(), m_Blob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11GeometryShader**>(&m_Shader)));
			break;
		case SV_GFX_SHADER_TYPE_UNDEFINED:
		default:
			SV::LogE("Invalid Shader Type '%u'", type);
			return false;
		}

		if (type != SV_GFX_SHADER_TYPE_VERTEX) m_Blob.Reset();

		return true;
	}

	void Shader_dx11::_Release()
	{
		if (m_Shader) {
			switch (m_ShaderType) {
			case SV_GFX_SHADER_TYPE_VERTEX:
			{
				auto vs = ToVS(m_Shader);
				vs->Release();
				break;
			}
			case SV_GFX_SHADER_TYPE_PIXEL:
			{
				auto ps = ToPS(m_Shader);
				ps->Release();
				break;
			}
			case SV_GFX_SHADER_TYPE_GEOMETRY:
			{
				auto gs = ToGS(m_Shader);
				gs->Release();
				break;
			}
			}
		}
		m_Blob.Reset();
	}

	void Shader_dx11::_Bind(CommandList& cmd)
	{
		DirectX11Device& dx11 = ParseDevice(cmd.GetDevice());

		switch (m_ShaderType) {
		case SV_GFX_SHADER_TYPE_VERTEX:
		{
			auto vs = ToVS(m_Shader);
			
			dx11.deferredContext[cmd.GetID()]->VSSetShader(vs, nullptr, 0u);

			break;
		}
		case SV_GFX_SHADER_TYPE_PIXEL:
		{
			auto ps = ToPS(m_Shader);
			
			dx11.deferredContext[cmd.GetID()]->PSSetShader(ps, nullptr, 0u);

			break;
		}
		case SV_GFX_SHADER_TYPE_GEOMETRY:
		{
			auto gs = ToGS(m_Shader);

			dx11.deferredContext[cmd.GetID()]->GSSetShader(gs, nullptr, 0u);
			
			break;
		}
		}
	}

	void Shader_dx11::_Unbind(CommandList& cmd)
	{
		DirectX11Device& dx11 = ParseDevice(cmd.GetDevice());

		switch (m_ShaderType) {
		case SV_GFX_SHADER_TYPE_VERTEX:
			dx11.deferredContext[cmd.GetID()]->VSSetShader(nullptr, nullptr, 0u);
			break;
		case SV_GFX_SHADER_TYPE_PIXEL:
			dx11.deferredContext[cmd.GetID()]->PSSetShader(nullptr, nullptr, 0u);
			break;
		case SV_GFX_SHADER_TYPE_GEOMETRY:
			dx11.deferredContext[cmd.GetID()]->GSSetShader(nullptr, nullptr, 0u);
			break;
		}
	}

	/////////////////////////////////INPUT LAYOUT//////////////////////////////////////////
	bool InputLayout_dx11::_Create(const SV_GFX_INPUT_ELEMENT_DESC* d, ui32 count, const Shader& vs, SV::Graphics& device)
	{
		SV_ASSERT(count < 16);

		if (vs->GetType() != SV_GFX_SHADER_TYPE_VERTEX) {
			SV::LogE("To create an input layout must have a vertex shader");
			return false;
		}

		D3D11_INPUT_ELEMENT_DESC desc[16];

		for (ui32 i = 0; i < count; ++i) {
			desc[i].AlignedByteOffset = d[i].AlignedByteOffset;
			desc[i].Format = ParseFormat(d[i].Format);
			desc[i].InputSlot = d[i].InputSlot;
			desc[i].InputSlotClass = (d[i].InstancedData) ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;
			desc[i].InstanceDataStepRate = d[i].InstanceDataStepRate;
			desc[i].SemanticIndex = d[i].SemanticIndex;
			desc[i].SemanticName = d[i].SemanticName;
		}

		DirectX11Device& dx11 = ParseDevice(device);
		Shader_dx11* shader = reinterpret_cast<Shader_dx11*>(vs.Get());

		dxCheck(dx11.device->CreateInputLayout(desc, count, shader->GetBlob()->GetBufferPointer(), shader->GetBlob()->GetBufferSize(), &m_InputLayout));

		return true;
	}
	void InputLayout_dx11::_Release()
	{
		m_InputLayout.Reset();
	}
	void InputLayout_dx11::_Bind(CommandList& cmd)
	{
		DirectX11Device& dx11 = ParseDevice(cmd.GetDevice());
		dx11.deferredContext[cmd.GetID()]->IASetInputLayout(m_InputLayout.Get());
	}
	void InputLayout_dx11::_Unbind(CommandList& cmd)
	{
		DirectX11Device& dx11 = ParseDevice(cmd.GetDevice());
		dx11.deferredContext[cmd.GetID()]->IASetInputLayout(nullptr);
	}

	/////////////////////////////////TEXTURE//////////////////////////////////////////
	bool Texture_dx11::_Create(void* data, ui32 width, ui32 height, SV_GFX_FORMAT format, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, SV::Graphics& device)
	{
		DirectX11Device& dx11 = ParseDevice(device);
		// Resource
		{
			D3D11_TEXTURE2D_DESC desc = {};
			desc.ArraySize = 1u;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = (CPUWriteAccess ? D3D11_CPU_ACCESS_WRITE : 0u) | (CPUReadAccess ? D3D11_CPU_ACCESS_READ : 0u);
			desc.Format = ParseFormat(format);
			desc.Width = width;
			desc.Height = height;
			desc.MipLevels = 1u;
			desc.MiscFlags = 0u;
			desc.SampleDesc.Count = 1u;
			desc.SampleDesc.Quality = 0u;
			desc.Usage = ParseUsage(usage);

			if (data) {
				D3D11_SUBRESOURCE_DATA sdata;
				sdata.pSysMem = data;
				sdata.SysMemPitch = width * SV::GetFormatSize(format);

				dxCheck(dx11.device->CreateTexture2D(&desc, &sdata, &m_Texture));
			}
			else {
				dxCheck(dx11.device->CreateTexture2D(&desc, nullptr, &m_Texture));
			}
		}

		// Shader resource view
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC desc;
			svZeroMemory(&desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
			desc.Format = ParseFormat(format);
			desc.Texture2D.MipLevels = 1u;
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

			dxCheck(dx11.device->CreateShaderResourceView(m_Texture.Get(), &desc, &m_ShaderResouceView));
		}

		return true;
	}
	void Texture_dx11::_Release()
	{
		m_Texture.Reset();
		m_ShaderResouceView.Reset();
	}
	void Texture_dx11::_Update(void* data, ui32 size, CommandList& cmd)
	{
		UpdateBuffer(m_Texture.Get(), m_Usage, m_Size, data, size, cmd);
	}
	void Texture_dx11::_Bind(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd)
	{
		DirectX11Device& dx11 = ParseDevice(cmd.GetDevice());
		dx11.stateManager[cmd.GetID()].BindTex(m_ShaderResouceView.Get(), type, slot);
	}
	void Texture_dx11::_Unbind(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd)
	{
		DirectX11Device& dx11 = ParseDevice(cmd.GetDevice());
		dx11.stateManager[cmd.GetID()].UnbindTex(type, slot);
	}

	/////////////////////////////////SAMPLER//////////////////////////////////////////
	bool Sampler_dx11::_Create(SV_GFX_TEXTURE_ADDRESS_MODE addressMode, SV_GFX_TEXTURE_FILTER filter, Graphics& device)
	{
		D3D11_SAMPLER_DESC desc;
		svZeroMemory(&desc, sizeof(D3D11_SAMPLER_DESC));
		desc.AddressU = ParseAddressMode(addressMode);
		desc.AddressV = ParseAddressMode(addressMode);
		desc.AddressW = ParseAddressMode(addressMode);
		desc.Filter = ParseTextureFilter(filter);

		DirectX11Device& dx11 = ParseDevice(device);
		dxCheck(dx11.device->CreateSamplerState(&desc, &m_SamplerState));

		return true;
	}
	void Sampler_dx11::_Release()
	{
		m_SamplerState.Reset();
	}
	void Sampler_dx11::_Bind(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd)
	{
		DirectX11Device& dx11 = ParseDevice(cmd.GetDevice());
		dx11.stateManager[cmd.GetID()].BindSam(m_SamplerState.Get(), type, slot);
	}
	void Sampler_dx11::_Unbind(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd)
	{
		DirectX11Device& dx11 = ParseDevice(cmd.GetDevice());
		dx11.stateManager[cmd.GetID()].UnbindSam(type, slot);
	}

}