#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

	void read_file(char const * filename, uint8_t ** out_buffer, int * out_size);
	void read_text_file(char const* filename, char** out_buffer, int* out_size);
	void get_exe_path(char * out_buffer, int buffer_size);

#ifdef __cplusplus
}
#endif 

#endif
