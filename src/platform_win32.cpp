
#include "platform.h"

#include <windows.h>

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobj_loader_c.h"


struct platform_state
{
  HWND handle;
  HINSTANCE instance;
  bool is_running; 
};

enum control_bindings
{
  ACTION1,
  ACTION2,
  ACTION3,
  ACTION_COUNT,
};

enum control_state
{
  CONTROL_UP,
  CONTROL_HELD,
  CONTROL_DOWN,
  CONTROL_RELEASED,
};


// Internal global state
global platform_state state;


internal LRESULT CALLBACK win32_message_callback(HWND window_handle, UINT message_id, WPARAM param_w, LPARAM param_l)
{
  LRESULT result = 0;
  // u32 vkcode = (u32) param_w;
  switch (message_id) 
  {
    case WM_CLOSE:
    {
      state.is_running = false;
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


internal char* mmap_file(size_t* len, const char* filename) {
  HANDLE file = CreateFileA(
    filename,
    GENERIC_READ,
    FILE_SHARE_READ,
    NULL,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
    NULL
  );
  if (file == INVALID_HANDLE_VALUE) { /* E.g. Model may not have materials. */
    return NULL;
  }
  HANDLE fileMapping = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
  ASSERT(fileMapping != INVALID_HANDLE_VALUE, "ERROR: Failed to map file.");
  LPVOID fileMapView = MapViewOfFile(fileMapping, FILE_MAP_READ, 0, 0, 0);
  char* fileMapViewChar = (char*)fileMapView;
  ASSERT(fileMapView != NULL, "ERROR: Failed to create map view.");
  DWORD file_size = GetFileSize(file, NULL);
  (*len) = (size_t)file_size;
  return fileMapViewChar;
}


internal void get_file_data(void* ctx, const char* filename, const int is_mtl, const char* obj_filename, char** data, size_t* len)
{
  // NOTE: If you allocate the buffer with malloc(),
  // You can define your own memory management struct and pass it through `ctx`
  // to store the pointer and free memories at clean up stage(when you quit an
  // app)
  // This example uses mmap(), so no free() required.
  (void)ctx;
  if (!filename)
  {
    ASSERT(filename != NULL, "ERROR: Invalid file.");
    fprintf(stderr, "null filename\n");
    (*data) = NULL;
    (*len) = 0;
    return;
  }
  size_t data_len = 0;
  *data = mmap_file(&data_len, filename);
  (*len) = data_len;
}


void platform_status()
{
  printf("Library working\n");
}


platform_window platform_window_init()
{
  platform_window wind = {};
  state.instance = GetModuleHandleA(0);
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
  window_class.hInstance = state.instance;
  window_class.hCursor = LoadCursor(0, IDC_ARROW);
  // window_class.hbrBackground = CreateSolidBrush(RGB(255, 0, 255)); // Magenta background
  window_class.hbrBackground = (HBRUSH)COLOR_WINDOW;
  window_class.lpszClassName = "WINDOW_CLASS";
  ATOM window_id = RegisterClassExA(&window_class);
  ASSERT(window_id, "ERROR: Failed to register window.");
  
  // Create the window we described.
  state.handle = CreateWindowExA(
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
    state.instance,      // window_handle: handle to this window's module.
    0                          // window_data_pointer: Pointer to CREATESTRUCT var that sends a message to the window.
  );
  ASSERT(state.handle, "ERROR: Failed to create window.");

  // Get monitor info and resize the window.
  MONITORINFO monitor_info = {};
  monitor_info.cbSize = sizeof(MONITORINFO);
  HMONITOR monitor_handle = MonitorFromWindow(state.handle, MONITOR_DEFAULTTOPRIMARY);
  BOOL monitor_info_success = GetMonitorInfoA(monitor_handle, &monitor_info);
  ASSERT(monitor_info_success, "ERROR: Failed to get monitor info.");
  i32 monitor_width  = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
  i32 monitor_height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;
  // Calculate our desired window dimensions.
  // Get DPI scaling factor
  u32 dpi = GetDpiForWindow(state.handle);
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
    state.handle,
    HWND_TOP,
    window_x,
    window_y,
    window_w,
    window_h,
    0
  );
  ASSERT(window_success, "Failed to resize and open window.");
  wind.width = window_w;
  wind.height = window_h;
  wind.state = &state;
  return wind;
}


void platform_window_show()
{
  state.is_running = true;
  i32 display_flags = SW_SHOW;
  ShowWindow(state.handle, display_flags);
  UpdateWindow(state.handle);
}


bool platform_is_running()
{
  return state.is_running;
}


void platform_message_process(platform_window *window)
{
  MSG message = {};
  // This has to be in condition otherwise you'll process the message twice.
  while (PeekMessageA(&message, state.handle, 0, 0, PM_REMOVE))
  {
    // This section basically sends the message to the loop we setup with our window. (win32_message_procedure_ansi)
    TranslateMessage(&message); // turn keystrokes into characters
    DispatchMessageA(&message); // tell OS to call window procedure
  }
}


const char* control_state_log(control_state s) {
  switch (s) {
  case CONTROL_UP:       return "up";
  case CONTROL_HELD:  return "held";
  case CONTROL_DOWN:     return "down";
  case CONTROL_RELEASED: return "released";
  default:       return "unknown";
  }
}


void platform_control_set(i32 *input_map)
{
  // TODO: How to make a function that listens for next input and sets this map to that key.
  // TODO: I should write the key bindings to a file and read from that, if it doesn't exist make it when starting up.
  input_map[ACTION1] = WM_LBUTTONDOWN;
  input_map[ACTION2] = WM_RBUTTONDOWN;
  input_map[ACTION3] = 'A';
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


void platform_input_state_set(i32* input_map, control_state *input_state, i32 input, control_state state)
{
  const char *type = control_state_log(state);
  if (input_map[ACTION1] == input)
  {
    printf("Action 1, %s\n", type);
    input_state[ACTION1] = state;
  }
  else if (input_map[ACTION2] == input)
  {
    printf("Action 2, %s\n", type);
    input_state[ACTION2] = state;
  }
  else if (input_map[ACTION3] == input)
  {
    printf("Action 3, %s\n", type);
    input_state[ACTION3] = state;
  }
}


control_state mouse_input(i32 *input_map, control_state *input_state, i32 button)
{
  i32 i;
  for (i = 0; i < ACTION_COUNT; ++i)
  {
    if (input_map[i] == button) break;
  }
  // Get previous state
  control_state previous = input_state[i];
  if (previous == CONTROL_UP || previous == CONTROL_RELEASED)
  {
    return CONTROL_DOWN;
  }
  else
  {
    // TODO: Mouse input on windows doesn't send held messages, so I'll just have to know when to transition it.
    return CONTROL_HELD;
  }
}


void message_process(HWND handle, i32 *input_map, control_state *input_state)
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
        // POINTS p = MAKEPOINTS(message.lParam);
        // printf("%s time=%lu pos=(%d,%d)\n", "Left mouse click", message.time, p.x, p.y);
        i32 target = WM_LBUTTONDOWN;
        control_state s = mouse_input(input_map, input_state, target);
        platform_input_state_set(input_map, input_state, target, s);
        break;
      }
      case(WM_LBUTTONUP):
      {
        // POINTS p = MAKEPOINTS(message.lParam);
        // printf("%s time=%lu pos=(%d,%d)\n", "Left mouse release", message.time, p.x, p.y);
        platform_input_state_set(input_map, input_state, WM_LBUTTONDOWN, CONTROL_RELEASED);
        break;
      }
      case(WM_RBUTTONDOWN):
      {
        // POINTS p = MAKEPOINTS(message.lParam);
        // printf("%s time=%lu pos=(%d,%d)\n", "Right mouse click", message.time, p.x, p.y);
        i32 target = WM_RBUTTONDOWN;
        control_state s = mouse_input(input_map, input_state, target);
        platform_input_state_set(input_map, input_state, target, s);
        break;
      }
      case(WM_RBUTTONUP):
      {
        // POINTS p = MAKEPOINTS(message.lParam);
        // printf("%s time=%lu pos=(%d,%d)\n", "Right mouse release", message.time, p.x, p.y);
        platform_input_state_set(input_map, input_state, WM_RBUTTONDOWN, CONTROL_RELEASED);
        break;
      }
      case(WM_KEYDOWN):
      case(WM_KEYUP):
      {
        // bit 30: The previous key state. The value is 1 if the key is down before the message is sent, or it is zero if the key is up.
        bool32 was_down = (message.lParam & (1 << 30));
        // bit 31: The transition state. The value is always 0 for a WM_KEYDOWN message.
        bool32 is_down = ((message.lParam & (1 << 31)) == 0);
        control_state down_state;
        control_state state;
        down_state = was_down ? CONTROL_HELD : CONTROL_DOWN;
        state = is_down ?  down_state : CONTROL_RELEASED;
        platform_input_state_set(input_map, input_state, vkcode, state);
        if (vkcode == VK_ESCAPE)
        {
          SendMessageA(handle, WM_CLOSE, 0, 0);
        }
        break;
      }
    }
    // This section basically sends the message to the loop we setup with our window. (win32_message_procedure_ansi)
    TranslateMessage(&message); // turn keystrokes into characters
    DispatchMessageA(&message); // tell OS to call window procedure
  }
}


