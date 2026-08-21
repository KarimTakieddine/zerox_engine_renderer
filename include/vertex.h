#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace renderer
{
    struct Vertex
    {
        glm::vec3 position  { 0.0f, 0.0f, 0.0f };
        glm::vec3 color     { 1.0f, 1.0f, 1.0f };
        glm::vec2 uv        { 0.0f, 0.0f };
    };
}
