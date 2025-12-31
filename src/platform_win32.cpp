#include <windows.h>

#include "platform.h"
#include "opengl.h"


struct platform_state
{
  HWND handle;
  HINSTANCE instance;
  HDC render_context;
  bool is_running; 
};


// Internal global state
global platform_state *windstate; // Global ptr to internal state


// Unique windows functions not supposed to be loaded with the others.
typedef BOOL WINAPI glfunc_wglChoosePixelFormatARB(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
typedef HGLRC WINAPI glfunc_wglCreateContextAttribsARB (HDC hDC, HGLRC hShareContext, const int *attribList);


internal LRESULT CALLBACK win32_message_callback(HWND window_handle, UINT message_id, WPARAM param_w, LPARAM param_l)
{
  LRESULT result = 0;
  // u32 vkcode = (u32) param_w;
  switch (message_id) 
  {
    case WM_CLOSE:
    {
      ShowWindow(windstate->handle, SW_HIDE);  // Hide window immediately
      windstate->is_running = false;
      PostQuitMessage(0);
      break;
    }
    case WM_SIZE: 
    {
      // Save the new width and height of the client area. 
      // dwClientX = LOWORD(lParam); 
      // dwClientY = HIWORD(lParam); 
      break;
    }
    default: 
    {
      result = DefWindowProcA(window_handle, message_id, param_w, param_l); 
    }
  }
  return result;
}


internal char* file_mmap(size_t* len, const char* filename) {
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


internal input_key platform_input_translate( u32 id )
{
  input_key output;
  switch (id)
  {
    case (MK_LBUTTON): output = KEY_SELECT;  break;
    case (VK_ESCAPE):  output = KEY_ESCAPE;  break;
    default:           output = KEY_UNKNOWN; break; 
  };
  return output;
}


void platform_file_data(void* ctx, const char* filename, const int is_mtl, const char* obj_filename, char** data, size_t* len)
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
  *data = file_mmap(&data_len, filename);
  (*len) = data_len;
}


void platform_init(arena *a)
{
  windstate = arena_push_struct(a, platform_state);
  windstate->is_running = true;
}


platform_window platform_window_init()
{
  platform_window wind = {};
  windstate->instance = GetModuleHandleA(0);
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
  window_class.hInstance = windstate->instance;
  window_class.hCursor = LoadCursor(0, IDC_ARROW);
  // window_class.hbrBackground = CreateSolidBrush(RGB(255, 0, 255)); // Magenta background
  window_class.hbrBackground = (HBRUSH)COLOR_WINDOW;
  window_class.lpszClassName = "WINDOW_CLASS";
  ATOM window_id = RegisterClassExA(&window_class);
  ASSERT(window_id, "ERROR: Failed to register window.");
  
  // Create the window we described.
  windstate->handle = CreateWindowExA(
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
    windstate->instance,      // window_handle: handle to this window's module.
    0                          // window_data_pointer: Pointer to CREATESTRUCT var that sends a message to the window.
  );
  ASSERT(windstate->handle, "ERROR: Failed to create window.");

  // Get monitor info and resize the window.
  MONITORINFO monitor_info = {};
  monitor_info.cbSize = sizeof(MONITORINFO);
  HMONITOR monitor_handle = MonitorFromWindow(windstate->handle, MONITOR_DEFAULTTOPRIMARY);
  BOOL monitor_info_success = GetMonitorInfoA(monitor_handle, &monitor_info);
  ASSERT(monitor_info_success, "ERROR: Failed to get monitor info.");
  i32 monitor_width  = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
  i32 monitor_height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;
  // Calculate our desired window dimensions.
  // Get DPI scaling factor
  u32 dpi = GetDpiForWindow(windstate->handle);
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
    windstate->handle,
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
  wind.state = &windstate;
  return wind;
}


void platform_window_show()
{
  i32 display_flags = SW_SHOW;
  ShowWindow(windstate->handle, display_flags);
  UpdateWindow(windstate->handle);
}


void platform_window_size(platform_window *wind)
{
  // Get window dimension
  RECT size;
  GetClientRect(windstate->handle, &size);
  wind->width  = size.right;
  wind->height = size.bottom;
  if ((wind->width == 0) && (wind->height == 0))
  {
    printf("minimized\n");
  }
}


bool platform_is_running()
{
  return windstate->is_running;
}


void platform_message_process( platform_window *window, input_state *inputs )
{
  MSG message = {};
  // This has to be in condition otherwise you'll process the message twice.
  while (PeekMessageA(&message, windstate->handle, 0, 0, PM_REMOVE))
  {
    u32 vkcode = (u32) message.wParam;
    u32 message_id = message.message;
    switch (message_id)
    {
      case(WM_LBUTTONUP):
      {
        vkcode = MK_LBUTTON;
        input_key app_key = platform_input_translate( vkcode );
        inputs[app_key] = INPUT_RELEASED;
        break;
      };
      case(WM_LBUTTONDOWN):
      {
        input_key app_key = platform_input_translate( vkcode );
        inputs[app_key] = INPUT_DOWN;
        break;
      };
      case(WM_KEYUP):
      case(WM_KEYDOWN):
      {
        input_key app_key = platform_input_translate( vkcode );
        // bit 30: The previous key state. The value is 1 if the key is down before the message is sent, or it is zero if the key is up.
        bool32 was_down = (message.lParam & (1 << 30));
        // bit 31: The transition state. The value is always 0 for a WM_KEYDOWN message.
        bool32 is_down = ((message.lParam & (1 << 31)) == 0);
        // Previous input handling stuff
        input_state down_state = was_down ? INPUT_HELD : INPUT_DOWN;
        input_state state = is_down ?  down_state : INPUT_RELEASED;
        inputs[app_key] = state;
        // Debugging stuff
        /*
        if (vkcode == 	0x41)
        {
          printf("Key: A ");
          switch (state)
          {
            case (INPUT_DOWN):     printf("down\n"); break;
            case (INPUT_UP):       printf("up\n"); break;
            case (INPUT_HELD):     printf("held\n"); break;
            case (INPUT_RELEASED): printf("released\n"); break;
          };
        }
        */
        break;
      };
    };
    // This section basically sends the message to the loop we setup with our window. (win32_message_procedure_ansi)
    TranslateMessage(&message); // turn keystrokes into characters
    DispatchMessageA(&message); // tell OS to call window procedure
  }
}


void platform_opengl_init()
{
  glfunc_wglChoosePixelFormatARB *wglChoosePixelFormatARB = 0;
  glfunc_wglCreateContextAttribsARB *wglCreateContextAttribsARB = 0;

  // Use first window to load OpenGL stuff
  {
    HDC fakeDC = GetDC(windstate->handle);
    PIXELFORMATDESCRIPTOR pixelFormat = {0};
    pixelFormat.nSize = sizeof(pixelFormat);
    pixelFormat.nVersion = 1;
    pixelFormat.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pixelFormat.iPixelType = PFD_TYPE_RGBA;
    pixelFormat.cColorBits = 32;
    pixelFormat.cAlphaBits = 8;
    pixelFormat.cDepthBits = 24;
  
    int chosenPixelFormat = ChoosePixelFormat(fakeDC, &pixelFormat);
    ASSERT(chosenPixelFormat != 0, "ERROR: Failed to choose pixel format descriptor.");

    int isPixelFormatSet = SetPixelFormat(fakeDC, chosenPixelFormat, &pixelFormat);
    ASSERT(isPixelFormatSet != 0, "ERROR: Failed to set the pixel format descriptor.");

    HGLRC fakeRC = wglCreateContext(fakeDC);
    ASSERT(fakeRC != 0, "ERROR: Failed to create fake rendering context.");

    int isFakeCurrent = wglMakeCurrent(fakeDC, fakeRC);
    ASSERT(isFakeCurrent != 0, "ERROR: Failed to make the OpenGL rendering context the current rendering context.");

    wglChoosePixelFormatARB    = (glfunc_wglChoosePixelFormatARB*) wglGetProcAddress("wglChoosePixelFormatARB");
    wglCreateContextAttribsARB = (glfunc_wglCreateContextAttribsARB*) wglGetProcAddress("wglCreateContextAttribsARB");
    bool loading_failed = (wglChoosePixelFormatARB == 0 || wglCreateContextAttribsARB == 0);
    ASSERT(loading_failed == false, "ERROR: Failed to load OpenGL functions.");
    wglMakeCurrent(fakeDC, 0);
    wglDeleteContext(fakeRC);
    ReleaseDC(windstate->handle, fakeDC);
    DestroyWindow(windstate->handle);
    UnregisterClassA("WINDOW_CLASS", windstate->instance);
    windstate->handle = nullptr;
    windstate->instance = nullptr;
  }

  // Create the real window
  {
    // Re-create the window
    platform_window_init();
    // Get the device context
    windstate->render_context = GetDC(windstate->handle);
    ASSERT(windstate->render_context != 0, "ERROR: Failed to get device context.");
    const int pixelAttribs[] =
    {
      WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
      WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
      WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
      WGL_SWAP_METHOD_ARB,    WGL_SWAP_COPY_ARB,
      WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
      WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
      WGL_COLOR_BITS_ARB,     32,
      WGL_ALPHA_BITS_ARB,     8,
      WGL_DEPTH_BITS_ARB,     24,
      0 // Terminate with 0, otherwise OpenGL will throw an Error!
    };
    UINT numPixelFormats;
    int pixelFormat = 0;
    BOOL chosenPixelFormatARB = wglChoosePixelFormatARB(
      windstate->render_context,
      pixelAttribs,
      0, // Float List
      1, // Max Formats
      &pixelFormat,
      &numPixelFormats
    );
    ASSERT(chosenPixelFormatARB != 0, "ERROR: Failed to wglChoosePixelFormatARB");
    PIXELFORMATDESCRIPTOR pixelFormatDescriptor = {0};
    DescribePixelFormat(windstate->render_context, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pixelFormatDescriptor);
    BOOL isPixelFormatSet = SetPixelFormat(windstate->render_context, pixelFormat, &pixelFormatDescriptor);
    ASSERT(isPixelFormatSet != 0, "ERROR: Failed to set the pixel format.");
    const int contextAttribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_PROFILE_MASK_ARB, 
        WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        WGL_CONTEXT_FLAGS_ARB, 
        WGL_CONTEXT_DEBUG_BIT_ARB,
        0 // Terminate the Array
    };
    HGLRC renderingContext = wglCreateContextAttribsARB(windstate->render_context, 0, contextAttribs);
    ASSERT(renderingContext != 0, "ERROR: Failed to create rendering context.");
    BOOL isContextSet = wglMakeCurrent(windstate->render_context, renderingContext);
    ASSERT(isContextSet != 0, "ERROR: Failed to set the device and rendering context.");
  }
}


