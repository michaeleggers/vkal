

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "platform.h"


#ifdef _WIN32

#include <Windows.h>

void win32_read_file(char const * filename, uint8_t ** out_buffer, int * out_size)
{
	//FILE * file = 0;
	//fopen_s(&file, filename, "rb");
	//fseek(file, 0L, SEEK_END);
	//*out_size = ftell(file);
	//fseek(file, 0L, SEEK_SET);
	//*out_buffer = (char*)malloc(*out_size);
	//fread(*out_buffer, sizeof(char), *out_size, file);
	//fclose(file);
	HANDLE fileHandle;
	fileHandle = CreateFile(filename,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (fileHandle == INVALID_HANDLE_VALUE)
		printf("unable to open file: %s\n", filename);

	DWORD filesize = GetFileSize(fileHandle, NULL);

	*out_buffer = (uint8_t*)VirtualAlloc(
		NULL,
		(filesize+1) * sizeof(uint8_t),
		MEM_RESERVE | MEM_COMMIT,
		PAGE_READWRITE
	);
	//*out_buffer = (uint8_t*)malloc((filesize + 1) * sizeof(uint8_t));
	assert( (*out_buffer != NULL) && "Failed to allocate memory for file" );

	struct _OVERLAPPED ov = { 0 };
	DWORD numBytesRead = 0;
	DWORD error;
	if (ReadFile(fileHandle, *out_buffer, filesize, &numBytesRead, NULL) == 0)
	{
		error = GetLastError();
		char errorMsgBuf[256];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			errorMsgBuf, (sizeof(errorMsgBuf) / sizeof(char)), NULL);

		printf("%s\n", errorMsgBuf);
	}
	else
	{
		(*out_buffer)[filesize] = '\0';
		*out_size = filesize;
	}
	CloseHandle(fileHandle);
}

void win32_get_exe_path(char * out_buffer, int buffer_size)
{
	DWORD len = GetModuleFileNameA(NULL, out_buffer, buffer_size);
	if ( !len ) {
		DWORD error = GetLastError();
		char errorMsgBuf[256];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			errorMsgBuf, (sizeof(errorMsgBuf) / sizeof(char)), NULL);

		printf("%s\n", errorMsgBuf);
	}

	// strip actual name of the .exe
	char * last = out_buffer + len;
	while ( *last != '\\') {
		*last-- = '\0';
	}	

	// NOTE: GetCurrentDirectory returns path from WHERE the exe was called from.
	//if (!GetCurrentDirectory(
	//	buffer_size,
	//	out_buffer
	//)) {
	//	DWORD error = GetLastError();
	//	char errorMsgBuf[256];
	//	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	//		NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	//		errorMsgBuf, (sizeof(errorMsgBuf) / sizeof(char)), NULL);

	//	printf("%s\n", errorMsgBuf);
	//}
}

void * win32_initialize_memory(uint32_t size)
{
	void * result = malloc(size);
	assert( (result != NULL) && "Failed to initialize memory" );
	memset(result, 0, size);
	return result;
}

#elif __APPLE__

void mac_read_file_binary(char const * filename, char ** out_buffer, int * out_size)
{

}

#elif __linux__

#endif 

void init_platform(Platform * p)
{
	/* Platform specific */
#ifdef _WIN32
	p->read_file = win32_read_file;
	p->get_exe_path = win32_get_exe_path;
	p->initialize_memory = win32_initialize_memory;	

#elif __APPLE__
	p->read_file    = mac_read_file;
	p->get_exe_path = mac_get_exe_path;

#elif __linux__


#endif

	/* Platform independent*/

}
