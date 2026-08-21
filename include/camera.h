#pragma once

#include <glm/vec3.hpp>
#include <glm/matrix.hpp>

namespace renderer
{
    struct Camera
    {
        glm::mat4 localToWorld	{ 1.0f };
        glm::mat4 localRotation	{ 1.0f };
        glm::mat4 view			{ 1.0f };
        glm::mat4 projection	{ 1.0f };
    };
}
