#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <string>


void		read_file(char const * filename, uint8_t ** out_buffer, int * out_size);
std::string read_text_file(char const* filename);
void		get_exe_path(char * out_buffer, int buffer_size);


#endif
