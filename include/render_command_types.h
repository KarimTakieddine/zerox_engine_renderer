#pragma once

#include <cstdint>
#include <cstddef>

#include <zerox_material.h>
#include <zerox_texture.h>
#include <zerox_transform.h>

#include "render_batch.h"

namespace renderer
{
    using AllocateBatchCommand = RenderBatch;

    struct alignas(16) UpdateMaterialCommand
    {
        ZeroXBridge::Material material;
        uint64_t batchIndex { 0 };
        uint64_t entityIndex{ 0 };
    };

    struct alignas(16) UpdateTransformCommand
    {
        ZeroXBridge::Transform transform;
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
