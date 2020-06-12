#include "core.h"

#include "Graphics.h"

#include "Dx11.h"

namespace SV {
	namespace User {

		SV_GFX_API g_GraphicsAPI = SV_GFX_API_INVALID;
		std::vector<SV::Adapter> g_Adapters;

		namespace _internal {
			bool Initialize()
			{
				// Select Graphics API
				g_GraphicsAPI = SV_GFX_API_DX11;

				// Graphics Adapters
				{
					SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);

					ComPtr<IDXGIFactory1> factory;
					CreateDXGIFactory1(__uuidof(IDXGIFactory1), &factory);

					ComPtr<IDXGIAdapter1> adapter_dx11;
					for (ui32 i = 0; factory->EnumAdapters1(i, &adapter_dx11) != DXGI_ERROR_NOT_FOUND; ++i) {

						SV::Adapter adapter;

						{
							DXGI_ADAPTER_DESC desc;
							adapter_dx11->GetDesc(&desc);

							adapter.description = desc.Description;
						}

						ComPtr<IDXGIOutput> output;
						for (ui32 j = 0; adapter_dx11->EnumOutputs(j, &output) != DXGI_ERROR_NOT_FOUND; ++j) {

							SV::Adapter::OutputMode outputMode;

							{
								DXGI_OUTPUT_DESC outputDesc;
								output->GetDesc(&outputDesc);

								outputMode.deviceName = outputDesc.DeviceName;
								outputMode.resolution.x = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
								outputMode.resolution.y = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;
							}
							
							{
								DXGI_MODE_DESC matchMode;
								matchMode.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
								matchMode.Width = outputMode.resolution.x;
								matchMode.Height = outputMode.resolution.y;
								matchMode.RefreshRate.Denominator = 0;
								matchMode.RefreshRate.Numerator = 0;
								matchMode.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
								matchMode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

								DXGI_MODE_DESC outputModeDesc;

								output->FindClosestMatchingMode(&matchMode, &outputModeDesc, NULL);

								outputMode.format = (SV_GFX_FORMAT)outputModeDesc.Format;
								outputMode.width = outputModeDesc.Width;
								outputMode.height = outputModeDesc.Height;
								outputMode.refreshRate.denominator = outputModeDesc.RefreshRate.Denominator;
								outputMode.refreshRate.numerator = outputModeDesc.RefreshRate.Numerator;
								outputMode.scaling = (SV_GFX_MODE_SCALING)outputModeDesc.Scaling;
								outputMode.scanlineOrdering = (SV_GFX_MODE_SCANLINE_ORDER)outputModeDesc.ScanlineOrdering;
							}

							adapter.modes.push_back(outputMode);
						}

						g_Adapters.push_back(adapter);
					}
				}


				return true;
			}
		}

		const std::vector<Adapter>& GetAdapters()
		{
			return g_Adapters;
		}
		SV_GFX_API GetGraphicsAPI()
		{
			return g_GraphicsAPI;
		}
	}
}