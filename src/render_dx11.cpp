
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>


// Data types
struct render_state
{
  ID3D11Device* device;
  IDXGISwapChain* swapchain;
  ID3D11DeviceContext* context;
  ID3D11RenderTargetView* render_target; // Pointer to object containing render target info
};

render_state render_init(HWND handle)
{
  // Initialize render state data
  render_state state = {};
  // Initialize result variable used for a lot of creation
  HRESULT result;
  UINT device_flags = 0;
  #if defined(_DEBUG)  
  device_flags |= D3D11_CREATE_DEVICE_DEBUG;
  #endif

  D3D_DRIVER_TYPE driver_type = D3D_DRIVER_TYPE_HARDWARE;
	ID3D11Texture2D* depth_buffer;
	ID3D11RenderTargetView* render_view;
	ID3D11DepthStencilView* depth_view;
	D3D11_VIEWPORT viewport;


  // Feature level array
  D3D_FEATURE_LEVEL feature_level;
	result = D3D11CreateDevice(
    0,                 // default adapter
    driver_type,
    0,                 // no software device
    device_flags, 
    0, 0,              // default feature level array
    D3D11_SDK_VERSION,
    &state.device,
    &feature_level,
    &state.context     // Immediate context
  );
  ASSERT(SUCCEEDED(result), "ERROR: Failed to create the device.");
  ASSERT(feature_level == D3D_FEATURE_LEVEL_11_0, "ERROR: Failed to init d3d11.");

  // Check 4X MSAA quality support
  u32 msaa_quality;
  result = state.device->CheckMultisampleQualityLevels(
		DXGI_FORMAT_R8G8B8A8_UNORM,
    4,
    &msaa_quality
  );
  ASSERT(SUCCEEDED(result), "ERROR: Failed to query MSAA support.");
	ASSERT( msaa_quality > 0, "ERROR: This should always be supported and greater than 0.");


  DXGI_SWAP_CHAIN_DESC scd = {};
  scd.SampleDesc.Count = 1;

  // sd.BufferDesc.Width  = mClientWidth;
	// sd.BufferDesc.Height = mClientHeight;
	// sd.BufferDesc.RefreshRate.Numerator = 60;
	// sd.BufferDesc.RefreshRate.Denominator = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	// sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	// sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

  // No MSAA
  scd.SampleDesc.Count   = 1;
  scd.SampleDesc.Quality = 0;

  scd.BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.BufferCount  = 1;
	scd.OutputWindow = handle;
  scd.Windowed = TRUE;
	scd.SwapEffect   = DXGI_SWAP_EFFECT_DISCARD;
	scd.Flags        = 0;

  IDXGIDevice* dxgiDevice = 0;
	result = state.device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
	      
	IDXGIAdapter* dxgiAdapter = 0;
	result = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);

	IDXGIFactory* dxgiFactory = 0;
	result = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);

	result = dxgiFactory->CreateSwapChain(state.device, &scd, &state.swapchain);
  ASSERT(SUCCEEDED(result), "ERROR: Failed to create the swapchain.");
	
	dxgiDevice->Release();
	dxgiAdapter->Release();
	dxgiFactory->Release();


  // TODO: Copy/create OnResize() function as seen in d3d11-book and call it here
  
  // TODO: Is this still necessary?
  // Create render target view
  ID3D11Texture2D* backBuffer = nullptr;
  state.swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
  state.device->CreateRenderTargetView(backBuffer, nullptr, &state.render_target);
  backBuffer->Release();
  state.context->OMSetRenderTargets(1, &state.render_target, nullptr);


  return state;
}

void frame_render(render_state *state)
{
  f32 color[4] = {0.0f, 0.325f, 0.282f, 1.0f};
  state->context->ClearRenderTargetView(state->render_target, color);
  state->swapchain->Present(1, 0); // vsync on
}

void render_close(render_state *state)
{
  state->render_target->Release();
  state->swapchain->Release();
  state->context->Release();
  state->device->Release();
}
