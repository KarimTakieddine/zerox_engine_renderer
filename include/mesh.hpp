#pragma once

#include "vertex.h"

namespace renderer
{
    struct Mesh
    {
        Vertex* vertices        { nullptr };
        unsigned int* triangles { nullptr };
        size_t vertexCount      { 0 };
        size_t triangleCount    { 0 };
    };
}
