
@echo off


set vulkan_include= %VK_SDK_PATH%\Include
set vulkan_lib_64=  %VK_SDK_PATH%\Lib

set glfw_include= ..\src\external\glfw
set glfw_lib_64=  ..\src\external\glfw\lib

set glm_include= ..\src\examples\external\glm

set flags_debug=   -std=c99 -m64 -Wall -Wextra -pedantic-errors -fextended-identifiers -Wno-unused-parameter -Wno-unused-variable -Wno-missing-braces -g -D _DEBUG -D VKAL_GLFW -I%vulkan_include% -I%glfw_include%
set flags_release=   -std=c99 -Wall -Wextra -pedantic-errors -fextended-identifiers

set clang_flags_debug= /TC /Z7 /DDEBUG /W4 /WX /MDd -Qunused-arguments
set clang_flags_debug_easy= /TC /Z7 /DDEBUG /DVKAL_GLFW /W4 /MDd -Qunused-arguments -Wno-unused-variable -Wno-unused-parameter -ferror-limit=100
set clang_flags_debug_easy_cpp= /Z7 /DDEBUG /DVKAL_GLFW /W4 /MDd -Qunused-arguments -Wno-unused-variable -Wno-unused-parameter -ferror-limit=100
set clang_flags_release= /TC /O2 /W4 /MD /DVKAL_GLFW -Qunused-arguments -Wno-unused-variable

set msvc_flags_debug= /TC /Z7 /DDEBUG /DVKAL_GLFW /W4 /MDd
set msvc_flags_debug_cpp= /Z7 /DDEBUG /DVKAL_GLFW /W0 /MDd 

set tcc_flags_debug= -Wall -g -DVKAL_GLFW -DDEBUG -m64
set tcc_flags_release= -Wall

pushd "%~dp0"
mkdir build
pushd build

REM ### GCC COMPILE ###
REM gcc %flags_debug%  ..\src\examples\textures_dynamic_descriptor.c ..\src\vkal.c ..\src\platform.c -L%vulkan_lib_64%\ -L%glfw_lib_64%\ -l:vulkan-1.lib -l:glfw3dll.lib -o gcc_dbg_dynamic_descriptor.exe
REM gcc %flags_debug%  ..\src\examples\texture.c ..\src\vkal.c ..\src\platform.c ..\src\examples\utils\tr_math.c -L%vulkan_lib_64%\ -L%glfw_lib_64%\ -l:vulkan-1.lib -l:glfw3dll.lib -o gcc_dbg_texture.exe
REM gcc %flags_debug%  ..\src\examples\textures.c ..\src\vkal.c ..\src\platform.c ..\src\examples\utils\tr_math.c -L%vulkan_lib_64%\ -L%glfw_lib_64%\ -l:vulkan-1.lib -l:glfw3dll.lib -o gcc_dbg_textures.exe
REM gcc %flags_debug%  ..\src\examples\textures_descriptorarray_push_constant.c ..\src\vkal.c ..\src\platform.c ..\src\examples\utils\tr_math.c -L%vulkan_lib_64%\ -L%glfw_lib_64%\ -l:vulkan-1.lib -l:glfw3dll.lib -o gcc_dbg_textures_descriptorarray_push_constant.exe
REM gcc %flags_debug%  ..\src\examples\rendertexture.c ..\src\vkal.c ..\src\platform.c -L%vulkan_lib_64%\ -L%glfw_lib_64%\ -l:vulkan-1.lib -l:glfw3dll.lib -o gcc_dbg_rendertexture.exe
REM gcc %flags_debug%  ..\src\examples\model_loading.c ..\src\examples\utils\tr_math.c ..\src\examples\utils\model.c ..\src\vkal.c ..\src\platform.c -L%vulkan_lib_64%\ -L%glfw_lib_64%\ -l:vulkan-1.lib -l:glfw3dll.lib -o gcc_dbg_model_loading.exe
REM gcc %flags_debug%  ..\src\examples\model_loading_md.c ..\src\examples\utils\tr_math.c ..\src\examples\utils\model.c ..\src\vkal.c ..\src\platform.c -L%vulkan_lib_64%\ -L%glfw_lib_64%\ -l:vulkan-1.lib -l:glfw3dll.lib -o gcc_dbg_model_loading_md.exe


