
#include <windows.h>

struct platform_window
{
  HWND handle;
  HINSTANCE instance;
  f32 width;
  f32 height;
};


struct clock
{
  f64 secs_per_frame; // The amount of time between frames (seconds).
  f64 ticks_per_sec;  // The frequencey of the performance counter which is fixed at start up and constant (Hz).
  i64 curr;           // The current value of the counter at the start of the frame (ticks).
  i64 prev;           // The value of the counter at the start of the previous frame (ticks).
  f64 delta;          // The amount of seconds that have passed from the beginning of the frame to the end (secs).
};


enum control_bindings
{
  action1,
  action2,
  action3,
  control_count,
};

enum control_state
{
  up,
  pressed,
  down,
  released,
};


void platform_control_set(i32 *input_map)
{
  // TODO: How to make a function that listens for next input and sets this map to that key.
  // TODO: I should write the key bindings to a file and read from that, if it doesn't exist make it when starting up.
  input_map[action1] = WM_LBUTTONDOWN;
  input_map[action2] = WM_RBUTTONDOWN;
  input_map[action3] = 'A';
}


static const char* msg_name(UINT m) {
    switch (m) {
    case WM_LBUTTONDOWN:    return "WM_LBUTTONDOWN";
    case WM_LBUTTONUP:      return "WM_LBUTTONUP";
    case WM_LBUTTONDBLCLK:  return "WM_LBUTTONDBLCLK";
    case WM_NCLBUTTONDOWN:  return "WM_NCLBUTTONDOWN";
    case WM_NCLBUTTONUP:    return "WM_NCLBUTTONUP";
    case WM_NCLBUTTONDBLCLK:return "WM_NCLBUTTONDBLCLK";
    default:                return nullptr;
    }
}


internal LRESULT CALLBACK win32_message_callback(HWND window_handle, UINT message_id, WPARAM param_w, LPARAM param_l)
{
  LRESULT result = 0;
  // u32 vkcode = (u32) param_w;
  switch (message_id) 
  {
    case WM_CLOSE:
    {
      is_running = false;
      PostQuitMessage(0);
      break;
    }
    case WM_SIZE: 
    {
      // Save the new width and height of the client area. 
      // dwClientX = LOWORD(lParam); 
      // dwClientY = HIWORD(lParam); 
    }
    default: 
    {
      result = DefWindowProcA(window_handle, message_id, param_w, param_l); 
    }
  }
  return result;
}


void message_process(HWND handle, i32 *input_map)
{
  MSG message = {};
  // This has to be in condition otherwise you'll process the message twice.
  while (PeekMessageA(&message, handle, 0, 0, PM_REMOVE))
  {
    u32 vkcode = (u32) message.wParam;
    // const char* name = msg_name(message.message);
    u32 message_id = message.message;
    switch (message_id)
    {
      case(WM_LBUTTONDOWN):
      {
        if (WM_LBUTTONDOWN == input_map[action1])
        {
          handle_input();
          POINTS p = MAKEPOINTS(message.lParam);
          printf("%s time=%lu pos=(%d,%d)\n", "Left mouse click", message.time, p.x, p.y);
        }
        else if (WM_LBUTTONDOWN == input_map[action2])
        {
          printf("Right click\n");
        }
        else if (WM_LBUTTONDOWN == input_map[action3])
        {
          printf("Action 3\n");
        }
      }
      case(WM_KEYDOWN):
      {
        if (vkcode == input_map[action1])
        {
          POINTS p = MAKEPOINTS(message.lParam);
          printf("%s time=%lu pos=(%d,%d)\n", "Left mouse click", message.time, p.x, p.y);
        }
        else if (vkcode == input_map[action2])
        {
          printf("Right click\n");
        }
        else if (vkcode == input_map[action3])
        {
          printf("Action 3\n");
        }
        break;
      }
    }
    // This section basically sends the message to the loop we setup with our window. (win32_message_procedure_ansi)
    TranslateMessage(&message); // turn keystrokes into characters
    DispatchMessageA(&message); // tell OS to call window procedure
  }
}


