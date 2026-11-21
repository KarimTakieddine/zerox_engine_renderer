#pragma once

#include <memory_cursor.hpp>

#include "camera.h"
#include "eye.h"
#include "frustum.h"
#include "locations_descriptor.h"
#include "render_entity_bindings.h"
#include "shader.h"
#include "shader_program.h"
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
        DynamicSpan<Shader, IsConst> shaders;
        DynamicSpan<ShaderProgram, IsConst> shaderPrograms;
        DynamicSpan<char, IsConst> shaderUniformNames;
        DynamicSpan<ShaderUniformVariable, IsConst> shaderUniformVariables;
        DynamicSpan<EntityBindings, IsConst> renderEntityBindings;
        DynamicSpan<std::byte, IsConst> meshSpan;
        DynamicSpan<LocationsDescriptor, IsConst> locationsDescriptors;
        DynamicSpan<Eye, IsConst> cameraEye;
        DynamicSpan<Frustum, IsConst> cameraFrustum;
        DynamicSpan<Camera, IsConst> camera;
        DynamicSpan<unsigned int, IsConst> uniformBuffer;
        DynamicSpan<UniformBufferSegment, IsConst> uniformBufferSegments;
        DynamicSpan<uint64_t, IsConst> renderBatchCount;
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
            memory.shaderUniformNames,
            memory.shaderUniformVariables,
            memory.renderEntityBindings,
            memory.meshSpan,
            memory.locationsDescriptors,
            memory.cameraEye,
            memory.cameraFrustum,
            memory.camera,
            memory.uniformBuffer,
            memory.uniformBufferSegments,
            memory.renderBatchCount,
            memory.renderBatchSpan
        };
    }
}
