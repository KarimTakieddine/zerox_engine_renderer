#pragma once

#include <cstddef>

namespace renderer
{
    struct RenderBatch
    {
        size_t descriptorIndex      { 0 };
        unsigned int shaderProgram  { 0 };
        unsigned int vertexArray    { 0 };
        int elememtCount            { 0 };
    };
}
