#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

typedef void(*READ_FILE_BINARY)(char const * filename, uint8_t ** out_buffer, int * out_size);
typedef void*(*INITIALIZE_MEMORY)(uint32_t size);
typedef struct Platform
{
	READ_FILE_BINARY rfb;
	INITIALIZE_MEMORY initialize_memory;
} Platform;

/* Interface */
void init_platform(Platform * p);

/* API specific. Not supposed to be called by user. */


#endif
