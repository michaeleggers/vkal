#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <string>


void		read_file(char const * filename, uint8_t ** out_buffer, int * out_size);
std::string read_text_file(char const* filename);
void		get_exe_path(char * out_buffer, int buffer_size);
std::string concat_paths(std::string a, std::string b);
void		init_directories(char const* assets_dir, char const* shaders_dir);
std::string get_assets_dir();
std::string get_shaders_dir();
void		read_asset_file(char const* filename, uint8_t** out_buffer, int* out_size);
void		read_shader_file(char const* filename, uint8_t** out_buffer, int* out_size);



#endif
