#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

typedef void(*READ_FILE_BINARY)(char const * filename, uint8_t ** out_buffer, int * out_size);
typedef void(*GET_EXE_PATH)(uint8_t * out_buffer, int * out_size);
typedef void*(*INITIALIZE_MEMORY)(uint32_t size);
typedef struct Platform
{
	READ_FILE_BINARY  rfb;
	GET_EXE_PATH      gep;
	INITIALIZE_MEMORY initialize_memory;
} Platform;

/* Interface */

#ifdef __cplusplus
extern "C"{
#endif 

void init_platform(Platform * p);

/* API specific. Not supposed to be called by user. */

#ifdef __cplusplus
}
#endif


#endif
