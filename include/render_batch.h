#pragma once

#include <cstdint>

namespace renderer
{
    enum class DrawMode : unsigned int
    {
        TRIANGLES   = 0,
        LINES       = 1
    };

    struct alignas(16) RenderBatch
    {
        uint64_t descriptorIndex    { 0 };
        unsigned int shaderProgram  { 0 };
        unsigned int vertexArray    { 0 };
        int elementCount            { 0 };
        DrawMode drawMode           { DrawMode::TRIANGLES };
    };
}
