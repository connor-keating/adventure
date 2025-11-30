
#include "core.cpp"
#include "platform.h"

#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
  // Init memory
  // Allocate all program memory upfront.
  #if _DEBUG
    void *memory_base = (void*)Terabytes(2);
  #else
    void *memory_base = 0;
  #endif
  size_t memory_size = (size_t) Gigabytes(5);
  void *raw_memory = platform_memory_alloc(memory_base, memory_size);
  arena memory = arena_init(raw_memory, memory_size);
  // Open a window
  platform_init(&memory);
  platform_window window = platform_window_init();
  HWND *wind_handle = (HWND*) platform_window_handle();
  // Init d3d
  IDXGISwapChain* swapchain;
  ID3D11Device* device;
  ID3D11DeviceContext* devicecontext;
  ID3D11Texture2D* rendertarget;
  ID3D11RenderTargetView* rendertargetview;
  ID3DBlob* cso;
  ID3D11VertexShader* vertexshader;
  ID3D11PixelShader* pixelshader;
  DXGI_SWAP_CHAIN_DESC swapchaindesc = { { 0, 0, {}, DXGI_FORMAT_R8G8B8A8_UNORM }, { 1 }, 32, 2, *wind_handle, 1 };
  D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, 0, 0, 0, 7, &swapchaindesc, &swapchain, &device, 0, &devicecontext);
  swapchain->GetDesc(&swapchaindesc);
  swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&rendertarget);
  device->CreateRenderTargetView(rendertarget, 0, &rendertargetview);
  D3DCompileFromFile(L"shaders/minimal.hlsl", 0, 0, "vertex_shader", "vs_5_0", 0, 0, &cso, 0);
  device->CreateVertexShader(cso->GetBufferPointer(), cso->GetBufferSize(), 0, &vertexshader);
  D3DCompileFromFile(L"shaders/minimal.hlsl", 0, 0, "pixel_shader", "ps_5_0", 0, 0, &cso, 0);
  device->CreatePixelShader(cso->GetBufferPointer(), cso->GetBufferSize(), 0, &pixelshader);
  D3D11_VIEWPORT viewport = { 0, 0, (float)swapchaindesc.BufferDesc.Width, (float)swapchaindesc.BufferDesc.Height, 0, 1 };
  // Projection matrix creation
  float plane1 = -5.0f;
  float plane2 =  5.0f;
  // glm::mat4 projection = glm::mat4(1.0f);
  glm::mat4 projection = glm::ortho(
    plane1, plane2, // X
    plane1, plane2, // Y
    -0.1f, 100.f  // Z
  );
  ID3D11Buffer* buffer;
  D3D11_BUFFER_DESC desc   = {};
  desc.ByteWidth           = sizeof(projection);   // for constant buffers: multiple of 16 bytes
  desc.Usage               = D3D11_USAGE_IMMUTABLE;    // we'll update it frequently
  desc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
  desc.CPUAccessFlags      = 0; // CPU can write
  desc.MiscFlags           = 0;
  desc.StructureByteStride = 0;
  D3D11_SUBRESOURCE_DATA resource;
  resource.pSysMem = &projection;
  HRESULT hr = device->CreateBuffer(&desc, &resource, &buffer);
  devicecontext->VSSetConstantBuffers( 0, 1, &buffer );
  devicecontext->PSSetConstantBuffers( 0, 1, &buffer );
  // Open the window and start the app
  input_state inputs[KEY_COUNT];
  platform_window_show();
  while (platform_is_running())
  {
    platform_message_process( &window, inputs );
    if (inputs[KEY_ESCAPE] == INPUT_DOWN)
    {
      platform_window_close();
    }
    devicecontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    devicecontext->VSSetShader(vertexshader, 0, 0);
    devicecontext->RSSetViewports(1, &viewport);
    devicecontext->PSSetShader(pixelshader, 0, 0);
    devicecontext->OMSetRenderTargets(1, &rendertargetview, 0);
    devicecontext->Draw(3, 0);
    swapchain->Present(1, 0);
  }
}

