// Source code
#include "render.h"

// External code
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>


struct shaders
{
  ID3D11VertexShader *vertex;
  ID3D11InputLayout  *vertex_in;
  ID3D11PixelShader  *pixel;
};


struct render_buffer
{
  ID3D11Buffer* buffer;
  u32 stride;
  u32 offset;
};

/*
1. Create array of ID3D11RasterizerState variables. It can be global and made in the init function.
*/

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


internal ID3D11InputLayout * render_vertex_description(ID3DBlob *vert_shader)
{
  D3D11_INPUT_ELEMENT_DESC il[] =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  };
  ID3D11InputLayout* layout = nullptr;
  renderer->device->CreateInputLayout(
    il, 
    _countof(il),
    vert_shader->GetBufferPointer(),
    vert_shader->GetBufferSize(),
    &layout
  );
  return layout;
}


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


rbuffer_ptr render_buffer_init(arena *a, buffer_type t, void* data, u32 stride, u32 byte_count)
{
  render_buffer *out = arena_push_struct(a, render_buffer);
  u32 flags = 0;
  switch (t)
  {
    case (VERTS): flags = D3D11_BIND_VERTEX_BUFFER; break;
    case (ELEMS): flags = D3D11_BIND_INDEX_BUFFER;  break;
    default: break;
  };
  ASSERT( (flags != 0), "Failed to set D3D11 flags.");
  D3D11_BUFFER_DESC vbd;
  vbd.Usage = D3D11_USAGE_IMMUTABLE;
  vbd.ByteWidth = byte_count;
  vbd.BindFlags = flags;
  vbd.CPUAccessFlags = 0;
  vbd.MiscFlags = 0;
  D3D11_SUBRESOURCE_DATA vinitData;
  vinitData.pSysMem = data;
  HRESULT hr = renderer->device->CreateBuffer(&vbd, &vinitData, &out->buffer);
  ASSERT( SUCCEEDED(hr), "Failed to create vertex buffer." );
  out->stride = stride;
  out->offset = 0;
  return out;
}


shaders_ptr shader_init(arena *a)
{
  // Init output
  shaders *s = arena_push_struct(a, shaders);
  return s;
}


void shader_load(shaders *s, shader_type t, const char *file, const char *entry, const char *target)
{
  // Convert file path to UTF-8 char
  wchar_t filewide[256];
  MultiByteToWideChar(CP_ACP, 0, file, -1, filewide, 256);
  // Compile shaders
  ID3DBlob* blob = nullptr;
  ID3DBlob* erro = nullptr;
  HRESULT hr = D3DCompileFromFile(
    filewide,
    nullptr,
    nullptr,
    entry,
    target,
    D3DCOMPILE_ENABLE_STRICTNESS,
    0,
    &blob,
    &erro
  );
  // Create shader object
  switch (t)
  {
    case (VERTEX):
    {
      renderer->device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &s->vertex);
      // 3) Define input layout 
      s->vertex_in = render_vertex_description(blob);
      break;
    }
    case (PIXEL):
    {
      renderer->device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &s->pixel);
      break;
    }
    default: break;
  };
}


void render_draw(rbuffer_ptr vbuffer, shaders_ptr s)
{
  renderer->context->IASetVertexBuffers(0, 1, &vbuffer->buffer, &vbuffer->stride, &vbuffer->offset);
  renderer->context->IASetInputLayout(s->vertex_in);
  renderer->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  renderer->context->VSSetShader(s->vertex, 0, 0);
  renderer->context->PSSetShader(s->pixel, 0, 0);
  renderer->context->Draw(3, 0);
}


void frame_init()
{
  f32 color[4] = {0.0f, 0.325f, 0.282f, 1.0f};
  renderer->context->ClearRenderTargetView(renderer->render_target, color);
  renderer->context->ClearDepthStencilView(renderer->depth_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
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
