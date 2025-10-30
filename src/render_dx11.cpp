
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
	D3D11_VIEWPORT viewport;
	ID3D11Texture2D* depth_buffer;
	ID3D11DepthStencilView* depth_view;
};

global render_state *renderer;


void render_resize(i32 width, i32 height)
{
  // Release the old views, as they hold references to the buffers we
	// will be destroying.  Also release the old depth/stencil buffer.

	renderer->render_target->Release();
	renderer->depth_view->Release();
	renderer->depth_buffer->Release();

	// Resize the swap chain and recreate the render target view.
  HRESULT success = 0;
	success = renderer->swapchain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
  ASSERT(SUCCEEDED(success), "Failed to resize swapchain buffers.");
	ID3D11Texture2D* backBuffer;
	success = renderer->swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)(&backBuffer));
  ASSERT(SUCCEEDED(success), "Failed to get back buffer.");
	success = renderer->device->CreateRenderTargetView(backBuffer, 0, &renderer->render_target);
  ASSERT(SUCCEEDED(success), "Failed to create render target view.");
	backBuffer->Release();

	// Create the depth/stencil buffer and view.
	D3D11_TEXTURE2D_DESC depthStencilDesc = {};
	depthStencilDesc.Width     = width;
	depthStencilDesc.Height    = height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Use 4X MSAA? --must match swap chain MSAA values.
	// No MSAA
  depthStencilDesc.SampleDesc.Count   = 1;
  depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage          = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags      = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0; 
	depthStencilDesc.MiscFlags      = 0;

	success = renderer->device->CreateTexture2D(&depthStencilDesc, 0, &renderer->depth_buffer);
  ASSERT(SUCCEEDED(success), "Failed to create depth buffer.");
	success = renderer->device->CreateDepthStencilView(renderer->depth_buffer, 0, &renderer->depth_view);
  ASSERT(SUCCEEDED(success), "Failed to create depth view.");

	// Bind the render target view and depth/stencil view to the pipeline.
	renderer->context->OMSetRenderTargets(1, &renderer->render_target, renderer->depth_view);

	// Set the viewport transform.
	renderer->viewport.TopLeftX = 0;
	renderer->viewport.TopLeftY = 0;
	renderer->viewport.Width    = static_cast<float>(width);
	renderer->viewport.Height   = static_cast<float>(height);
	renderer->viewport.MinDepth = 0.0f;
	renderer->viewport.MaxDepth = 1.0f;

	renderer->context->RSSetViewports(1, &renderer->viewport);
}


void render_init(arena *a)
{
  // Initialize render state data
  renderer = arena_push_struct(a, render_state);
  HWND *window = (HWND*) platform_window_handle();
  // Initialize result variable used for a lot of creation
  HRESULT result;
  UINT device_flags = 0;
  #if defined(_DEBUG)  
  device_flags |= D3D11_CREATE_DEVICE_DEBUG;
  #endif

  D3D_DRIVER_TYPE driver_type = D3D_DRIVER_TYPE_HARDWARE;

  // Feature level array
  D3D_FEATURE_LEVEL feature_level;
	result = D3D11CreateDevice(
    0,                 // default adapter
    driver_type,
    0,                 // no software device
    device_flags, 
    0, 0,              // default feature level array
    D3D11_SDK_VERSION,
    &renderer->device,
    &feature_level,
    &renderer->context     // Immediate context
  );
  ASSERT(SUCCEEDED(result), "ERROR: Failed to create the device.");
  ASSERT(feature_level == D3D_FEATURE_LEVEL_11_0, "ERROR: Failed to init d3d11.");

  // Check 4X MSAA quality support
  u32 msaa_quality;
  result = renderer->device->CheckMultisampleQualityLevels(
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
	scd.OutputWindow = *window;
  scd.Windowed     = TRUE;
	scd.SwapEffect   = DXGI_SWAP_EFFECT_DISCARD;
	scd.Flags        = 0;

  IDXGIDevice* dxgiDevice = 0;
	result = renderer->device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
	      
	IDXGIAdapter* dxgiAdapter = 0;
	result = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);

	IDXGIFactory* dxgiFactory = 0;
	result = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);

	result = dxgiFactory->CreateSwapChain(renderer->device, &scd, &renderer->swapchain);
  ASSERT(SUCCEEDED(result), "ERROR: Failed to create the swapchain.");
	
	dxgiDevice->Release();
	dxgiAdapter->Release();
	dxgiFactory->Release();

  RECT size;
  GetClientRect(*window, &size);
  i32 width = size.right;
  i32 height = size.bottom;
  // TODO: All code below is duplicated in render_resize, but at this time there's nothing to release.
  // It feels wasteful to include an if(x) then release everytime we call resize...

  // Resize the swap chain and recreate the render target view.
  HRESULT success = 0;
	success = renderer->swapchain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
  ASSERT( SUCCEEDED(success) , "Failed to resize swapchain buffers.");
	ID3D11Texture2D* backBuffer;
	success = renderer->swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)(&backBuffer));
  ASSERT( SUCCEEDED(success) , "Failed to get back buffer.");
	success = renderer->device->CreateRenderTargetView(backBuffer, 0, &renderer->render_target);
  ASSERT( SUCCEEDED(success) , "Failed to create render target view.");
	backBuffer->Release();

	// Create the depth/stencil buffer and view.
	D3D11_TEXTURE2D_DESC depthStencilDesc = {};
	depthStencilDesc.Width     = width;
	depthStencilDesc.Height    = height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Use 4X MSAA? --must match swap chain MSAA values.
	// No MSAA
  depthStencilDesc.SampleDesc.Count   = 1;
  depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage          = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags      = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0; 
	depthStencilDesc.MiscFlags      = 0;

	success = renderer->device->CreateTexture2D(&depthStencilDesc, 0, &renderer->depth_buffer);
  ASSERT( SUCCEEDED(success) , "Failed to create depth buffer.");
	success = renderer->device->CreateDepthStencilView(renderer->depth_buffer, 0, &renderer->depth_view);
  ASSERT( SUCCEEDED(success) , "Failed to create depth view.");

	// Bind the render target view and depth/stencil view to the pipeline.
	renderer->context->OMSetRenderTargets(1, &renderer->render_target, renderer->depth_view);

	// Set the viewport transform.
	renderer->viewport.TopLeftX = 0;
	renderer->viewport.TopLeftY = 0;
	renderer->viewport.Width    = (float)(width);
	renderer->viewport.Height   = (float)(height);
	renderer->viewport.MinDepth = 0.0f;
	renderer->viewport.MaxDepth = 1.0f;

	renderer->context->RSSetViewports(1, &renderer->viewport);

}


void frame_init()
{
  f32 color[4] = {0.0f, 0.325f, 0.282f, 1.0f};
  renderer->context->ClearRenderTargetView(renderer->render_target, color);
}


void frame_render()
{
  renderer->swapchain->Present(1, 0); // vsync on
}


void render_close()
{
  renderer->render_target->Release();
  renderer->swapchain->Release();
  renderer->context->Release();
  renderer->device->Release();
}
