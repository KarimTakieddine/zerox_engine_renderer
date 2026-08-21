#pragma once

#include <glm/matrix.hpp>

namespace renderer
{
    struct Transform
    {
        glm::mat4 localToWorld	{ 1.0f };
        glm::mat4 localRotation	{ 1.0f };
        glm::mat4 localScale	{ 1.0f };
    };
}
