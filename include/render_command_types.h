#pragma once

#include <cstdint>
#include <cstddef>

#include "material.h"
#include "render_batch.h"
#include "render_command.h"
#include "transform.h"

namespace renderer
{
    struct alignas(16) AllocateBatchCommand
    {
        uint64_t shaderProgramIndex { 0 };
        uint64_t vertexArrayIndex   { 0 };
        uint64_t locationsIndex     { 0 };
        uint64_t meshIndex          { 0 };
        uint64_t entityCount        { 0 };
        uint64_t drawMode           { 0 };
    };

    struct alignas(16) UpdateMaterialCommand
    {
        Material material;
        uint64_t batchIndex { 0 };
        uint64_t entityIndex{ 0 };
    };

    struct alignas(16) UpdateTransformCommand
    {
        Transform transform;
        uint64_t batchIndex { 0 };
        uint64_t entityIndex{ 0 };
    };

    struct alignas(16) SetClientAreaCommand
    {
        uint64_t width  { 0 };
        uint64_t height { 0 };
    };

    struct alignas(16) SetTextureLayerCommand
    {
        uint64_t batchIndex { 0 };
        uint64_t entityIndex{ 0 };
        uint64_t layer      { 0 };

    private:
        uint64_t reserved   { 0 };
    };
}
