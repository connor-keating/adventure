// Source code
#include "render.h"

// External code
#include <d3d11.h>
#include <dxgi.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>

#if defined(_DEBUG)
#pragma comment(lib, "dxguid.lib")
#endif

/*
1. Create array of ID3D11RasterizerState variables. It can be global and made in the init function.
*/

struct shaders
{
  ID3D11VertexShader *vertex;
  ID3D11InputLayout  *vertex_in;
  ID3D11PixelShader  *pixel;
};

struct rbuffer
{
  ID3D11Buffer* buffer;
  u32 stride;
  u32 offset;
};

struct texture
{
  texture_dimension dim;
  void* texture;
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
  ID3D11DepthStencilState* depth_stencil_enabled;
  ID3D11DepthStencilState* depth_stencil_disabled;
};

struct render_data
{
  shaders *s; // Array of shaders. Length set by app.
  u64 shader_first_available;
};

global render_state *renderer;
global render_data  *rdata;


#if defined(_DEBUG)
// Info queue that stores debug messages
global ID3D11InfoQueue* info_queue;

// Print all D3D11 debug messages from the info queue
internal void debug_print()
{
  if (!info_queue) return;
  u64 num_messages = info_queue->GetNumStoredMessages();
  for (u64 i = 0; i < num_messages; i++)
  {
    SIZE_T message_length = 0;
    info_queue->GetMessage(i, nullptr, &message_length);
    D3D11_MESSAGE* message = (D3D11_MESSAGE*)malloc(message_length);
    info_queue->GetMessage(i, message, &message_length);
    const char* severity_str = "UNKNOWN";
    switch (message->Severity)
    {
      case D3D11_MESSAGE_SEVERITY_CORRUPTION: severity_str = "CORRUPTION"; break;
      case D3D11_MESSAGE_SEVERITY_ERROR:      severity_str = "ERROR";      break;
      case D3D11_MESSAGE_SEVERITY_WARNING:    severity_str = "WARNING";    break;
      case D3D11_MESSAGE_SEVERITY_INFO:       severity_str = "INFO";       break;
      case D3D11_MESSAGE_SEVERITY_MESSAGE:    severity_str = "MESSAGE";    break;
    }
    char msg[256];
    snprintf( msg, 256, "[D3D11 %s] %s\n", severity_str, message->pDescription );
    printf( "%s", msg );
    free(message);
  }
  info_queue->ClearStoredMessages();
}

// Report live DXGI/D3D11 objects (resource leaks)
internal void debug_objects()
{
  IDXGIDebug* dxgi_debug = nullptr;
  typedef HRESULT(WINAPI* DXGIGetDebugInterfaceFunc)(REFIID, void**);
  HMODULE dxgi_debug_dll = LoadLibraryA("dxgidebug.dll");
  if (dxgi_debug_dll)
  {
    DXGIGetDebugInterfaceFunc DXGIGetDebugInterface_func =
      (DXGIGetDebugInterfaceFunc)GetProcAddress(dxgi_debug_dll, "DXGIGetDebugInterface");

    if (DXGIGetDebugInterface_func)
    {
      HRESULT hr = DXGIGetDebugInterface_func(__uuidof(IDXGIDebug), (void**)&dxgi_debug);
      if (SUCCEEDED(hr) && dxgi_debug)
      {
        printf("\n=== DXGI Live Objects Report ===\n");
        dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
        dxgi_debug->Release();
      }
    }
    FreeLibrary(dxgi_debug_dll);
  }
}
#endif


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


internal u32 buffer_type_get( buffer_type t )
{
  u32 flags = 0;
  switch (t)
  {
    case (BUFF_VERTS): flags = D3D11_BIND_VERTEX_BUFFER;    break;
    case (BUFF_ELEMS): flags = D3D11_BIND_INDEX_BUFFER;     break;
    case (BUFF_CONST): flags = D3D11_BIND_CONSTANT_BUFFER;  break;
    default: break;
  };
  ASSERT( (flags != 0), "Failed to set D3D11 flags.");
  return flags;
}


