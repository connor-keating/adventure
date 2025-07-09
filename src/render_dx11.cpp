
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>


// Data types
struct render_state
{
  IDXGISwapChain* swapchain;
  ID3D11Device* device;
  ID3D11DeviceContext* context;
  ID3D11RenderTargetView* render_target; // Pointer to object containing render target info
};

render_state render_init(HWND handle)
{
  render_state state = {};

  DXGI_SWAP_CHAIN_DESC scd = {};
  scd.BufferCount = 1;
  scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  scd.OutputWindow = handle;                  // <-- your Win32 HWND
  scd.SampleDesc.Count = 1;
  scd.Windowed = TRUE;

  D3D11CreateDeviceAndSwapChain(
      nullptr,
      D3D_DRIVER_TYPE_HARDWARE,
      nullptr,
      0,
      nullptr, 0,
      D3D11_SDK_VERSION,
      &scd,
      &state.swapchain,
      &state.device,
      nullptr,
      &state.context
  );

  // Create render target view
  ID3D11Texture2D* backBuffer = nullptr;
  state.swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
  state.device->CreateRenderTargetView(backBuffer, nullptr, &state.render_target);
  backBuffer->Release();
  state.context->OMSetRenderTargets(1, &state.render_target, nullptr);


  return state;
}
