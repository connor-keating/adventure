#include "core.h"


void _console_write_error(const char *message)
{
  int char_written_count = 0;
  char sentence_buffer[8192] = {};
  char_written_count = snprintf(sentence_buffer, sizeof(sentence_buffer), "ERROR: %s \033[0m", message);
  // Write to terminal
  if (char_written_count < 0) {
    puts("Failed to format the error message.");
    return;
  }
  fprintf(stderr, "%s\n", sentence_buffer);
}


/// @brief Check if input x is a power of 2 (1, 2, 4, 8, 16, ...).
/// @param x 64 bit usigned integer.
/// @return 
int is_power_of_two(u64 x)
{
	u64 y = (x-1);
  int answer = (x & y) == 0;
  return answer;
}


u32 string_length(const char *array)
{
  ASSERT(array, "ERROR: Input a valid string.");
  u32 counter = 0;
  u32 limit = 500;
  while (counter < limit)
  {
    char index = array[counter];
    counter++;
    // Putting this last means the length includes the null terminator.
    if (index == '\0') break;
  }
  return counter;
}


string string_init(const char *array)
{
  string output = {};
  ASSERT(array, "ERROR: Input a valid string.");
  output.length = string_length(array);
  output.data = array;
  return output;
}


#pragma region Memory handlers

arena arena_init(void *buffer, size_t size)
{
  arena out = {};
  out.buffer = buffer;
  out.length = size;
  out.offset_old = 0;
  out.offset_new = 0;
  return out;
}


arena_savepoint arena_save(arena *original)
{
  arena_savepoint point;
  point.original = original;
  point.offset_old = original->offset_old;
  point.offset_new = original->offset_new;
  return point;
}


void arena_pop(arena_savepoint point)
{
  point.original->offset_old = point.offset_old;
  point.original->offset_new = point.offset_new;
}


uintptr_t pointer_align_forward(uintptr_t pointer, size_t alignment)
{
  bool32 aligned = is_power_of_two(alignment);
  ASSERT(aligned, "Arena pointer is not a power of 2.");
  uintptr_t  p, a, modulo;
  p = pointer;
  a = (uintptr_t) alignment;
  modulo = p & (a-1);
  if (modulo) 
  {
    p += a - modulo;
  }
  return p;
}


#define arena_push_array(arena, count, type) (type *) arena_alloc_align(arena, count*sizeof(type), _Alignof(type))
#define arena_push_struct(arena, type) (type*) arena_alloc_align(arena, sizeof(type), _Alignof(type))
void *arena_alloc_align(arena *arena, size_t size, size_t align)
{
  // Align 'offset_new' forward to the specified alignment
	uintptr_t curr_ptr = (uintptr_t)arena->buffer + (uintptr_t)arena->offset_new;
	uintptr_t offset = pointer_align_forward(curr_ptr, align);
	offset -= (uintptr_t)arena->buffer; // Change to relative offset
	// Check to see if the backing memory has space left
	if (offset+size > arena->length) 
  {
    // Return NULL if the arena is out of memory (or handle differently)
    ASSERT(0, "Arena is out of memory\n");
	}
  void *ptr = (u8 *)arena->buffer + offset;
  arena->offset_old = offset;
  arena->offset_new = offset+size;
  // Zero new memory by default
  memset((u8*)ptr, 0, size);
  return ptr;
}


void * arena_alloc(arena *arena, size_t size)
{
  void *allocation = arena_alloc_align(arena, size, DEFAULT_ALIGNMENT);
  return allocation;
}


arena subarena_init(arena *parent, size_t byte_count)
{
  void *raw = arena_alloc(parent, byte_count);
  arena subarena = arena_init(raw, byte_count);
  return subarena;
}


// Use this to create subarenas dedicated to a specific data type so its densely packed in memory.
#define subarena_for(parent, count, type) subarena_aligned_init((parent), (count*sizeof(type)), _Alignof(type))
arena subarena_aligned_init(arena *parent, size_t byte_count, size_t alignment)
{
  void *raw = arena_alloc_align(parent, byte_count, alignment);
  arena subarena = arena_init(raw, byte_count);
  return subarena;
}


const char * arena_alloc_string(arena *arena, const char *input)
{
  u32 length = string_length(input);
  size_t char_size = sizeof(char);
  char *output = (char*) arena_alloc_align(arena, length*char_size, char_size);
  memcpy(output, input, length);
  return output;
}


void arena_free_last(arena *a)
{
  uintptr_t ptr = (uintptr_t) a->buffer + (uintptr_t) a->offset_old;
  i32 diff = a->offset_new - a->offset_old;
  memset((void*)ptr, 0, diff);
  a->offset_new = a->offset_old;
}


void arena_free_all(arena *a)
{
  a->offset_new = 0;
  a->offset_old = 0;
}

#pragma endregion