internal void rasterizer_init()
{
  // Check out Frank Luna's RenderStates.cpp
  // Default rasterizer state
  D3D11_RASTERIZER_DESC rasterDesc = {};
  rasterDesc.FillMode              = D3D11_FILL_SOLID;
  rasterDesc.CullMode              = D3D11_CULL_BACK;
  // rasterDesc.CullMode              = D3D11_CULL_NONE;  // Disable backface culling
  rasterDesc.FrontCounterClockwise = true;
  rasterDesc.DepthClipEnable       = true;
  renderer->device->CreateRasterizerState(&rasterDesc, &renderer->rasterizer_default);
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

  #if defined(_DEBUG)
  {
    result = renderer->device->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&info_queue);
    ASSERT( SUCCEEDED(result), "Failed to create info queue for debugging." );

    // Break on D3D11 errors (will trigger debugger breakpoint)
    // Comment these out to just print errors without crashing
    // info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    // info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
    // OPTIONAL: break on warnings (noisy)
    // info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, TRUE);

    // Filter spammy messages (optional)
    D3D11_MESSAGE_ID hide[] = {
        D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
        D3D11_MESSAGE_ID_DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET
    };
    D3D11_INFO_QUEUE_FILTER filter = {};
    filter.DenyList.NumIDs = _countof(hide);
    filter.DenyList.pIDList = hide;
    info_queue->AddStorageFilterEntries(&filter);
  }
  #endif
  
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
  // Create all depth stencil states
  // Depth stencil state with depth testing enabled
  D3D11_DEPTH_STENCIL_DESC depthDesc = {};
  depthDesc.DepthEnable    = TRUE;
  depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
  depthDesc.DepthFunc      = D3D11_COMPARISON_LESS;
  depthDesc.StencilEnable  = FALSE;
  renderer->device->CreateDepthStencilState(&depthDesc, &renderer->depth_stencil_enabled);
  renderer->context->OMSetDepthStencilState(renderer->depth_stencil_enabled, 0);
  // Depth stencil state with depth testing disabled (for UI)
  depthDesc.DepthEnable = FALSE;
  renderer->device->CreateDepthStencilState(&depthDesc, &renderer->depth_stencil_disabled);

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


