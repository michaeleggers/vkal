
cmake_minimum_required(VERSION 3.10)

project(VKAL VERSION 1.0)

cmake_minimum_required(VERSION 3.12)

find_package(Vulkan REQUIRED)


###########################################################
# Libraries, VKAL and its dependencies (GLFW, SDL2,...)
###########################################################

# GLFW
add_library(glfw SHARED IMPORTED) 
set_property(TARGET glfw PROPERTY 
             IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/src/external/glfw/lib/glfw3.dll)
set_property(TARGET glfw PROPERTY 
             IMPORTED_IMPLIB ${CMAKE_SOURCE_DIR}/src/external/glfw/lib/glfw3dll.lib )
SET_PROPERTY(TARGET glfw PROPERTY FOLDER "Libraries")
              
# Vulkan Abstraction Layer (VKAL)
add_compile_definitions(VKAL_GLFW)
add_library(libvkal
    src/vkal.c
    src/vkal.h
)
target_include_directories(libvkal
    PUBLIC src/external/glfw/ Vulkan::Vulkan
)
target_link_libraries(libvkal PUBLIC
    Vulkan::Vulkan 
    glfw
)
SET_PROPERTY(TARGET libvkal PROPERTY FOLDER "Libraries")

###########################################################
# Libraries, Examples
###########################################################

#PhysFS
add_library(physfs SHARED IMPORTED)
set_property( TARGET physfs PROPERTY 
              IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/src/examples/external/physfs-3.0.2/build/Debug/physfs.dll )
set_property( TARGET physfs PROPERTY 
              IMPORTED_IMPLIB ${CMAKE_SOURCE_DIR}/src/examples/external/physfs-3.0.2/build/Debug/physfs.lib )
SET_PROPERTY(TARGET physfs PROPERTY FOLDER "Libraries")

###########################################################
# Example Executables
###########################################################

add_executable(Quake2Level
    src/examples/q2bsp/quake2_level.c
    src/examples/q2bsp/q2_io.c
    src/examples/q2bsp/q2_io.h
    src/examples/q2bsp/q2_common.c
    src/examples/q2bsp/q2_common.h
    src/examples/q2bsp/q2_r_local.c
    src/examples/q2bsp/q2_r_local.h
    src/examples/q2bsp/q2_vk_render.h
    src/examples/q2bsp/q2_vk_render.c
    src/examples/q2bsp/q2_e_parser.c    
    src/examples/q2bsp/q2_e_parser.h
    src/examples/q2bsp/q2bsp.c
    src/examples/q2bsp/q2bsp.h
    
    src/examples/utils/tr_math.c
    src/platform.c
    src/platform.h
)
target_include_directories(Quake2Level
    PUBLIC src/examples/external/stb  src/examples/external/physfs-3.0.2/src src/external/glfw/
)
target_link_libraries(Quake2Level 
    PUBLIC libvkal
    PRIVATE glfw
    PRIVATE physfs
)
set_property(TARGET Quake2Level PROPERTY FOLDER "Examples")
