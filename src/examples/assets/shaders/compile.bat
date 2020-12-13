@echo off

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

%VULKAN_SDK%/bin/glslc q2bsp.vert -o q2bsp_vert.spv
%VULKAN_SDK%/bin/glslc q2bsp.frag -o q2bsp_frag.spv

%VULKAN_SDK%/bin/glslc q2bsp_sky.vert -o q2bsp_sky_vert.spv
%VULKAN_SDK%/bin/glslc q2bsp_sky.frag -o q2bsp_sky_frag.spv

%VULKAN_SDK%/bin/glslc q2bsp_trans.vert -o q2bsp_trans_vert.spv
%VULKAN_SDK%/bin/glslc q2bsp_trans.frag -o q2bsp_trans_frag.spv


REM CMAKE SHADERS
REM TODO: HOW TO MAKE THIS BETTER?
%VULKAN_SDK%/bin/glslc texture.vert -o ..\..\..\..\cmakebuild\shaders\texture_vert.spv
%VULKAN_SDK%/bin/glslc texture.frag -o ..\..\..\..\cmakebuild\shaders\texture_frag.spv

%VULKAN_SDK%/bin/glslc textures.vert -o ..\..\..\..\cmakebuild\shaders\textures_vert.spv
%VULKAN_SDK%/bin/glslc textures.frag -o ..\..\..\..\cmakebuild\shaders\textures_frag.spv

%VULKAN_SDK%/bin/glslc textures_dynamic_descriptor.vert -o ..\..\..\..\cmakebuild\shaders\textures_dynamic_descriptor_vert.spv
%VULKAN_SDK%/bin/glslc textures_dynamic_descriptor.frag -o ..\..\..\..\cmakebuild\shaders\textures_dynamic_descriptor_frag.spv
          
%VULKAN_SDK%/bin/glslc textures_descriptorarray_push_constant.vert -o ..\..\..\..\cmakebuild\shaders\textures_descriptorarray_push_constant_vert.spv
%VULKAN_SDK%/bin/glslc textures_descriptorarray_push_constant.frag -o ..\..\..\..\cmakebuild\shaders\textures_descriptorarray_push_constant_frag.spv          

%VULKAN_SDK%/bin/glslc rendertexture.vert           -o ..\..\..\..\cmakebuild\shaders\rendertexture_vert.spv
%VULKAN_SDK%/bin/glslc rendertexture.frag           -o ..\..\..\..\cmakebuild\shaders\rendertexture_frag.spv
%VULKAN_SDK%/bin/glslc rendertexture_composite.frag -o ..\..\..\..\cmakebuild\shaders\rendertexture_composite_frag.spv

%VULKAN_SDK%/bin/glslc model_loading.vert -o ..\..\..\..\cmakebuild\shaders\model_loading_vert.spv
%VULKAN_SDK%/bin/glslc model_loading.frag -o ..\..\..\..\cmakebuild\shaders\model_loading_frag.spv

%VULKAN_SDK%/bin/glslc model_loading_md.vert -o ..\..\..\..\cmakebuild\shaders\model_loading_md_vert.spv
%VULKAN_SDK%/bin/glslc model_loading_md.frag -o ..\..\..\..\cmakebuild\shaders\model_loading_md_frag.spv

%VULKAN_SDK%/bin/glslc primitives.vert -o ..\..\..\..\cmakebuild\shaders\primitives_vert.spv
%VULKAN_SDK%/bin/glslc primitives.frag -o ..\..\..\..\cmakebuild\shaders\primitives_frag.spv

%VULKAN_SDK%/bin/glslc primitives_textured_rect.vert -o ..\..\..\..\cmakebuild\shaders\primitives_textured_rect_vert.spv
%VULKAN_SDK%/bin/glslc primitives_textured_rect.frag -o ..\..\..\..\cmakebuild\shaders\primitives_textured_rect_frag.spv

%VULKAN_SDK%/bin/glslc ttf_drawing.vert -o ..\..\..\..\cmakebuild\shaders\ttf_drawing_vert.spv
%VULKAN_SDK%/bin/glslc ttf_drawing.frag -o ..\..\..\..\cmakebuild\shaders\ttf_drawing_frag.spv

%VULKAN_SDK%/bin/glslc q2bsp.vert -o ..\..\..\..\cmakebuild\shaders\q2bsp_vert.spv
%VULKAN_SDK%/bin/glslc q2bsp.frag -o ..\..\..\..\cmakebuild\shaders\q2bsp_frag.spv

%VULKAN_SDK%/bin/glslc q2bsp_sky.vert -o ..\..\..\..\cmakebuild\shaders\q2bsp_sky_vert.spv
%VULKAN_SDK%/bin/glslc q2bsp_sky.frag -o ..\..\..\..\cmakebuild\shaders\q2bsp_sky_frag.spv

%VULKAN_SDK%/bin/glslc q2bsp_trans.vert -o ..\..\..\..\cmakebuild\shaders\q2bsp_trans_vert.spv
%VULKAN_SDK%/bin/glslc q2bsp_trans.frag -o ..\..\..\..\cmakebuild\shaders\q2bsp_trans_frag.spv
