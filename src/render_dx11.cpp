
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

render_state render_init()
{
  render_state state = {};
  return state;
}
