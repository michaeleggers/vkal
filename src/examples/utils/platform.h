#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

typedef void(*READ_FILE)(char const * filename, uint8_t ** out_buffer, int * out_size);
typedef void(*GET_EXE_PATH)(char * out_buffer, int buffer_size);
typedef void*(*INITIALIZE_MEMORY)(uint32_t size);
typedef struct Platform
{
	READ_FILE         read_file;
	GET_EXE_PATH      get_exe_path;
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
