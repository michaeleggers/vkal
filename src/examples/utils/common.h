#ifndef _COMMON_H_
#define _COMMON_H_

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext.hpp>

// NOTE: Vulkan's y is down in NDC.
//       See: https://anki3d.org/vulkan-coordinate-system/
//            https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
//       This matrix adjusts this before going to ClipSpace (see the -1 at [1, 1]).
const glm::mat4 adjust_y_for_vulkan_ndc(
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, -1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f);

#endif