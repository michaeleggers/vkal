#make a build folder, go into it and call
#cmake -G "Visual Studio 15 2017 Win64" -DWINDOWING=VKAL_GLFW ..
#to use GLFW as a windowing backend.


cmake_minimum_required(VERSION 3.10)
project(VKAL VERSION 1.0)
cmake_minimum_required(VERSION 3.12)

if (WIN32)
    message ("Configuring for WINDOWS platform: Checking if VULKAN is installed...")
    find_package(Vulkan REQUIRED)
endif (WIN32)

if (APPLE)
    message ("Configuring for APPLE platform...")
endif (APPLE)

if (${WINDOWING} STREQUAL "VKAL_GLFW")
    message ("Using GLFW...")
	# GLFW, https://www.glfw.org/docs/latest/build.html#build_link_cmake_source
	set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
	add_subdirectory(src/external/glfw332src)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
add_subdirectory(src/vkal)