void window_init(platform_window *wind)
{
  wind->instance = GetModuleHandleA(0);
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
  window_class.hInstance = wind->instance;
  window_class.hCursor = LoadCursor(0, IDC_ARROW);
  // window_class.hbrBackground = CreateSolidBrush(RGB(255, 0, 255)); // Magenta background
  window_class.hbrBackground = (HBRUSH)COLOR_WINDOW;
  window_class.lpszClassName = "WINDOW_CLASS";
  ATOM window_id = RegisterClassExA(&window_class);
  ASSERT(window_id, "ERROR: Failed to register window.");
  
  // Create the window we described.
  wind->handle = CreateWindowExA(
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
    wind->instance,      // window_handle: handle to this window's module.
    0                          // window_data_pointer: Pointer to CREATESTRUCT var that sends a message to the window.
  );
  ASSERT(wind->handle, "ERROR: Failed to create window.");

  // Get monitor info and resize the window.
  MONITORINFO monitor_info = {};
  monitor_info.cbSize = sizeof(MONITORINFO);
  HMONITOR monitor_handle = MonitorFromWindow(wind->handle, MONITOR_DEFAULTTOPRIMARY);
  BOOL monitor_info_success = GetMonitorInfoA(monitor_handle, &monitor_info);
  ASSERT(monitor_info_success, "ERROR: Failed to get monitor info.");
  i32 monitor_width  = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
  i32 monitor_height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;
  // Calculate our desired window dimensions.
  // Get DPI scaling factor
  u32 dpi = GetDpiForWindow(wind->handle);
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
    wind->handle,
    HWND_TOP,
    window_x,
    window_y,
    window_w,
    window_h,
    0
  );
  ASSERT(window_success, "Failed to resize and open window.");
}


void window_size_get(platform_window *wind)
{
  // Get window dimension
  RECT size;
  GetClientRect(wind->handle, &size);
  wind->width  = size.right;
  wind->height = size.bottom;
  if ((wind->width == 0) && (wind->height == 0))
  {
    printf("minimized\n");
  }
}


void * platform_memory_alloc(void *mem_base, size_t mem_size)
{
  void *memory = VirtualAlloc(mem_base, mem_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  ASSERT(memory, "ERROR: Unable to allocate application memory.");
  return memory;
}


int file_exists(const char *filepath)
{
  int exists = 0;
  FILE *file = fopen(filepath, "r");
  if (file) {
    fclose(file);
    exists =  1;
  }
  return exists;
}


/// @brief 
/// @param file 
/// @param scratch 
/// @return 
/// @note If reading text files you have to add a \0 char.
const char * read_file(const char *file, arena *scratch, size_t *out_size)
{
  FILE *stream;
  char *contents = 0;
  fopen_s(&stream, file, "rb");
  ASSERT(stream, "ERROR: Failed to read file.");
  fseek(stream, 0, SEEK_END);
  *out_size = ftell(stream);
  contents = (char*) arena_alloc(scratch, *out_size);
  fseek(stream, 0, SEEK_SET);
  size_t bytes_read = fread(contents, 1, *out_size, stream);
  bool8 success = bytes_read == *out_size;
  ASSERT(success, "ERROR: Read incorrect number of bytes from file.");
  fclose(stream);
  return contents;
}


i64 clock_time()
{
  LARGE_INTEGER temp;
  QueryPerformanceCounter(&temp);
  return temp.QuadPart;
}


clock clock_init(f64 fps_target)
{
  clock c = {};
  c.secs_per_frame = 1.0 / fps_target;
  LARGE_INTEGER temp;
  QueryPerformanceFrequency(&temp);
  c.ticks_per_sec = (f64) temp.QuadPart;
  c.prev = clock_time();
  return c;
}


void clock_update(clock *c)
{
  c->curr = clock_time();
  // The amount of ticks that have passed from the beginning of the frame to the end (ticks).
  // TODO: Should this be its own function?
  f64 delta_ticks = (f64) (c->curr - c->prev);
  c->delta = delta_ticks / c->ticks_per_sec;
  // TODO: End wall clock diff
  c->prev = c->curr;
}