REM ### CLANG COMPILE ###
REM clang-cl %clang_flags_debug_easy%  ..\src\examples\textures_dynamic_descriptor.c ..\src\vkal.c ..\src\platform.c ..\src\examples\utils\tr_math.c -o clang_dbg_textures_dynamic_descriptor.exe /I%vulkan_include% /I%glfw_include% /link %vulkan_lib_64%\vulkan-1.lib %glfw_lib_64%\glfw3dll.lib
REM clang-cl %clang_flags_debug_easy%  ..\src\examples\texture.c ..\src\vkal.c ..\src\platform.c ..\src\examples\utils\tr_math.c -o clang_dbg_texture.exe /I%vulkan_include% /I%glfw_include% /link %vulkan_lib_64%\vulkan-1.lib %glfw_lib_64%\glfw3dll.lib
REM clang-cl %clang_flags_debug_easy%  ..\src\examples\textures.c ..\src\vkal.c ..\src\platform.c ..\src\examples\utils\tr_math.c -o clang_dbg_textures.exe /I%vulkan_include% /I%glfw_include% /link %vulkan_lib_64%\vulkan-1.lib %glfw_lib_64%\glfw3dll.lib
REM clang-cl %clang_flags_debug_easy%  ..\src\examples\textures_descriptorarray_push_constant.c ..\src\vkal.c ..\src\platform.c -o clang_dbg_textures_descriptorarray_push_constant.exe /I%vulkan_include% /I%glfw_include% /link %vulkan_lib_64%\vulkan-1.lib %glfw_lib_64%\glfw3dll.lib
REM clang-cl %clang_flags_debug_easy%  ..\src\examples\rendertexture.c ..\src\vkal.c ..\src\platform.c -o clang_dbg_rendertexture.exe /I%vulkan_include% /I%glfw_include% /link %vulkan_lib_64%\vulkan-1.lib %glfw_lib_64%\glfw3dll.lib
REM clang-cl %clang_flags_debug_easy%  ..\src\examples\primitives.c ..\src\vkal.c ..\src\platform.c ..\src\examples\utils\tr_math.c -o clang_dbg_primitives.exe /I%vulkan_include% /I%glfw_include% /link %vulkan_lib_64%\vulkan-1.lib %glfw_lib_64%\glfw3dll.lib
REM clang-cl %clang_flags_debug_easy%  ..\src\examples\primitives_dynamic.c ..\src\vkal.c ..\src\platform.c ..\src\examples\utils\tr_math.c -o clang_dbg_primitives_dynamic.exe /I%vulkan_include% /I%glfw_include% /link %vulkan_lib_64%\vulkan-1.lib %glfw_lib_64%\glfw3dll.lib
REM clang-cl %clang_flags_debug_easy%  ..\src\examples\model_loading.c ..\src\examples\utils\tr_math.c ..\src\examples\utils\model.c ..\src\vkal.c ..\src\platform.c -o clang_dbg_model_loading.exe /I%vulkan_include% /I%glfw_include% /link %vulkan_lib_64%\vulkan-1.lib %glfw_lib_64%\glfw3dll.lib
REM clang-cl %clang_flags_debug_easy%  ..\src\examples\mesh_skinning.c ..\src\examples\utils\tr_math.c ..\src\examples\utils\model.c ..\src\vkal.c ..\src\platform.c -o clang_dbg_mesh_skinning.exe /I%vulkan_include% /I%glfw_include% /link %vulkan_lib_64%\vulkan-1.lib %glfw_lib_64%\glfw3dll.lib
REM clang-cl %clang_flags_debug_easy_cpp%  ..\src\examples\model_loading_md_glm.cpp ..\src\examples\utils\tr_math.c ..\src\vkal.c ..\src\platform.c -o clang_dbg_model_loading_md_glm.exe /I%vulkan_include% /I%glfw_include% /I%glm_include% /link %vulkan_lib_64%\vulkan-1.lib %glfw_lib_64%\glfw3dll.lib
REM clang-cl %clang_flags_debug_easy%  ..\src\examples\ttf_drawing.c ..\src\vkal.c ..\src\platform.c ..\src\examples\utils\tr_math.c -o clang_dbg_ttf_drawing.exe /I%vulkan_include% /I%glfw_include% /link %vulkan_lib_64%\vulkan-1.lib %glfw_lib_64%\glfw3dll.lib
REM clang-cl %clang_flags_debug_easy%  ..\src\examples\quake2_level.c ..\src\vkal.c ..\src\platform.c ..\src\examples\utils\tr_math.c ..\src\examples\utils\q2bsp.c -o clang_dbg_quake2_level.exe /I%vulkan_include% /I%glfw_include% /link %vulkan_lib_64%\vulkan-1.lib %glfw_lib_64%\glfw3dll.lib

