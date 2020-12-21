#ifndef Q2_IO_H
#define Q2_IO_H

#include "q2_common.h"


static Image generate_checkerboard_img(void);
Image  load_image_file_from_dir(char * dir, char * file);
void   q2_destroy_image(Image * img);
#endif