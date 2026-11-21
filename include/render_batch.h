#pragma once

#include <cstdint>

namespace renderer
{
    enum class DrawMode : uint64_t
    {
        TRIANGLES   = 0,
        LINES       = 1
    };

    struct alignas(16) RenderBatch
    {
        size_t entityCount      { 0 };
        DrawMode drawMode       { DrawMode::TRIANGLES };
        int meshIndex           { -1 };
        int shaderProgramIndex  { -1 };
        int vertexArrayIndex    { -1 };
        int elementCount        { 0 };
    };
}