REM ### MSVC COMPILE ###
REM cl %msvc_flags_debug%  ..\src\examples\textures_dynamic_descriptor.c ..\src\examples\utils\tr_math.c ..\src\vkal.c ..\src\platform.c -o msvc_dbg_textures_dynamic_descriptor.exe /I%vulkan_include% /I%glfw_include% /link %vulkan_lib_64%\vulkan-1.lib %glfw_lib_64%\glfw3dll.lib
REM cl %msvc_flags_debug%  ..\src\examples\texture.c ..\src\examples\utils\tr_math.c ..\src\vkal.c ..\src\platform.c -o msvc_dbg_texture.exe /I%vulkan_include% /I%glfw_include% /link %vulkan_lib_64%\vulkan-1.lib %glfw_lib_64%\glfw3dll.lib
REM cl %msvc_flags_debug%  ..\src\examples\textures.c ..\src\examples\utils\tr_math.c ..\src\vkal.c ..\src\platform.c -o msvc_dbg_textures.exe /I%vulkan_include% /I%glfw_include% /link %vulkan_lib_64%\vulkan-1.lib %glfw_lib_64%\glfw3dll.lib
REM cl %msvc_flags_debug%  ..\src\examples\textures_descriptorarray_push_constant.c ..\src\examples\utils\tr_math.c ..\src\vkal.c ..\src\platform.c -o msvc_dbg_textures_descriptorarray_push_constant.exe /I%vulkan_include% /I%glfw_include% /link %vulkan_lib_64%\vulkan-1.lib %glfw_lib_64%\glfw3dll.lib
REM cl %msvc_flags_debug%  ..\src\examples\rendertexture.c ..\src\examples\utils\tr_math.c ..\src\vkal.c ..\src\platform.c -o msvc_dbg_rendertexture.exe /I%vulkan_include% /I%glfw_include% /link %vulkan_lib_64%\vulkan-1.lib %glfw_lib_64%\glfw3dll.lib
REM cl %msvc_flags_debug%  ..\src\examples\model_loading.c ..\src\examples\utils\tr_math.c ..\src\examples\utils\model.c ..\src\vkal.c ..\src\platform.c -o msvc_dbg_model_loading.exe /I%vulkan_include% /I%glfw_include% /link %vulkan_lib_64%\vulkan-1.lib %glfw_lib_64%\glfw3dll.lib
REM cl %msvc_flags_debug%  ..\src\examples\model_loading_md.c ..\src\examples\utils\tr_math.c ..\src\examples\utils\model.c ..\src\vkal.c ..\src\platform.c -o msvc_dbg_model_loading_md.exe /I%vulkan_include% /I%glfw_include% /link %vulkan_lib_64%\vulkan-1.lib %glfw_lib_64%\glfw3dll.lib
REM cl %msvc_flags_debug_cpp%  ..\src\examples\model_loading_md_glm.cpp ..\src\examples\utils\tr_math.c ..\src\vkal.c ..\src\platform.c -o msvc_dbg_model_loading_md_glm.exe /I%vulkan_include% /I%glfw_include% /I%glm_include% /link %vulkan_lib_64%\vulkan-1.lib %glfw_lib_64%\glfw3dll.lib
cl %msvc_flags_debug%  ..\src\examples\quake2_level.c ..\src\examples\utils\tr_math.c ..\src\examples\utils\q2bsp.c ..\src\vkal.c ..\src\platform.c -o msvc_dbg_quake2_level.exe /I%vulkan_include% /I%glfw_include% /link %vulkan_lib_64%\vulkan-1.lib %glfw_lib_64%\glfw3dll.lib

REM ### TCC COMPILE - NOTE: gl.h not found at the moment. not sure why it even wants the gl.h file! ###
REM tcc %tcc_flags_debug%   ..\src\main.c -o tcc_dbg_main.exe
REM tcc %tcc_flags_debug%   ..\src\examples\model_loading.c ..\src\examples\utils\tr_math.c ..\src\examples\utils\model.c ..\src\vkal.c ..\src\platform.c -o tcc_dbg_model_loading.exe -I%vulkan_include% -I%glfw_include% -l%vulkan_lib_64%\vulkan-1.lib -l%glfw_lib_64%\glfw3dll.lib
REM tcc %tcc_flags_release% ..\src\main.c -o tcc_rel_main.exe


popd
popd


