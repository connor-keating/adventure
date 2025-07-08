
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>


struct render_state
{
  IDXGISwapChain* swapchain;
};


render_state *render_init()
{
  printf("Initializing D3D11.\n");
  // TODO: What from this function needs to go in here?
  render_state state = {};

  IDXGISwapChain* swapchain;
  ID3D11Device* device;
  ID3D11DeviceContext* context;
  ID3D11RenderTargetView* render_target; // Pointer to object containing render target info

  DXGI_SWAP_CHAIN_DESC scd = {};
  scd.BufferCount       = 1;                               // One backbuffer
  scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;      // 32-bit color
  scd.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT; // How to use the swapchain
  scd.OutputWindow      = handle;                          // window handle
  scd.SampleDesc.Count  = 1;                               // How many multisamples
  scd.Windowed          = true;                            // windowed / full-screen mode

  D3D_FEATURE_LEVEL actual_level;
  UINT flags = D3D11_CREATE_DEVICE_DEBUG;
  D3D11CreateDeviceAndSwapChain(
      nullptr,
      D3D_DRIVER_TYPE_HARDWARE,
      nullptr,
      flags,
      nullptr, 0,
      D3D11_SDK_VERSION,
      &scd,
      &swapchain,
      &device,
      &actual_level,
      &context
  );

  // Create render target view
  ID3D11Texture2D* backbuffer = nullptr;
  swapchain->GetBuffer(
    0,                          // Number of backbuffers we only have one so 0
    __uuidof(ID3D11Texture2D),  // ID of the COM object
    (void**)&backbuffer         // Location of texture object.
  );
  // Create the render target object
  device->CreateRenderTargetView(backbuffer, nullptr, &render_target);
  // Release pointer because its now being handled by render target
  backbuffer->Release();
  context->OMSetRenderTargets(1, &render_target, nullptr);
  
  // Set the viewport
  RECT rect;
  GetClientRect(handle, &rect);
  D3D11_VIEWPORT viewport = {};
  viewport.TopLeftX     = rect.left;
  viewport.TopLeftY     = rect.top;
  viewport.Width        = rect.right;
  viewport.Height       = rect.bottom;
  context->RSSetViewports(1, &viewport);
  return &state;
}


void render_frame(render_state  *state)
{
  f32 color[4] = {0.0f, 0.325f, 0.282f, 1.0f};
  context->ClearRenderTargetView(render_target, color);
  swapchain->Present(1, 0); // vsync on
}


void render_close(render_state *state)
{
  printf("Closing renderer.\n");
  // close and release all existing COM objects
  swapchain->Release();
  device->Release();
  context->Release();
}
