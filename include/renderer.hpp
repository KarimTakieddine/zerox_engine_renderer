#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>

#include <bump_allocator.hpp>
#include <memory_cursor.hpp>
#include <memory_view.hpp>

#include <zerox_texture.h>

#include "graphics_memory.hpp"
#include "mesh_span.hpp"
#include "render_batch.h"
#include "render_command_types.h"

namespace ZeroXBridge
{
    struct RenderEntity;
}

namespace renderer
{
    struct LocationsDescriptor;
    struct Eye;
    struct Frustum;
    struct Shader;
    struct Command;
    struct RenderBuffer;

    static constexpr size_t ALLOCATOR_SIZE      = 1 << 14;
    static constexpr uint64_t MEMORY_ALIGNMENT  = 16;

    using Allocator = BumpAllocator<16>;

    struct ShaderCompileStep
    {
        const size_t* shaderIndices { nullptr };
        size_t shaderCount          { 0 };
    };

    struct ShaderLocationsLink
    {
        size_t locationsIndex       { 0 };
        size_t shaderProgramIndex   { 0 };
    };

    struct alignas(16) TextureArrayConfig
    {
        ZeroXBridge::Texture data;
        const char* const* paths{ nullptr };
        size_t textureCount     { 0 };
    };

    struct TextureConfig
    {
        ZeroXBridge::Texture texture;
        const char* path{ nullptr };
    };

    struct GraphicsConfig
    {
        const Mesh* meshes                              { nullptr };
        const TextureConfig* textures                   { nullptr };
        const ShaderLoadInfo* shaders                   { nullptr };
        const Eye* cameraEye                            { nullptr };
        const Frustum* cameraFrustum                    { nullptr };
        const char* const cameraUniformBuffer           { nullptr };
        const char* const* cameraUniformNames           { nullptr };
        const ShaderCompileStep* shaderCompileSteps     { nullptr };
        const TextureArrayConfig* textureArrays         { nullptr };
        size_t meshCount                                { 0 };
        size_t bufferCount                              { 0 };
        size_t vertexArrayCount                         { 0 };
        size_t textureCount                             { 0 };
        size_t textureArrayCount                        { 0 };
        size_t shaderCount                              { 0 };
        size_t shaderProgramCount                       { 0 };
        size_t compileStepCount                         { 0 };
    };

