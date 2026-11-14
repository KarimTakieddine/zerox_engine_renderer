#pragma once

#include <cstdint>
#include <cstddef>

#include <zerox_material.h>
#include <zerox_texture.h>
#include <zerox_transform.h>

#include "render_batch.h"
#include "render_command.h"

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

    inline constexpr size_t IMAGE_PATH_SIZE{ 240 };

    struct alignas(16) UploadTextureCommand
    {
        char path[IMAGE_PATH_SIZE];

        ZeroXBridge::Texture texture;
    };

    inline constexpr size_t IMAGE_PATH_ENTRY_SIZE   { 32 };
    inline constexpr size_t MAX_TEXTURE_ARRAY_PATHS { 7 };

    struct UploadTextureArrayCommand
    {
        char paths[MAX_TEXTURE_ARRAY_PATHS][IMAGE_PATH_ENTRY_SIZE];

        ZeroXBridge::Texture texture;
        uint64_t textureCount   { 0 };
        uint64_t reserved       { 0 };
    };
}
