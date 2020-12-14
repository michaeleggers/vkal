
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <physfs.h>

#include "../stb_image.h"

#include "q2_io.h"

// TODO: do not use malloc here. use preallocated scratch buffer or something like that!
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
	   getchar();
	   exit(-1);
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