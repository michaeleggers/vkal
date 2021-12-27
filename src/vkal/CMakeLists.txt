cmake_minimum_required(VERSION 3.10)
project(VKAL VERSION 1.0)
cmake_minimum_required(VERSION 3.12)


if (WIN32)
    find_package(Vulkan REQUIRED)
endif (WIN32)

if (APPLE)
    find_package(Vulkan REQUIRED)
endif (APPLE)

message("Cmake Module Path: " ${Vulkan_INCLUDE_DIRS})

add_library(vkal STATIC
    vkal.c
    vkal.h
)

if (WIN32)
    target_include_directories(vkal
        PUBLIC Vulkan::Vulkan
        # if (${WINDOWING} STREQUAL "VKAL_SDL")
	    #     PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../external/SDL/include
        # endif()
    )
    target_link_libraries(vkal PUBLIC
        Vulkan::Vulkan
    )
endif (WIN32)

if (APPLE)
    target_include_directories(vkal
        # PUBLIC /macOS/Debug/MoltenVK/include
        PUBLIC Vulkan::Vulkan
    )
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
endif()	

target_compile_definitions(vkal PUBLIC ${WINDOWING})