void * platform_window_handle()
{
  return (void*)&windstate->handle;
}


void platform_swapbuffers()
{
  SwapBuffers(windstate->render_context);
}


const char * platform_file_read(const char *file, arena *scratch, size_t *out_size)
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


int platform_file_exists(const char *filepath)
{
  int exists = 0;
  FILE *file = fopen(filepath, "r");
  if (file) {
    fclose(file);
    exists =  1;
  }
  return exists;
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



void * platform_memory_alloc(void *mem_base, size_t mem_size)
{
  void *memory = VirtualAlloc(mem_base, mem_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  ASSERT(memory, "ERROR: Unable to allocate application memory.");
  return memory;
}


void platform_window_close()
{
  SendMessageA(windstate->handle, WM_CLOSE, 0, 0);
}


// TODO: Delete?
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
  // This func gives time units "counts"
  QueryPerformanceCounter(&temp);
  return temp.QuadPart;
}


clock platform_clock_init(f64 fps_target)
{
  clock c = {};
  c.base = 0;
  c.curr = 0;
  c.prev = 0;
  c.stop = 0;
  c.delta = -1.0f;
  c.paused = false;
  c.secs_per_frame = 1.0 / fps_target;
  LARGE_INTEGER temp;
  // This function gives a frequency (counts per sec or counts/sec)
  QueryPerformanceFrequency(&temp);
  f64 counts_per_sec = (f64) temp.QuadPart;
  c.secs_per_count = 1.0f / counts_per_sec;
  return c;
}


void platform_clock_reset(clock *c)
{
  i64 t_current = platform_clock_time();
  c->base = t_current;
  c->prev = t_current;
  c->paused = false;
  c->stop = 0;
}


void platform_clock_update(clock *c)
{
  c->curr = platform_clock_time();
  // The amount of ticks that have passed from the beginning of the frame to the end (ticks).
  // TODO: Should this be its own function?
  f64 delta_ticks = (f64) (c->curr - c->prev);
  c->delta = delta_ticks * c->secs_per_count;
  // Prepare for next frame
  c->prev = c->curr;
  // Force nonnegative.  The DXSDK's CDXUTTimer mentions that if the 
	// processor goes into a power save mode or we get shuffled to another
	// processor, then mDeltaTime can be negative.
	if(c->delta < 0.0)
	{
		c->delta = 0.0;
	}
  // TODO: End wall clock diff
}


void* platform_dll_load(const char *filepath)
{
  HMODULE dll_handle = LoadLibraryA(filepath);
  ASSERT(dll_handle, "Failed to load DLL.\n");
  return (void*)dll_handle;
}


void * platform_dll_func_load(void *dll, const char *func_name)
{
  HMODULE dll_in = (HMODULE) dll;
  void *func_ptr = (void*) GetProcAddress( dll_in, func_name );
  return func_ptr;
}


void platform_sleep(u32 miliseconds)
{
  Sleep(miliseconds);
}


void platform_cursor_client_position(f32 *xout, f32 *yout, f64 width, f64 height)
{
  f64 w_half  = width / 2;
  f64 h_half = height / 2;
  f32 xtemp = 0;
  f32 ytemp = 0;
  POINT p = {};
  GetCursorPos( &p );
  ScreenToClient(windstate->handle, &p);
  xtemp = (f32)p.x;
  ytemp = (f32)p.y;
  *xout = myclamp(xtemp, 0.0f, width);
  *yout = myclamp(ytemp, 0.0f, height);
}