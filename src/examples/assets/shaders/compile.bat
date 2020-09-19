@echo off

%VULKAN_SDK%/bin/glslc textures.vert -o textures_vert.spv
%VULKAN_SDK%/bin/glslc textures.frag -o textures_frag.spv

%VULKAN_SDK%/bin/glslc textures_descriptorarray.vert -o textures_descriptorarray_vert.spv
%VULKAN_SDK%/bin/glslc textures_descriptorarray.frag -o textures_descriptorarray_frag.spv
                         