
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
  render_state state = {};
  return &state;
}


void render_close(render_state *state)
{
  printf("Closing renderer.\n");
}
