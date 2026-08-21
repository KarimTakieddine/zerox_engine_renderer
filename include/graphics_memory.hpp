#pragma once

#include <memory_cursor.hpp>

#include "camera.h"
#include "eye.h"
#include "frustum.h"
#include "locations_descriptor.h"
#include "spans.hpp"
#include "uniform_buffer_segment.h"

namespace renderer
{
    template<bool IsConst = false>
    struct GraphicsMemory
    {
        DynamicSpan<unsigned int, IsConst> bufferObjects;
        DynamicSpan<unsigned int, IsConst> vertexArrayObjects;
        DynamicSpan<unsigned int, IsConst> textures;
        DynamicSpan<unsigned int, IsConst> shaders;
        DynamicSpan<unsigned int, IsConst> shaderPrograms;
        DynamicSpan<std::byte, IsConst> meshSpan;
        DynamicSpan<LocationsDescriptor, IsConst> locationsDescriptors;
        DynamicSpan<Eye, IsConst> cameraEye;
        DynamicSpan<Frustum, IsConst> cameraFrustum;
        DynamicSpan<Camera, IsConst> camera;
        DynamicSpan<unsigned int, IsConst> uniformBuffer;
        DynamicSpan<UniformBufferSegment, IsConst> uniformBufferSegments;
        DynamicSpan<std::byte, IsConst> renderBatchSpan;
    };

    using ConstGraphicsMemory   = GraphicsMemory<true>;
    using MutableGraphicsMemory = GraphicsMemory<false>;

    inline ConstGraphicsMemory freezeGraphicsMemory(const MutableGraphicsMemory& memory)
    {
        return {
            memory.bufferObjects,
            memory.vertexArrayObjects,
            memory.textures,
            memory.shaders,
            memory.shaderPrograms,
            memory.meshSpan,
            memory.locationsDescriptors,
            memory.cameraEye,
            memory.cameraFrustum,
            memory.camera,
            memory.uniformBuffer,
            memory.uniformBufferSegments,
            memory.renderBatchSpan
        };
    }
}