    template<bool IsConst = false>
    inline GraphicsMemory<IsConst> readGraphicsMemory(std::conditional_t<IsConst, const Allocator*, Allocator*> allocator)
    {
        GraphicsMemory<IsConst> graphicsMemory;

        // TODO(Karim): Maybe just dynamically get sizeInBytes here?

        FixedSpan<std::byte, IsConst, ALLOCATOR_SIZE> allocatorSpan(reinterpret_cast<std::conditional_t<IsConst, const std::byte*, std::byte*>>(allocator->peek()), ALLOCATOR_SIZE);

        std::conditional_t<IsConst, ConstMemoryView, MutableMemoryView> memoryView(allocatorSpan);

        MemoryCursor<MEMORY_ALIGNMENT> cursor;

        /*
            "When the name of a member template specialization appears after . or -> in a postfix-expression, or after nested-name-specifier
            in a qualified-id, and the postfix-expression or qualified-id explicitly depends on a template-parameter, the member template
            name must be prefixed by the keyword template."
        */

        DynamicSpan<unsigned int, IsConst> bufferObjects    = memoryView.template read_contiguous_array<unsigned int>(cursor.getOffset());
        graphicsMemory.bufferObjects                        = bufferObjects;
        cursor.step_array<unsigned int>(bufferObjects.size());

        DynamicSpan<unsigned int, IsConst> vertexArrayObjects   = memoryView.template read_contiguous_array<unsigned int>(cursor.getOffset());
        graphicsMemory.vertexArrayObjects                       = vertexArrayObjects;
        cursor.step_array<unsigned int>(vertexArrayObjects.size());

        DynamicSpan<unsigned int, IsConst> textures = memoryView.template read_contiguous_array<unsigned int>(cursor.getOffset());
        graphicsMemory.textures                     = textures;
        cursor.step_array<unsigned int>(textures.size());

        const uint64_t meshDataOffset   = cursor.getOffset();
        const uint64_t meshCount        = *memoryView.template read_object<uint64_t>(meshDataOffset).data();
        cursor.step<uint64_t>();
        
        for (uint64_t i = 0; i < meshCount; ++i)
        {
            cursor.step<MeshBufferIndices>();
            
            const uint64_t vertexCount = *memoryView.template read_object<uint64_t>(cursor.getOffset()).data();
            cursor.step_array<Vertex>(vertexCount);

            const uint64_t triangleCount = *memoryView.template read_object<uint64_t>(cursor.getOffset()).data();
            cursor.step_array<unsigned int>(triangleCount);
        }

        graphicsMemory.meshSpan = allocatorSpan.subspan(meshDataOffset, cursor.getOffset() - meshDataOffset);

        DynamicSpan<Shader, IsConst> shaders    = memoryView.template read_contiguous_array<Shader>(cursor.getOffset());
        graphicsMemory.shaders                  = shaders;
        cursor.step_array<Shader>(shaders.size());

        DynamicSpan<ShaderProgram, IsConst> shaderPrograms  = memoryView.template read_contiguous_array<ShaderProgram>(cursor.getOffset());
        graphicsMemory.shaderPrograms                       = shaderPrograms;
        cursor.step_array<ShaderProgram>(shaderPrograms.size());

        DynamicSpan<Eye, IsConst> cameraEye = memoryView.template read_object<Eye>(cursor.getOffset());
        graphicsMemory.cameraEye            = cameraEye;
        cursor.step<Eye>();

        DynamicSpan<Frustum, IsConst> cameraFrustum = memoryView.template read_object<Frustum>(cursor.getOffset());
        graphicsMemory.cameraFrustum                = cameraFrustum;
        cursor.step<Frustum>();

        DynamicSpan<Camera, IsConst> camera = memoryView.template read_object<Camera>(cursor.getOffset());
        graphicsMemory.camera               = camera;
        cursor.step<Camera>();

        DynamicSpan<unsigned int, IsConst> uniformBuffer    = memoryView.template read_object<unsigned int>(cursor.getOffset());
        graphicsMemory.uniformBuffer                        = uniformBuffer;
        cursor.step<unsigned int>();

        DynamicSpan<UniformBufferSegment, IsConst> uniformBufferSegments    = memoryView.template read_contiguous_array<UniformBufferSegment>(cursor.getOffset());
        graphicsMemory.uniformBufferSegments                                = uniformBufferSegments;
        cursor.step_array<UniformBufferSegment>(uniformBufferSegments.size());

        if (allocator->getLiveSizeInBytes() == cursor.getOffset())
        {
            return graphicsMemory;
        }

        DynamicSpan<char, IsConst> shaderAttributeNames = memoryView.template read_contiguous_array<char>(cursor.getOffset());
        graphicsMemory.shaderAttributeNames             = shaderAttributeNames;
        cursor.step_array<char>(shaderAttributeNames.size());

        DynamicSpan<ShaderVariable, IsConst> shaderAttributes   = memoryView.template read_contiguous_array<ShaderVariable>(cursor.getOffset());
        graphicsMemory.shaderAttributes                         = shaderAttributes;
        cursor.step_array<ShaderVariable>(shaderAttributes.size());

        DynamicSpan<char, IsConst> shaderUniformNames   = memoryView.template read_contiguous_array<char>(cursor.getOffset());
        graphicsMemory.shaderUniformNames               = shaderUniformNames;
        cursor.step_array<char>(shaderUniformNames.size());

        DynamicSpan<ShaderVariable, IsConst> shaderUniforms = memoryView.template read_contiguous_array<ShaderVariable>(cursor.getOffset());
        graphicsMemory.shaderUniforms                       = shaderUniforms;
        cursor.step_array<ShaderVariable>(shaderUniforms.size());

        if (allocator->getLiveSizeInBytes() == cursor.getOffset())
        {
            return graphicsMemory;
        }

        DynamicSpan<EntityBindings, IsConst> renderEntityBindings   = memoryView.template read_contiguous_array<EntityBindings>(cursor.getOffset());
        graphicsMemory.renderEntityBindings                         = renderEntityBindings;
        cursor.step_array<EntityBindings>(renderEntityBindings.size());

        graphicsMemory.renderBatchCount = memoryView.template read_object<uint64_t>(cursor.getOffset());
        cursor.step<uint64_t>();

        graphicsMemory.renderBatchSpan = allocatorSpan.subspan(cursor.getOffset(), allocator->getLiveSizeInBytes() - cursor.getOffset());

        return graphicsMemory;
    }

    
    void allocateBuffers(Allocator* allocator, size_t count);
    void allocateVertexArrays(Allocator* allocator, size_t count);
    void allocateTextures(Allocator* allocator, size_t count);
    void allocateShaders(Allocator* allocator, size_t count);
    void allocateShaderPrograms(Allocator* allocator, size_t count);
    void allocateMeshes(Allocator* allocator, size_t count, const Mesh* meshes);
    void allocateCamera(Allocator* allocator);
    void allocateUniformBuffer(Allocator* allocator, size_t segmentCount);
    void allocateRenderBatchCount(Allocator* allocator);
    void allocateGraphicsResources(Allocator* allocator, const GraphicsConfig* config);