void render_data_init( arena *a, u64 shader_count )
{
  // TODO: Moving shaders into render_data. shaders load will return the index
  rdata = arena_push_struct( a, render_data);
  rdata->s = arena_push_array( a, shader_count, shaders );
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


rbuffer* rbuffer_init(arena *a, buffer_type t, void* data, u32 stride, u32 byte_count)
{
  rbuffer *out = arena_push_struct(a, rbuffer);
  out->stride = stride;
  out->offset = 0;
  // Determine bind flags based on buffer type
  u32 flags = buffer_type_get(t);
  D3D11_BUFFER_DESC vbd;
  vbd.Usage = D3D11_USAGE_IMMUTABLE;
  vbd.ByteWidth = byte_count;
  vbd.BindFlags = flags;
  vbd.CPUAccessFlags = 0;
  vbd.MiscFlags = 0;
  D3D11_SUBRESOURCE_DATA vinitData;
  vinitData.pSysMem = data;
  HRESULT hr = renderer->device->CreateBuffer(&vbd, &vinitData, &out->buffer);
  ASSERT( SUCCEEDED(hr), "Failed to create buffer." );
  return out;
}


void rbuffer_close( rbuffer* b)
{
  b->buffer->Release();
}


rbuffer* rbuffer_dynamic_init(arena *a, buffer_type t, void *data, u32 stride, u32 byte_count)
{
  // Initialize output in arena
  rbuffer *out = arena_push_struct(a, rbuffer);
  out->stride = stride;
  out->offset = 0;
  // Determine bind flags based on buffer type
  u32 flags = buffer_type_get(t);
  // Describe the buffer
  D3D11_BUFFER_DESC desc  = {};
  desc.ByteWidth          = byte_count;   // for constant buffers: multiple of 16 bytes
  desc.Usage              = D3D11_USAGE_DYNAMIC;    // we'll update it frequently
  desc.BindFlags          = flags;
  desc.CPUAccessFlags     = D3D11_CPU_ACCESS_WRITE; // CPU can write
  desc.MiscFlags          = 0;
  desc.StructureByteStride = 0;
  // Create the buffer
  HRESULT hr = renderer->device->CreateBuffer(&desc, nullptr, &out->buffer);
  ASSERT(SUCCEEDED(hr), "Failed to create dynamic buffer.");
  return out;
}


void render_constant_set( rbuffer* b, u32 slot )
{
  // Shared buffer for both shaders
  renderer->context->VSSetConstantBuffers( slot, 1, &b->buffer );
  renderer->context->PSSetConstantBuffers( slot, 1, &b->buffer );
}


void rbuffer_update(rbuffer* b, void* data, u32 byte_count)
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


void render_draw(rbuffer* vbuffer, shaders* s, u32 count)
{
  renderer->context->IASetVertexBuffers(0, 1, &vbuffer->buffer, &vbuffer->stride, &vbuffer->offset);
  renderer->context->IASetInputLayout(s->vertex_in);
  renderer->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  renderer->context->VSSetShader(s->vertex, 0, 0);
  renderer->context->PSSetShader(s->pixel, 0, 0);
  // renderer->context->RSSetState(renderer->rasterizer_default);
  renderer->context->Draw(count, 0);
}


void render_draw_elems(rbuffer* vbuffer, rbuffer* ebuffer, u64 shader_index, u32 count, u32 elem_start, u32 vert_start)
{
  shaders s = rdata->s[shader_index];
  renderer->context->IASetVertexBuffers(0, 1, &vbuffer->buffer, &vbuffer->stride, &vbuffer->offset);
  renderer->context->IASetIndexBuffer(ebuffer->buffer, DXGI_FORMAT_R32_UINT, 0);
  renderer->context->IASetInputLayout(s.vertex_in);
  renderer->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  renderer->context->VSSetShader(s.vertex, 0, 0);
  renderer->context->PSSetShader(s.pixel, 0, 0);
  // renderer->context->RSSetState(renderer->rasterizer_default);
  renderer->context->DrawIndexed(count, elem_start, vert_start);
}


void render_draw_ui( rbuffer* vbuffer, shaders* s, u32 count )
{
  renderer->context->OMSetDepthStencilState(renderer->depth_stencil_disabled, 0);
  render_draw( vbuffer, s, count);
  renderer->context->OMSetDepthStencilState(renderer->depth_stencil_enabled, 0);
}


void render_draw_ui_elems(rbuffer* vbuffer, rbuffer* ebuffer, u64 shader_index, u32 count, u32 elem_start, u32 vert_start)
{
  renderer->context->OMSetDepthStencilState(renderer->depth_stencil_disabled, 0);
  render_draw_elems( vbuffer, ebuffer, shader_index, count, elem_start, vert_start );
  renderer->context->OMSetDepthStencilState(renderer->depth_stencil_enabled, 0);
}


void render_close()
{
  renderer->context->ClearState();
  renderer->render_target->Release();
  renderer->depth_view->Release();
  renderer->depth_buffer->Release();
  renderer->blend_state->Release();
  renderer->rasterizer_default->Release();
  renderer->depth_stencil_enabled->Release();
  renderer->depth_stencil_disabled->Release();
  renderer->context->Release();
  renderer->swapchain->Release();
  #if defined(_DEBUG)
    if (info_queue) info_queue->Release();
  #endif
  renderer->device->Release();

  #if defined(_DEBUG)
    debug_objects();
    debug_print();
  #endif
}


texture* texture1d_init(arena *a, void* data, i32 width)
{
  texture *tex = arena_push_struct(a, texture);
  tex->dim = ONE;
  ID3D11Texture1D **actual = (ID3D11Texture1D**) &tex->texture;

  // Transfer functions are RGBA (4 channels)
  DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

  // Create texture description
  D3D11_TEXTURE1D_DESC desc = {};
  desc.Width = width;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.Format = format;
  desc.Usage = D3D11_USAGE_IMMUTABLE;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags = 0;
  desc.MiscFlags = 0;

  // Setup subresource data
  D3D11_SUBRESOURCE_DATA gpu_data = {};
  gpu_data.pSysMem = data;
  gpu_data.SysMemPitch = width * 4; // RGBA = 4 bytes per pixel

  // Create texture
  HRESULT hr = renderer->device->CreateTexture1D(&desc, &gpu_data, actual);
  ASSERT(SUCCEEDED(hr), "Failed to create texture1D.");

  // Create shader resource view
  D3D11_SHADER_RESOURCE_VIEW_DESC view_desc = {};
  view_desc.Format = desc.Format;
  view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
  view_desc.Texture1D.MostDetailedMip = 0;
  view_desc.Texture1D.MipLevels = 1;

  hr = renderer->device->CreateShaderResourceView( *actual, &view_desc, &tex->view );
  ASSERT(SUCCEEDED(hr), "Failed to create shader resource view for texture1D.");

  // Create sampler state (linear filtering for smooth gradients)
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
  ASSERT(SUCCEEDED(hr), "Failed to create sampler state for texture1D.");

  return tex;
}


texture* texture2d_init(arena *a, void* pixels, i32 width, i32 height, i32 channels)
{
  texture *tex = arena_push_struct(a, texture);
  tex->dim = TWO;
  ID3D11Texture2D **actual = (ID3D11Texture2D**) &tex->texture;
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
  HRESULT hr = renderer->device->CreateTexture2D(&desc, &gpu_data, actual );
  ASSERT(SUCCEEDED(hr), "Failed to create texture2D.");

  // Create shader resource view
  D3D11_SHADER_RESOURCE_VIEW_DESC gpu_desc = {};
  gpu_desc.Format = desc.Format;
  gpu_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  gpu_desc.Texture2D.MostDetailedMip = 0;
  gpu_desc.Texture2D.MipLevels = 1;

  hr = renderer->device->CreateShaderResourceView( *actual, &gpu_desc, &tex->view );
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


void texture_close( texture *tex )
{
  switch (tex->dim)
  {
    case (ONE):
    {
      ID3D11Texture1D *temp = (ID3D11Texture1D*) tex->texture;
      temp->Release();
      break;
    }
    case (TWO):
    {
      ID3D11Texture2D *temp = (ID3D11Texture2D*) tex->texture;
      temp->Release();
      break;
    }
    case (THREE):
    {
      ID3D11Texture3D *temp = (ID3D11Texture3D*) tex->texture;
      temp->Release();
      break;
    }
    default: ASSERT(false, "Unrecognized texture shape.\n"); break;
  };
  tex->view->Release();
  tex->sampler->Release();
}


texture* texture3d_init(arena *a, void* data, i32 width, i32 height, i32 depth)
{
  texture *tex = arena_push_struct(a, texture);
  tex->dim = THREE;
  ID3D11Texture3D **actual = (ID3D11Texture3D**) &tex->texture;

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
  HRESULT hr = renderer->device->CreateTexture3D( &desc, &gpu_data, actual );
  ASSERT(SUCCEEDED(hr), "Failed to create texture3D.");

  // Create shader resource view
  D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
  srv_desc.Format = desc.Format;
  srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
  srv_desc.Texture3D.MostDetailedMip = 0;
  srv_desc.Texture3D.MipLevels = 1;

  hr = renderer->device->CreateShaderResourceView( *actual, &srv_desc, &tex->view);
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


void texture_bind(texture *tex, u32 slot)
{
  renderer->context->PSSetShaderResources(slot, 1, &tex->view);
  renderer->context->PSSetSamplers(slot, 1, &tex->sampler);
}


u64 shader_init(arena *a)
{
  // Init output
  u64 available = rdata->shader_first_available;
  rdata->shader_first_available++;
  return available;
}


void shader_load( u64 shader_index, shader_type t, const char *file, const char *entry, const char *target)
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
  shaders *s = &rdata->s[shader_index];
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


void shader_close( shaders *s )
{
  s->vertex->Release();
  s->vertex_in->Release();
  s->pixel->Release();
}

void frame_init(f32 *background_color)
{
  renderer->context->ClearRenderTargetView(renderer->render_target, background_color);
  renderer->context->ClearDepthStencilView(renderer->depth_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}


void frame_render()
{
  #if defined(_DEBUG)
  debug_print();
  #endif
  renderer->swapchain->Present(1, 0); // vsync on
}

