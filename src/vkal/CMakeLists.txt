cmake_minimum_required(VERSION 3.10)
project(VKAL VERSION 1.0)
cmake_minimum_required(VERSION 3.12)

if (WIN32)
    find_package(Vulkan REQUIRED)
endif (WIN32)

add_library(libvkal STATIC
    vkal.c
    vkal.h
)

if (WIN32)
    target_include_directories(libvkal
        PUBLIC Vulkan::Vulkan
    )
    target_link_libraries(libvkal PUBLIC
        Vulkan::Vulkan 
    )
endif (WIN32)

if (APPLE)
    target_include_directories(libvkal
        PUBLIC /macOS/Debug/MoltenVK/include
    )
    target_link_libraries(libvkal
        PUBLIC /macOS/Debug/dylib/macOS/libMoltenVK.dylib
    )
endif (APPLE)

if (${WINDOWING} STREQUAL "VKAL_GLFW")
	target_link_libraries(libvkal PUBLIC
		glfw
	)
endif()	
target_compile_definitions(libvkal PUBLIC ${WINDOWING})