    void generateBuffers(const MutableGraphicsMemory& memory);
    void generateVertexArrays(const MutableGraphicsMemory& memory);

    void generateTextures(const MutableGraphicsMemory& memory);
    bool uploadTextures(const ConstGraphicsMemory& memory, size_t count, const char* const* paths, const ZeroXBridge::Texture* textures);
    bool uploadTextureArray(const ConstGraphicsMemory& memory, const ZeroXBridge::Texture& texture, size_t textureIndex, size_t textureCount, const char* const* imageFiles);

    bool createShader(Shader* shader, Shader::Type type);
    bool compileShader(Shader* shader, const char* content);
    bool loadShader(Shader* shader, const ShaderLoadInfo* loadInfo);
    bool loadShaders(const MutableGraphicsMemory& memory, const ShaderLoadInfo* loadInfo, size_t count);
    void unloadShader(Shader* shader);
    void unloadShaders(Shader* shaders, size_t count);
    void unloadShaders(const MutableGraphicsMemory& memory);

    bool createShaderProgram(ShaderProgram* program);
    bool linkShaderProgram(const MutableGraphicsMemory& memory, ShaderProgram* program, const size_t* indices, size_t count);
    bool loadShaderProgram(const MutableGraphicsMemory& memory, ShaderProgram* program, const ShaderCompileStep* shaderCompileStep);
    bool loadShaderPrograms(const MutableGraphicsMemory& memory, const ShaderCompileStep* shaderCompileSteps, size_t compileStepCount);
    void unloadShaderProgram(const MutableGraphicsMemory& memory, ShaderProgram* program);
    void unloadShaderPrograms(const MutableGraphicsMemory& memory, ShaderProgram* programs, size_t count);
    void unloadShaderPrograms(const MutableGraphicsMemory& memory);

    bool loadShaderUniforms(const ShaderProgram* program, size_t* count, char* nameBuffer, ShaderVariable* buffer);
    bool loadShaderUniforms(Allocator* allocator, ShaderProgram* programs, size_t count);
    bool loadShaderUniforms(Allocator* allocator, const MutableGraphicsMemory& memory);

