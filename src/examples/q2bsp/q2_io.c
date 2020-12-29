
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <physfs.h>

#include "../../stb_image.h"

#include "q2_io.h"

static Image generate_checkerboard_img(void)
{	
	Image img = (Image){ 0 };
	uint32_t width  = 64;
	uint32_t height = 64;
	unsigned char * data = (unsigned char*)malloc(width*height*4);
	uint32_t stride = 4;	
	for (uint32_t i = 0; i < height; ++i) {
		for (uint32_t j = 0; j < width; ++j) {
			uint8_t oddeven = ( (i/4) % 2 ) ^ ( (j/4) % 2 );
			uint8_t color = 255*oddeven;
			*(data + i*width*stride + j*stride + 0) = color;
			*(data + i*width*stride + j*stride + 1) = color;
			*(data + i*width*stride + j*stride + 2) = color;
			*(data + i*width*stride + j*stride + 3) = 0xFF;//(i / 16) % 2;		
		}
	}	

	img.width    = width;
	img.height   = height;
	img.channels = 4;
	img.data     = data;

	return img;
}

// TODO: do not use malloc here. use preallocated scratch buffer or something like that!
// TODO: what if file was not found? Return precomputed checkerboard or something?
Image load_image_file_from_dir(char * dir, char * file)
{
    Image image = (Image){ 0 };

	/* Build search path within archive */
	char searchpath[64] = { '\0' };
    int dir_len = string_length(dir);
    strcpy(searchpath, dir);
    searchpath[dir_len++] = '/';
    int file_name_len = string_length(file);
    strcpy(searchpath + dir_len, file);
    strcpy(searchpath + dir_len + file_name_len, ".tga");

	/* Use PhysFS to load file data */
	PHYSFS_File * phys_file = PHYSFS_openRead(searchpath);
    if ( !phys_file ) {
	   printf("PHYSFS_openRead() failed!\n  reason: %s.\n", PHYSFS_getLastError());
	   printf("Warning: Image File not found: %s\n", searchpath);
	   return generate_checkerboard_img();	 
	}
    uint64_t file_length = PHYSFS_fileLength(phys_file);
    void * buffer = malloc(file_length);
    PHYSFS_readBytes(phys_file, buffer, file_length);

	/* Finally, load the image from the memory buffer */
	int x, y, n;
    image.data = stbi_load_from_memory(buffer, (int)file_length, &x, &y, &n, 4);
    assert(image.data != NULL);
    image.width    = x;
    image.height   = y;
    image.channels = n;

    free(buffer);

	return image;
}

Image load_image_file(char const * file)
{
	Image image = (Image){0};
	int tw, th, tn;
	image.data = stbi_load(file, &tw, &th, &tn, 4);
	assert(image.data != NULL);
	image.width = tw;
	image.height = th;
	image.channels = tn;

	return image;
}

void  q2_destroy_image(Image * img)
{
	if (img->data) {
		free(img->data);
	}
}

void load_shader_from_dir(char * dir, char * file, uint8_t ** out_byte_code, int * out_code_size)
{
	uint8_t shader_path[128];
	concat_str(g_exe_dir, dir,  shader_path);
	concat_str(shader_path, file, shader_path);
	p.read_file(shader_path, out_byte_code, out_code_size);
}