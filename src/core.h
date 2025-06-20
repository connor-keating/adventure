#pragma once

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Unsigned int types.
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

// Signed int types.
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

// Floating point types
typedef float f32;
typedef double f64;

// Boolean types
typedef int bool32;
typedef char bool8;

// Our data types

/// @brief An arena is a memory management data structure. It is a tool for working on a block of memory.
typedef struct arena arena;
struct arena 
{
  void *buffer;
  size_t length;
  size_t offset_old;
  size_t offset_new;
};

/// @brief A struct to save the current state of the arena so that you can reset to the saved locations.
typedef struct arena_savepoint arena_savepoint;
struct arena_savepoint
{
  arena *original;
  size_t offset_old;
  size_t offset_new;
};


typedef struct string string;
struct string
{
  u32 length;
  const char *data;
};


// Useful macros
// #define true  1
// #define false 0
#define global   static
#define internal static
#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define DEFAULT_ALIGNMENT (2*sizeof(void *))
// long long (LL) == int64_t are 64 bits 
#define Kilobytes(value) ((value)*1024LL)
#define Megabytes(value) (Kilobytes(value)*1024LL)
#define Gigabytes(value) (Megabytes(value)*1024LL)
#define Terabytes(value) (Gigabytes(value)*1024LL)
#define clamp(value, min, max) ((value) < (min) ? (min) : ((value) > (max) ? (max) : (value)))


#pragma region Detect Compiler
#ifdef __clang__
    #define COMPILER_CLANG
#else
    #error "Unknown or unsupported compiler."
#endif
#pragma endregion


#pragma region Detect OS
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) 
  #define PLATFORM_WINDOWS 1
  #ifndef _WIN64
      #error "Requires Windows 64-bit."
  #endif
#elif defined(__ANDROID__)
  #define PLATFORM_ANDROID 1
#elif defined(__linux__) || defined(__gnu_linux__)
  #define PLATFORM_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
  #define PLATFORM_APPLE 1
  #include <TargetConditionals.h>
  #if TARGET_OS_IPHONE
    #define PLATFORM_IPHONE 1
  #elif TARGET_OS_IPAD
    #define PLATFORM_IPAD 1
  #elif TARGET_OS_MAC
    #define PLATFORM_MAC 1
  #elif TARGET_IPHONE_SIMULATOR
    #define PLATFORM_IOS_SIMULATOR 1
  #else
    #define PLATFORM "Unknown Apple Device"
  #endif
#elif defined(__FreeBSD__)
  #define PLATFORM_FREEBSD 1 
#else
  #define PLATFORM_UNKNOWN 1
#endif
#pragma endregion


#pragma region Detect CPU Architecture
// 64-bit
#if defined(__x86_64__) || defined(_M_X64)
  #define ARCH_x86_64 1 
#elif defined(__aarch64__) || defined(_M_ARM64)
  #define ARCH_ARM64
// 32-bit
#elif defined(__i386__) || defined(_M_IX86)
  #define ARCH_x86 1
#elif defined(__arm__) || defined(_M_ARM)
  #define ARCH_ARM
#else
    #error "Unknown or Unsupported Architecture." 
#endif
#pragma endregion


#pragma region Library function decorators
#ifdef _EXPORT
  // Exports
  #ifdef _MSC_VER
    // Windows
    #define LIBFUNC __declspec(dllexport)
  #else
    // Clang & gcc
    #define LIBFUNC __attribute__((visibility("default")))
  #endif
#else
  // Imports
  #ifdef _MSC_VER
  // Windows
  #define LIBFUNC __declspec(dllimport)
  #else
  // Clang & gcc
    #define LIBFUNC
  #endif
#endif
#pragma endregion





int is_power_of_two(u64 x);

u32 string_length(char* array);
string string_init(const char *array);

void              arena_init(arena *self, void *buffer, size_t size);
arena_savepoint   arena_save(arena *original);
uintptr_t         pointer_align_forward(uintptr_t pointer, size_t alignment);
void *            arena_alloc_align(arena *arena, size_t size, size_t align);
#define           arena_alloc_array(arena, count, type) (type *) arena_alloc_align(arena, count*sizeof(type), _Alignof(type))
void *            arena_alloc(arena *arena, size_t size);
const char *      arena_alloc_string(arena *arena, const char *input);
void              arena_pop(arena_savepoint point);
void              arena_free_last(arena *a);
void              arena_free_all(arena *a);


#if _DEBUG
  #define DEBUG(message) printf(message); fflush(stdout);
  #ifdef COMPILER_CLANG
    #define DEBUG_BREAK __builtin_trap()
  #endif
  // MSVC: #define DEBUG_BREAK() __debugbreak()
  #define ASSERT(expression, message)    \
  {                                      \
    if(!(expression))                  \
    {                                  \
      DEBUG_BREAK;                 \
    }                                  \
  }                                      
#else
  // If not debugging, this expands to nothing, so there is no cost of leaving this in your production build
  #define ASSERT(expression, message)
  #define DEBUG(message)
#endif