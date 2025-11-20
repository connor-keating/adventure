// Source code
#include "render.h"

// External code
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

/*
1. Create array of ID3D11RasterizerState variables. It can be global and made in the init function.
*/

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

struct texture2d
{
  ID3D11Texture2D* texture;
  ID3D11ShaderResourceView* view;
  ID3D11SamplerState* sampler;
};

struct texture3d
{
  ID3D11Texture3D* texture;
  ID3D11ShaderResourceView* view;
  ID3D11SamplerState* sampler;
};


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
  ID3D11BlendState* blend_state;
  ID3D11RasterizerState* rasterizer_default;
};

global render_state *renderer;


internal ID3D11InputLayout * render_vertex_description(ID3DBlob *vert_shader)
{
  D3D11_INPUT_ELEMENT_DESC il[] =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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


internal DXGI_FORMAT format_select(u32 nchannels)
{
  // TODO: Add channel size parameter
  DXGI_FORMAT selection;
  switch (nchannels)
  {
    case(1): selection = DXGI_FORMAT_R8_UNORM;       break;
    case(4): selection = DXGI_FORMAT_R8G8B8A8_UNORM; break;
    default: selection = DXGI_FORMAT_UNKNOWN;        break;
  }
  ASSERT( (selection != DXGI_FORMAT_UNKNOWN), "ERROR: Unsupported format selected.");
  return selection;
}


// TODO: Call this in render_init() Also disable clockwise ordering since who cares.
internal void rasterizer_init()
{
  // Check out Frank Luna's RenderStates.cpp
  D3D11_RASTERIZER_DESC rasterDesc = {};
  rasterDesc.FillMode              = D3D11_FILL_SOLID;
  rasterDesc.CullMode              = D3D11_CULL_BACK;  // Disable backface culling
  // rasterDesc.CullMode              = D3D11_CULL_NONE;  // Disable backface culling
  rasterDesc.FrontCounterClockwise = true;
  rasterDesc.DepthClipEnable       = true;
  renderer->device->CreateRasterizerState(&rasterDesc, &renderer->rasterizer_default);
  // Call this before you draw whatever you want
  renderer->context->RSSetState(renderer->rasterizer_default);
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

  // Create blend state for alpha transparency
  D3D11_BLEND_DESC blend_desc = {};
  blend_desc.AlphaToCoverageEnable  = false;
  blend_desc.IndependentBlendEnable = false;
  blend_desc.RenderTarget[0].BlendEnable = true;
  blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
  blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
  blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
  blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
  blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
  blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
  blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

  success = renderer->device->CreateBlendState(&blend_desc, &renderer->blend_state);
  ASSERT(SUCCEEDED(success), "Failed to create blend state.");

  // Enable alpha blending
  f32 blend_factor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  renderer->context->OMSetBlendState(renderer->blend_state, blend_factor, 0xffffffff);

  // Set default rasterizer and create every kind you need
  rasterizer_init();
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


rbuffer_ptr render_buffer_init(arena *a, buffer_type t, void* data, u32 stride, u32 byte_count)
{
  render_buffer *out = arena_push_struct(a, render_buffer);
  out->stride = stride;
  out->offset = 0;
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
  return out;
}


rbuffer_ptr render_buffer_dynamic_init(arena *a, buffer_type t, void *data, u32 stride, u32 byte_count)
{
  // Initialize output in arena
  render_buffer *out = arena_push_struct(a, render_buffer);
  out->stride = stride;
  out->offset = 0;
  // Describe the buffer
  D3D11_BUFFER_DESC desc  = {};
  desc.ByteWidth          = byte_count;   // for constant buffers: multiple of 16 bytes
  desc.Usage              = D3D11_USAGE_DYNAMIC;    // we’ll update it frequently
  desc.BindFlags          = D3D11_BIND_VERTEX_BUFFER;
  desc.CPUAccessFlags     = D3D11_CPU_ACCESS_WRITE; // CPU can write
  desc.MiscFlags          = 0;
  desc.StructureByteStride = 0;
  // Create the buffer
  HRESULT hr = renderer->device->CreateBuffer(&desc, nullptr, &out->buffer);
  ASSERT(SUCCEEDED(hr), "Failed to create dynamic buffer.");
  return out;
}


rbuffer_ptr render_buffer_constant_init( arena *a, size_t byte_count )
{
  // Create buffer in memory arena
  render_buffer *out = arena_push_struct(a, render_buffer);
  out->stride = 0;
  out->offset = 0;
  // Describe the constant buffer
  D3D11_BUFFER_DESC description;
  description.Usage = D3D11_USAGE_DYNAMIC;
  description.ByteWidth = byte_count;
  description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  description.MiscFlags = 0;
  description.StructureByteStride = 0;
  // Create the buffer
  HRESULT hr = renderer->device->CreateBuffer( &description, 0, &out->buffer);
  ASSERT(SUCCEEDED(hr), "Failed to create camera constant buffer.");
  return out;
}


void render_constant_set( rbuffer_ptr b )
{
  renderer->context->VSSetConstantBuffers( 0, 1, &b->buffer );
}


void render_buffer_update(rbuffer_ptr b, void* data, u32 byte_count)
{
  D3D11_MAPPED_SUBRESOURCE mapped;
  HRESULT hr = renderer->context->Map(b->buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  ASSERT(SUCCEEDED(hr), "Failed to map buffer");
  // Copy 3 vertices into the buffer
  memcpy(mapped.pData, data, byte_count);
  renderer->context->Unmap(b->buffer, 0);
}


void render_text_init(arena *a)
{
  vert_texture quad[] = {
    //  clip-space xy for a full-screen-ish quad, z=0
    { -0.5f,  0.5f, 0.0f, 0.0f, 0.0f }, // TL
    {  0.5f,  0.5f, 0.0f, 1.0f, 0.0f }, // TR
    {  0.5f, -0.5f, 0.0f, 1.0f, 1.0f }, // BR
    { -0.5f, -0.5f, 0.0f, 0.0f, 1.0f }, // BL
  };
  u32 idx[] = { 0,1,2, 0,2,3 };
  ID3D11Buffer *vb=nullptr, *ib=nullptr;

  D3D11_BUFFER_DESC bd = {};
  bd.Usage = D3D11_USAGE_IMMUTABLE;
  bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  bd.ByteWidth = (UINT)sizeof(quad);
  D3D11_SUBRESOURCE_DATA s = { quad, 0, 0 };
  renderer->device->CreateBuffer(&bd, &s, &vb);

  bd = {};
  bd.Usage = D3D11_USAGE_IMMUTABLE;
  bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
  bd.ByteWidth = (UINT)sizeof(idx);
  s = { idx, 0, 0 };
  renderer->device->CreateBuffer(&bd, &s, &ib);

  // Create shaders
  ID3DBlob* vs_blob = nullptr;
  ID3DBlob* ps_blob = nullptr;
  ID3DBlob* erro = nullptr;
  HRESULT hr = D3DCompileFromFile(
    L"shaders/text.hlsl",
    nullptr,
    nullptr,
    "VSMain",
    "vs_5_0",
    D3DCOMPILE_ENABLE_STRICTNESS,
    0,
    &vs_blob,
    &erro
  );
  hr = D3DCompileFromFile(
    L"shaders/text.hlsl",
    nullptr,
    nullptr,
    "PSMain",
    "ps_5_0",
    D3DCOMPILE_ENABLE_STRICTNESS,
    0,
    &ps_blob,
    &erro
  );

  // Input layout (matches HLSL)
  D3D11_INPUT_ELEMENT_DESC il[] = {
      { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  };
  // Create after compiling VS to get vsBlob
  ID3D11InputLayout* layout=nullptr;
  renderer->device->CreateInputLayout(
    il,
    _countof(il),
    vs_blob->GetBufferPointer(),
    vs_blob->GetBufferSize(),
    &layout
  );

}


void render_draw(rbuffer_ptr vbuffer, shaders_ptr s, u32 count)
{
  renderer->context->IASetVertexBuffers(0, 1, &vbuffer->buffer, &vbuffer->stride, &vbuffer->offset);
  renderer->context->IASetInputLayout(s->vertex_in);
  renderer->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  renderer->context->VSSetShader(s->vertex, 0, 0);
  renderer->context->PSSetShader(s->pixel, 0, 0);
  // renderer->context->RSSetState(renderer->rasterizer_default);
  renderer->context->Draw(count, 0);
}


void render_draw_elems(rbuffer_ptr vbuffer, rbuffer_ptr ebuffer, shaders_ptr s, u32 count, u32 elem_start, u32 vert_start)
{
  renderer->context->IASetVertexBuffers(0, 1, &vbuffer->buffer, &vbuffer->stride, &vbuffer->offset);
  renderer->context->IASetIndexBuffer(ebuffer->buffer, DXGI_FORMAT_R32_UINT, 0);
  renderer->context->IASetInputLayout(s->vertex_in);
  renderer->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
  renderer->context->VSSetShader(s->vertex, 0, 0);
  renderer->context->PSSetShader(s->pixel, 0, 0);
  // renderer->context->RSSetState(renderer->rasterizer_default);
  renderer->context->DrawIndexed(count, elem_start, vert_start);
}



void render_close()
{
  renderer->blend_state->Release();
  renderer->render_target->Release();
  renderer->swapchain->Release();
  renderer->context->Release();
  renderer->device->Release();
}


texture2d_ptr texture2d_init(arena *a, void* pixels, i32 width, i32 height, i32 channels)
{
  texture2d *tex = arena_push_struct(a, texture2d);
  // Select format
  DXGI_FORMAT form = format_select(channels);
  // Create texture description
  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width = width;
  desc.Height = height;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.Format = form;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Usage = D3D11_USAGE_IMMUTABLE;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags = 0;
  desc.MiscFlags = 0;

  // Setup subresource data
  D3D11_SUBRESOURCE_DATA gpu_data = {};
  gpu_data.pSysMem = pixels;
  gpu_data.SysMemPitch = width * channels; // bytes per row

  // Create texture
  HRESULT hr = renderer->device->CreateTexture2D(&desc, &gpu_data, &tex->texture);
  ASSERT(SUCCEEDED(hr), "Failed to create texture2D.");

  // Create shader resource view
  D3D11_SHADER_RESOURCE_VIEW_DESC gpu_desc = {};
  gpu_desc.Format = desc.Format;
  gpu_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  gpu_desc.Texture2D.MostDetailedMip = 0;
  gpu_desc.Texture2D.MipLevels = 1;

  hr = renderer->device->CreateShaderResourceView(tex->texture, &gpu_desc, &tex->view);
  ASSERT(SUCCEEDED(hr), "Failed to create shader resource view.");

  // Create sampler state
  D3D11_SAMPLER_DESC sampler_desc = {};
  sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampler_desc.MipLODBias = 0.0f;
  sampler_desc.MaxAnisotropy = 1;
  sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
  sampler_desc.BorderColor[0] = 0.0f;
  sampler_desc.BorderColor[1] = 0.0f;
  sampler_desc.BorderColor[2] = 0.0f;
  sampler_desc.BorderColor[3] = 0.0f;
  sampler_desc.MinLOD = 0.0f;
  sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
  hr = renderer->device->CreateSamplerState(&sampler_desc, &tex->sampler);
  ASSERT(SUCCEEDED(hr), "Failed to create sampler state.");
  return tex;
}



void texture2d_bind(texture2d *tex, u32 slot)
{
  renderer->context->PSSetShaderResources(slot, 1, &tex->view);
  renderer->context->PSSetSamplers(slot, 1, &tex->sampler);
}


texture3d* texture3d_init(arena *a, void* data, i32 width, i32 height, i32 depth)
{
  texture3d *tex = arena_push_struct(a, texture3d);

  // Create texture description for R8 format (single channel, 8-bit)
  D3D11_TEXTURE3D_DESC desc = {};
  desc.Width = width;
  desc.Height = height;
  desc.Depth = depth;
  desc.MipLevels = 1;
  desc.Format = DXGI_FORMAT_R8_UNORM;  // Single channel, normalized 0-1
  desc.Usage = D3D11_USAGE_IMMUTABLE;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags = 0;
  desc.MiscFlags = 0;

  // Setup subresource data
  D3D11_SUBRESOURCE_DATA gpu_data = {};
  gpu_data.pSysMem = data;
  gpu_data.SysMemPitch = width;           // bytes per row
  gpu_data.SysMemSlicePitch = width * height;  // bytes per 2D slice

  // Create texture
  HRESULT hr = renderer->device->CreateTexture3D(&desc, &gpu_data, &tex->texture);
  ASSERT(SUCCEEDED(hr), "Failed to create texture3D.");

  // Create shader resource view
  D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
  srv_desc.Format = desc.Format;
  srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
  srv_desc.Texture3D.MostDetailedMip = 0;
  srv_desc.Texture3D.MipLevels = 1;

  hr = renderer->device->CreateShaderResourceView(tex->texture, &srv_desc, &tex->view);
  ASSERT(SUCCEEDED(hr), "Failed to create 3D texture shader resource view.");

  // Create sampler state with NEAREST filtering (for voxel-like data)
  D3D11_SAMPLER_DESC sampler_desc = {};
  sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;  // NEAREST
  sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampler_desc.MipLODBias = 0.0f;
  sampler_desc.MaxAnisotropy = 1;
  sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
  sampler_desc.MinLOD = 0.0f;
  sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

  hr = renderer->device->CreateSamplerState(&sampler_desc, &tex->sampler);
  ASSERT(SUCCEEDED(hr), "Failed to create 3D texture sampler state.");

  return tex;
}


void texture3d_bind(texture3d *tex, u32 slot)
{
  renderer->context->PSSetShaderResources(slot, 1, &tex->view);
  renderer->context->PSSetSamplers(slot, 1, &tex->sampler);
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
  if (erro)
  {
    char* error_msg = (char*)erro->GetBufferPointer();
    printf("Shader compilation error:\n%s\n", error_msg);
    erro->Release();
  }
  ASSERT(SUCCEEDED(hr), "Failed to compile shader from file.");
  // Create shader object
  switch (t)
  {
    case (VERTEX):
    {
      renderer->device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &s->vertex);
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

