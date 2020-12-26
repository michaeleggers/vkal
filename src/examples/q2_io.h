#ifndef Q2_IO_H
#define Q2_IO_H

#include "q2_common.h"

static		Image checkerboard_img;
static		Image generate_checkerboard_img(void);
Image		load_image_file_from_dir(char * dir, char * file);
void		q2_destroy_image(Image * img);
Image		load_image_file(char const * file);
void		load_shader_from_dir(char * dir, char * file, uint8_t ** out_byte_code, int * out_code_size);

char        g_exe_dir[128];

#endif