    bool loadShaderAttributes(const ShaderProgram* program, size_t* count, char* nameBuffer, ShaderVariable* buffer);
    bool loadShaderAttributes(Allocator* allocator, ShaderProgram* programs, size_t count);
    bool loadShaderAttributes(Allocator* allocator, const MutableGraphicsMemory& memory);

    bool loadEntityBindings(const ShaderProgram* program, EntityBindings* entityBindings);
    bool loadEntityBindings(Allocator* allocator, const ConstGraphicsMemory& memory, const ShaderProgram* programs, size_t count);
    bool loadEntityBindings(Allocator* allocator, const ConstGraphicsMemory& memory);

    void generateMeshes(const MutableGraphicsMemory& memory);
    void uploadMeshes(const ConstGraphicsMemory& memory);
    void uploadMesh(const MeshSpan<true>& meshSpan);
    void setCameraEye(const MutableGraphicsMemory& memory, const Eye* eye);
    void setCameraFrustum(const MutableGraphicsMemory& memory, const Frustum* frustum);
    void updateCamera(const MutableGraphicsMemory& memory);
    void setCameraAspectRatio(const MutableGraphicsMemory& memory, int width, int height);
    void setViewportArea(int width, int height);
    bool generateUniformBuffer(const MutableGraphicsMemory& memory, size_t programIndex, const char* name, const char* const* names);
    bool mapCameraUniforms(const MutableGraphicsMemory& memory);
    void uploadUniformBuffer(const ConstGraphicsMemory& memory);
    void setVertexLayout(const ShaderProgram* program, const EntityBindings* entityBindings, unsigned int vertexArray, const MeshBufferIndices* buffers);
    void initializeGraphicsResources(Allocator* allocator, const GraphicsConfig* config);
    void setTextureParameters(unsigned int texture, unsigned int target, const ZeroXBridge::Texture* parameters, bool unbind = false);
    void initializeRenderBuffer(RenderBuffer* buffer);
    void destroyRenderBuffer(const RenderBuffer* buffer);
    void bindRenderBuffer(const RenderBuffer* buffer);


    void freeBuffers(const MutableGraphicsMemory& memory);
    void freeVertexArrays(const MutableGraphicsMemory& memory);
    void freeTextures(const MutableGraphicsMemory& memory);
    void freeShaders(const MutableGraphicsMemory& memory);
    void freeGraphicsResources(const MutableGraphicsMemory& memory);


    void initializeGraphicsState();
    void clearFrameBuffer();
    void renderEntity(const ZeroXBridge::RenderEntity* entity, const EntityBindings* entityBindings, DrawMode drawMode, int elementCount, const glm::vec2& pixelsPerUnit);
    void renderBatch(const ShaderProgram* shaderProgram, unsigned int vertexArray);
    void renderBatches(const ConstGraphicsMemory& memory, const glm::vec2& pixelsPerUnit);
    void render(const ConstGraphicsMemory& memory, const glm::vec2& pixelsPerUnit);

    void handleAllocateBatchCommand(Allocator* allocator, const MutableGraphicsMemory& memory, const AllocateBatchCommand& command);
    void handleUpdateTransformCommand(Allocator* allocator, const MutableGraphicsMemory& memory, const UpdateTransformCommand& command);
    void handleUpdateMaterialCommand(Allocator* allocator, const MutableGraphicsMemory& memory, const UpdateMaterialCommand& command);
    void handleSetClientAreaCommand(Allocator* allocator, const MutableGraphicsMemory& memory, const SetClientAreaCommand& command);
    void handleSetTextureLayerCommand(Allocator* allocator, const MutableGraphicsMemory& memory, const SetTextureLayerCommand& command);

    float getFovDegrees(const Camera* camera);
    glm::vec2 getViewportWorldDimensions(const Camera* camera, const Eye* eye, const Frustum* frustum);
    glm::vec2 getPixelsPerUnit(int clientWidth, int clientHeight, const glm::vec2& worldDimensions);
}