void window_size_get(platform_window *wind)
{
  // Get window dimension
  RECT size;
  GetClientRect(state.handle, &size);
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


const char * read_textfile(const char *file, arena *scratch, size_t *out_size)
{
  FILE *stream;
  char *contents = 0;
  fopen_s(&stream, file, "rb");
  ASSERT(stream, "ERROR: Failed to read file.");
  fseek(stream, 0, SEEK_END);
  size_t file_size = ftell(stream);
  *out_size = file_size + 1;
  contents = (char*) arena_alloc(scratch, *out_size);
  fseek(stream, 0, SEEK_SET);
  size_t bytes_read = fread(contents, 1, file_size, stream);
  bool8 success = bytes_read == file_size;
  ASSERT(success, "ERROR: Read incorrect number of bytes from file.");
  fclose(stream);
  // Null terminate the char array
  contents[*out_size] = '\0';
  return contents;
}


i64 platform_clock_time()
{
  LARGE_INTEGER temp;
  QueryPerformanceCounter(&temp);
  return temp.QuadPart;
}


clock platform_clock_init(f64 fps_target)
{
  clock c = {};
  c.secs_per_frame = 1.0 / fps_target;
  LARGE_INTEGER temp;
  QueryPerformanceFrequency(&temp);
  c.ticks_per_sec = (f64) temp.QuadPart;
  c.prev = platform_clock_time();
  return c;
}


void platform_clock_update(clock *c)
{
  c->curr = platform_clock_time();
  // The amount of ticks that have passed from the beginning of the frame to the end (ticks).
  // TODO: Should this be its own function?
  f64 delta_ticks = (f64) (c->curr - c->prev);
  c->delta = delta_ticks / c->ticks_per_sec;
  // TODO: End wall clock diff
  c->prev = c->curr;
}
