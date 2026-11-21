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
        DynamicSpan<std::byte, IsConst> meshSpan;
        DynamicSpan<Shader, IsConst> shaders;
        DynamicSpan<ShaderProgram, IsConst> shaderPrograms;
        DynamicSpan<Eye, IsConst> cameraEye;
        DynamicSpan<Frustum, IsConst> cameraFrustum;
        DynamicSpan<Camera, IsConst> camera;
        DynamicSpan<unsigned int, IsConst> uniformBuffer;
        DynamicSpan<UniformBufferSegment, IsConst> uniformBufferSegments;
        DynamicSpan<char, IsConst> shaderAttributeNames;
        DynamicSpan<ShaderVariable, IsConst> shaderAttributes;
        DynamicSpan<char, IsConst> shaderUniformNames;
        DynamicSpan<ShaderVariable, IsConst> shaderUniforms;
        DynamicSpan<EntityBindings, IsConst> renderEntityBindings;
        DynamicSpan<LocationsDescriptor, IsConst> locationsDescriptors;
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
            memory.meshSpan,
            memory.shaders,
            memory.shaderPrograms,
            memory.cameraEye,
            memory.cameraFrustum,
            memory.camera,
            memory.uniformBuffer,
            memory.uniformBufferSegments,
            memory.shaderAttributeNames,
            memory.shaderAttributes,
            memory.shaderUniformNames,
            memory.shaderUniforms,
            memory.renderEntityBindings,
            memory.locationsDescriptors,
            memory.renderBatchCount,
            memory.renderBatchSpan
        };
    }
}
