#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>


struct MemoryArena
{
	void * base;
	uint32_t size;
	uint32_t current;
};


typedef void(*READ_FILE_BINARY)(char const * filename, uint8_t ** out_buffer, int * out_size);
typedef void*(*MMALLOC)(MemoryArena * arena, uint32_t size);
typedef void(*MDALLOC)(MemoryArena * arena);
typedef uint32_t(*MEMUSED)(MemoryArena * arena);
typedef void*(*INITIALIZE_MEMORY)(uint32_t size);
struct Platform
{
	READ_FILE_BINARY rfb;
	MMALLOC mmalloc;
	MDALLOC mdalloc;
	MEMUSED memused;
	INITIALIZE_MEMORY initialize_memory;
};

/* Interface */
void init_platform(Platform * p);
void * mmalloc(MemoryArena * arena, uint32_t size);
void mdalloc(MemoryArena * arena);
void initialize_arena(MemoryArena * arena, void * base, uint32_t size);
uint32_t memused(MemoryArena * arena);

/* API specific. Not supposed to be called by user. */
void internal_zero_memory(void * memory, uint32_t size);

#endif