cmake_minimum_required(VERSION 3.10)
project(VKAL VERSION 1.0)
cmake_minimum_required(VERSION 3.12)
find_package(Vulkan REQUIRED)




add_library(libvkal STATIC
    vkal.c
    vkal.h
)
target_include_directories(libvkal
    PUBLIC Vulkan::Vulkan
)
target_link_libraries(libvkal PUBLIC
    Vulkan::Vulkan 
)
if (${WINDOWING} STREQUAL "VKAL_GLFW")
	target_link_libraries(libvkal PUBLIC
		glfw
	)
endif()	
target_compile_definitions(libvkal PUBLIC ${WINDOWING})