

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string>
#include <sstream>
#include <fstream>

#include "platform.h"

static std::string g_assets_dir;
static std::string g_shaders_dir;

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

	#include <mach-o/dyld.h>

	void get_exe_path(char * out_buffer, int buffer_size)
	{
		uint32_t u_buffer_size = (uint32_t)buffer_size;
		int error = _NSGetExecutablePath(out_buffer, &u_buffer_size);
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




/* Platform independent part. Should work on all systems. */

std::string concat_paths(std::string a, std::string b)
{
	size_t lastOfAIdx = a.size() - 1;
    char endOfA = a[lastOfAIdx];
    char startOfB = b[0];

	if (endOfA == '\\') { // On windows, this is a possibility (sadly)
		if (startOfB == '\\' || startOfB == '/') {
			a.resize(a.size() - 1);
		}
	}    
	else if (endOfA == '/') { // All systems
		if (startOfB == '/') {
			a.resize(a.size() - 1);
		}
    }	
    else {
		if (startOfB != '/') {
			a += '/';
		}
	}

    a += b;

    return a;
}

void read_file(char const* filename, uint8_t** out_buffer, int* out_size)
{	
	char exe_path[256];
	get_exe_path(exe_path, 256 * sizeof(char));

	std::string abs_path = concat_paths(std::string(exe_path), std::string(filename));

	FILE* file = fopen(abs_path.c_str(), "rb");
    if (!file) {
        fprintf(stderr, "Failed to read file: %s\n", abs_path.c_str());
        *out_buffer = NULL;
        *out_size = 0;
        
        return;
    }
	fseek(file, 0L, SEEK_END);
	*out_size = ftell(file);
	fseek(file, 0L, SEEK_SET);
	*out_buffer = (uint8_t*)malloc(*out_size);
	fread(*out_buffer, sizeof(char), *out_size, file);
	fclose(file);
}

std::string read_text_file(char const* filename)
{
	char exe_path[256];
	get_exe_path(exe_path, 256 * sizeof(char));

	std::string final_path = concat_paths(std::string(exe_path), std::string(filename));

	std::ifstream iFileStream;
	std::stringstream ss;
	iFileStream.open(final_path, std::ifstream::in);
	if (!iFileStream.good()) {
		fprintf(stderr, "Not able to open file: %s\n", final_path.c_str());
		system("pause"); // does this work on all systems?
		exit(-1);
	}
	ss << iFileStream.rdbuf();
	std::string data = ss.str();
	iFileStream.close();

	return data;
}

void read_asset_file(char const* filename, uint8_t** out_buffer, int* out_size)
{
	std::string final_path = concat_paths(g_assets_dir, std::string(filename));
	read_file(final_path.c_str(), out_buffer, out_size);
}

void read_shader_file(char const* filename, uint8_t** out_buffer, int* out_size) {
	std::string final_path = concat_paths(g_shaders_dir, std::string(filename));
	read_file(final_path.c_str(), out_buffer, out_size);
}

static void set_directory(std::string& dir_to_set, std::string dir_path) {
	dir_to_set = dir_path;
}

void init_directories(char const * assets_dir, char const * shaders_dir) {

	char exe_path[256];
	get_exe_path(exe_path, 256);

	// Default: Use location of executable

	if (!assets_dir) { 
		set_directory(g_assets_dir, std::string(exe_path));
	}
	else {
		g_assets_dir = std::string(assets_dir);
	}

	if (!shaders_dir) {
		set_directory(g_shaders_dir, std::string(exe_path));
	}
	else {
		g_shaders_dir = std::string(shaders_dir);
	}
}

std::string get_assets_dir() {
	return g_assets_dir;
}

std::string get_shaders_dir() {
	return g_shaders_dir;
}