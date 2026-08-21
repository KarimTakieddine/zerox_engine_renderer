#pragma once

#include "mesh.hpp"
#include "mesh_buffer_indices.h"
#include "spans.hpp"
#include "vertex.h"

namespace renderer
{
    template<bool IsConst = false>
    struct MeshSpan
    {
        DynamicSpan<MeshBufferIndices, IsConst> bufferIndices;
        DynamicSpan<Vertex, IsConst> vertices;
        DynamicSpan<unsigned int, IsConst> triangles;
    };
}
