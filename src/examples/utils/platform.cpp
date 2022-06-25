

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string>
#include <sstream>
#include <fstream>

#include "platform.h"

#ifdef _WIN32

#include <Windows.h>

	void get_exe_path(char * out_buffer, int buffer_size)
	{
		DWORD len = GetModuleFileNameA(NULL, out_buffer, buffer_size);
		if (!len) {
			DWORD error = GetLastError();
			char errorMsgBuf[256];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				errorMsgBuf, (sizeof(errorMsgBuf) / sizeof(char)), NULL);

			printf("%s\n", errorMsgBuf);
		}

		// strip actual name of the .exe
		char * last = out_buffer + len;
		while (*last != '\\') {
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

#elif __APPLE__

	void get_exe_path(char * out_buffer, int buffer_size)
	{
		int error = _NSGetExecutablePath(out_buffer, &buffer_size);
		if (error) {
			// TOOO: handle error
		}
		int len = strlen(out_buffer);
		char * slash = out_buffer + len + 1;
		while (len >= 0 && *slash != '/') { slash--; len--; }
		out_buffer[len + 1] = '\0';
	}

#elif __linux__
	#include <unistd.h>

	void get_exe_path(char * out_buffer, int buffer_size)
	{
		readlink("/proc/self/exe", out_buffer, buffer_size);
		int len = strlen(out_buffer);
		char * slash = out_buffer + len + 1;
		while (len >= 0 && *slash != '/') { slash--; len--; }
		out_buffer[len + 1] = '\0';
		printf("%s\n", out_buffer);
	}
#endif 




/* Platform independent (only C-libary which _should_ work on all systems). */

void read_file(char const* filename, uint8_t** out_buffer, int* out_size)
{
	char exe_path[256];
	get_exe_path(exe_path, 256 * sizeof(char));
	char abs_path[256];
	memcpy(abs_path, exe_path, 256);
	strcat(abs_path, filename);
	FILE* file = fopen(abs_path, "rb");
	fseek(file, 0L, SEEK_END);
	*out_size = ftell(file);
	fseek(file, 0L, SEEK_SET);
	*out_buffer = (uint8_t*)malloc(*out_size);
	fread(*out_buffer, sizeof(char), *out_size, file);
	fclose(file);
}

void read_text_file(char const* filename, char** out_buffer, int* out_size)
{
	char exe_path[256];
	get_exe_path(exe_path, 256 * sizeof(char));
	char abs_path[256];
	memcpy(abs_path, exe_path, 256);
	strcat(abs_path, filename);

	std::ifstream iFileStream;
	std::stringstream ss;
	iFileStream.open(abs_path, std::ifstream::in);
	ss << iFileStream.rdbuf();
	std::string data = ss.str();
	iFileStream.close();

	*out_size = data.length();
	*out_buffer = (char*)malloc(*out_size);
	memcpy(*out_buffer, data.c_str(), *out_size);
}