
#include "core.cpp"

#include <windows.h>
#include <stdio.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>


global bool is_running;


internal LRESULT CALLBACK win32_message_callback(HWND window_handle, UINT message_id, WPARAM param_w, LPARAM param_l)
{
  LRESULT result = 0;
  switch (message_id) 
  {
    case WM_CLOSE:
    {
      is_running = false;
      PostQuitMessage(0);
      break;
    }
    default: 
    {
      result = DefWindowProcA(window_handle, message_id, param_w, param_l); 
    }
  }
  return result;
}


void message_process(HWND handle)
{
  MSG message = {};
  bool32 message_current = true;
  while (message_current)
  {
    message_current = PeekMessageA(&message, handle, 0, 0, PM_REMOVE);
    // This section basically sends the message to the loop we setup with our window. (win32_message_procedure_ansi)
    TranslateMessage(&message); // turn keystrokes into characters
    DispatchMessageA(&message); // tell OS to call window procedure
  }
}


void window_init(HWND *handle, HINSTANCE *instance)
{
  *instance = GetModuleHandleA(0);
  i32 window_x = 50;
  i32 window_y = 90;
  i32 window_w = 50;
  i32 window_h = 50;
  const char *window_name = "Renderer";
  // Create win32 window class.
  WNDCLASSEXA window_class = {};
  window_class.cbSize = sizeof(window_class);
  window_class.style = CS_HREDRAW|CS_VREDRAW; // |CS_OWNDC;
  window_class.lpfnWndProc = win32_message_callback;
  window_class.hInstance = *instance;
  window_class.hCursor = LoadCursor(0, IDC_ARROW);
  // window_class.hbrBackground = CreateSolidBrush(RGB(255, 0, 255)); // Magenta background
  window_class.hbrBackground = (HBRUSH)COLOR_WINDOW;
  window_class.lpszClassName = "WINDOW_CLASS";
  ATOM window_id = RegisterClassExA(&window_class);
  ASSERT(window_id, "ERROR: Failed to register window.");
  
  // Create the window we described.
  *handle = CreateWindowExA(
    WS_EX_CLIENTEDGE,            // style_extended: has list of possible values.    
    window_class.lpszClassName,  // class_name: null-terminated string.         
    window_name,                  // name: string window name to display in title bar.              
    WS_OVERLAPPEDWINDOW,        // style_basic: has list of possible values.        
    window_x,                          // position_x: Horizontal window position      
    window_y,                          // position_y: Vertical window position         
    window_w,                          // width: Window width in screen coordinates
    window_h,                          // height: Window height in screen coordinates
    0,                          // window_parent: Handle to the parent window.
    0,                          // window_menu: Optional child window ID.
    *instance,      // window_handle: handle to this window's module.
    0                          // window_data_pointer: Pointer to CREATESTRUCT var that sends a message to the window.
  );
  ASSERT(handle, "ERROR: Failed to create window.");

  // Get monitor info and resize the window.
  MONITORINFO monitor_info = {};
  monitor_info.cbSize = sizeof(MONITORINFO);
  HMONITOR monitor_handle = MonitorFromWindow(*handle, MONITOR_DEFAULTTOPRIMARY);
  BOOL monitor_info_success = GetMonitorInfoA(monitor_handle, &monitor_info);
  ASSERT(monitor_info_success, "ERROR: Failed to get monitor info.");
  i32 monitor_width  = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
  i32 monitor_height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;
  // Calculate our desired window dimensions.
  // Get DPI scaling factor
  u32 dpi = GetDpiForWindow(*handle);
  f32 scale_factor = dpi / 96.0f; // Scaling factor (1.0 at 96 DPI, 1.5 at 144 DPI, etc.)
  i32 client_w = MulDiv(monitor_width, 1, 2 * scale_factor);
  i32 client_h = MulDiv(monitor_height, 1, 2 * scale_factor);
  RECT window_border = {0, 0, client_w, client_h};
  AdjustWindowRectExForDpi(&window_border, WS_OVERLAPPEDWINDOW, FALSE, 0, dpi);
  window_w = window_border.right - window_border.left;
  window_h = window_border.bottom - window_border.top;
  window_x = (monitor_width  - window_w) / 2;
  window_y = (monitor_height - window_h) / 2;
  BOOL window_success = SetWindowPos(
    *handle,
    HWND_TOP,
    window_x,
    window_y,
    window_w,
    window_h,
    0
  );
  ASSERT(window_success, "Failed to resize and open window.");
}


int main(int argc, char **argv)
{
  HWND handle;
  HINSTANCE instance;

  window_init(&handle, &instance);
  i32 display_flags = SW_SHOW;
  ShowWindow(handle, display_flags);
  UpdateWindow(handle);
  
  // Initialize D3D11
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

  is_running = true;
  while (is_running)
  {
    message_process(handle);

    // Render frame
    f32 color[4] = { 0.5f, 0.0f, 0.0f, 1.0f };
    context->ClearRenderTargetView(render_target, color);
    swapchain->Present(1, 0); // vsync on
  }

  // close and release all existing COM objects
  swapchain->Release();
  device->Release();
  context->Release();

  return 0;
}
