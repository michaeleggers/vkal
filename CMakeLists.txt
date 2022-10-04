#make a build folder, go into it and call
#cmake -G "Visual Studio 15 2017 Win64" -DWINDOWING=VKAL_GLFW ..
#to use GLFW as a windowing backend.


cmake_minimum_required(VERSION 3.24)
project(VKAL VERSION 1.0)

find_package(Vulkan COMPONENTS shaderc_combined REQUIRED)
message(STATUS "Vulkan_shaderc_combined_FOUND: ${Vulkan_shaderc_combined_FOUND}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

option (BUILD_EXAMPLES "Create Example Projects" OFF)

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

if (NOT DEFINED WINDOWING)
    set(WINDOWING "VKAL_GLFW")
endif()

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
elseif(${WINDOWING} STREQUAL "VKAL_WIN32")
	message("Using native WIN32 API...")
endif()


# add_subdirectory(src/vkal)
# Add VKAL sources as static lib
message("Cmake Module Path: " ${Vulkan_INCLUDE_DIRS})
add_library(vkal STATIC
    vkal.c
    vkal.h
)

if (WIN32)
    target_link_libraries(vkal
        PRIVATE Vulkan::Vulkan
        PRIVATE Vulkan::shaderc_combined
    )
    target_include_directories(vkal
        PRIVATE ${Vulkan_INCLUDE_DIRS})    
endif (WIN32)

if (UNIX)
    target_link_libraries(vkal PUBLIC
        Vulkan::Vulkan
        PUBLIC Vulkan::shaderc_combined
    )
endif (UNIX)

if (APPLE)
    # target_include_directories(vkal
    #     # PUBLIC /macOS/Debug/MoltenVK/include
    # )
    target_link_libraries(vkal
        # PUBLIC libMoltenVK.dylib
        PUBLIC Vulkan::Vulkan
        PUBLIC ${Vulkan_LIBRARIES} # XCode: include vulkan.framework from Vulkan SDK.
    )
endif (APPLE)
        
if (${WINDOWING} STREQUAL "VKAL_GLFW")
	target_link_libraries(vkal PUBLIC
		glfw
	)
elseif (${WINDOWING} STREQUAL "VKAL_SDL")
    target_link_libraries(vkal PUBLIC
        PUBLIC SDL2
        PUBLIC SDL2main
    )
    # target_include_directories(vkal
    #     PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../external/SDL/include
    #     PUBLIC ${Vulkan_INCLUDE_DIRS}
    # )     
    
endif()	

# Add example projects if set to ON
if (BUILD_EXAMPLES)
	add_subdirectory(src/examples/)
endif (BUILD_EXAMPLES)

target_compile_definitions(vkal PUBLIC ${WINDOWING})

# add_dependencies(GLFW_Texture       vkal)
# add_dependencies(GLFW_ModelLoading  vkal)
# add_dependencies(SDL_HelloTriangle  vkal)
# add_dependencies(GLFW_HelloTriangle vkal)







