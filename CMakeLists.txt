#make a build folder, go into it and call
#cmake -G "Visual Studio 15 2017 Win64" -DWINDOWING=VKAL_GLFW ..
#to use GLFW as a windowing backend.


cmake_minimum_required(VERSION 3.10)
project(VKAL VERSION 1.0)
cmake_minimum_required(VERSION 3.12)

find_package(Vulkan REQUIRED)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

# HACK: Unix Makefiles wont generate a Debug/Release/... folders. 
# So the realtive paths in the examples wont match. Annoying...
if (UNIX)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/out")
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/out")
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/out")
endif (UNIX)

# HACK: to the previous Hack. Apple is also Unix. So we need to overwrite
# this again because Xcode will generate Debug/Release/... folders.
if (APPLE)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
endif (APPLE)

if (APPLE)
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
endif (APPLE)

if (${WINDOWING} STREQUAL "VKAL_GLFW")
    message ("Using GLFW...")
	# GLFW, https://www.glfw.org/docs/latest/build.html#build_link_cmake_source
	set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
	add_subdirectory(src/external/glfw332src)
elseif(${WINDOWING} STREQUAL "VKAL_SDL")
	message("Using SDL2...")
	add_subdirectory(src/external/SDL)
	# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/external/SDL)
endif()


add_subdirectory(src/vkal)

# Examples
add_subdirectory(src/examples)

target_compile_definitions(vkal PUBLIC ${WINDOWING})

add_dependencies(GLFW_Texture       vkal)
add_dependencies(GLFW_ModelLoading  vkal)
add_dependencies(SDL_HelloTriangle  vkal)
add_dependencies(GLFW_HelloTriangle vkal)







