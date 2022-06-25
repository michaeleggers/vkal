@echo off

cd %~dp0

echo "COMPILING SHADERS..."

%VULKAN_SDK%/bin/glslc hello_triangle.vert -o hello_triangle_vert.spv
%VULKAN_SDK%/bin/glslc hello_triangle.frag -o hello_triangle_frag.spv

%VULKAN_SDK%/bin/glslc texture.vert -o texture_vert.spv
%VULKAN_SDK%/bin/glslc texture.frag -o texture_frag.spv

%VULKAN_SDK%/bin/glslc textures.vert -o textures_vert.spv
%VULKAN_SDK%/bin/glslc textures.frag -o textures_frag.spv

%VULKAN_SDK%/bin/glslc textures_dynamic_descriptor.vert -o textures_dynamic_descriptor_vert.spv
%VULKAN_SDK%/bin/glslc textures_dynamic_descriptor.frag -o textures_dynamic_descriptor_frag.spv
          
%VULKAN_SDK%/bin/glslc textures_descriptorarray_push_constant.vert -o textures_descriptorarray_push_constant_vert.spv
%VULKAN_SDK%/bin/glslc textures_descriptorarray_push_constant.frag -o textures_descriptorarray_push_constant_frag.spv          

%VULKAN_SDK%/bin/glslc rendertexture.vert           -o rendertexture_vert.spv
%VULKAN_SDK%/bin/glslc rendertexture.frag           -o rendertexture_frag.spv
%VULKAN_SDK%/bin/glslc rendertexture_composite.frag -o rendertexture_composite_frag.spv

%VULKAN_SDK%/bin/glslc model_loading.vert -o model_loading_vert.spv
%VULKAN_SDK%/bin/glslc model_loading.frag -o model_loading_frag.spv

%VULKAN_SDK%/bin/glslc model_loading_md.vert -o model_loading_md_vert.spv
%VULKAN_SDK%/bin/glslc model_loading_md.frag -o model_loading_md_frag.spv

%VULKAN_SDK%/bin/glslc primitives.vert -o primitives_vert.spv
%VULKAN_SDK%/bin/glslc primitives.frag -o primitives_frag.spv

%VULKAN_SDK%/bin/glslc primitives_textured_rect.vert -o primitives_textured_rect_vert.spv
%VULKAN_SDK%/bin/glslc primitives_textured_rect.frag -o primitives_textured_rect_frag.spv

%VULKAN_SDK%/bin/glslc ttf_drawing.vert -o ttf_drawing_vert.spv
%VULKAN_SDK%/bin/glslc ttf_drawing.frag -o ttf_drawing_frag.spv

%VULKAN_SDK%/bin/glslc imgui.vert -o imgui_vert.spv
%VULKAN_SDK%/bin/glslc imgui.frag -o imgui_frag.spv
