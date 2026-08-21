#pragma once

#include <glm/vec3.hpp>

namespace renderer
{
    struct Eye
    {
        glm::vec3 position  { 0.0f, 0.0f, 0.0f };
        glm::vec3 target    { 0.0f, 0.0f, 0.0f };
        glm::vec3 up        { 0.0f, 0.0f, 0.0f };
    };
}
