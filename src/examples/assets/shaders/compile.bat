@echo off

%VULKAN_SDK%/bin/glslc textures.vert -o textures_vert.spv
%VULKAN_SDK%/bin/glslc textures.frag -o textures_frag.spv

%VULKAN_SDK%/bin/glslc textures_descriptorarray.vert -o textures_descriptorarray_vert.spv
%VULKAN_SDK%/bin/glslc textures_descriptorarray.frag -o textures_descriptorarray_frag.spv
          
%VULKAN_SDK%/bin/glslc textures_descriptorarray_push_constant.vert -o textures_descriptorarray_push_constant_vert.spv
%VULKAN_SDK%/bin/glslc textures_descriptorarray_push_constant.frag -o textures_descriptorarray_push_constant_frag.spv          

%VULKAN_SDK%/bin/glslc rendertexture.vert -o rendertexture_vert.spv
%VULKAN_SDK%/bin/glslc rendertexture.frag -o rendertexture_frag.spv
