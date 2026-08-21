#pragma once

#include <cstddef>
#include <cstdint>

#include <bump_allocator.hpp>
#include <memory_cursor.hpp>
#include <memory_view.hpp>

#include "graphics_memory.hpp"
#include "mesh_span.hpp"
#include "render_batch_span.hpp"

namespace renderer
{
    struct LocationsDescriptor;
    struct Eye;
    struct Frustum;
    struct RenderEntity;
    struct Shader;

    static constexpr size_t ALLOCATOR_SIZE      = 1 << 13;
    static constexpr uint64_t MEMORY_ALIGNMENT  = 16;

    using Allocator = BumpAllocator<16>;

    struct PlatformFunctions
    {
        typedef size_t (*GetFileSizeFunc)(const char*);
        typedef bool (*ReadFileFunc)(const char*, char*, size_t);

        GetFileSizeFunc getFileSize;
        ReadFileFunc readFile;
    };

    struct GraphicsConfig
    {
        const Mesh* meshes                      { nullptr };
        const char* const* textures             { nullptr };
        const size_t* renderEntityCounts        { nullptr };
        const Shader* shaders                   { nullptr };
        const Eye* cameraEye                    { nullptr };
        const Frustum* cameraFrustum            { nullptr };
        const Camera* camera                    { nullptr };
        const char* const cameraUniformBuffer   { nullptr };
        const char* const* cameraUniformNames   { nullptr };
        size_t meshCount                        { 0 };
        size_t bufferCount                      { 0 };
        size_t vertexArrayCount                 { 0 };
        size_t textureCount                     { 0 };
        size_t shaderCount                      { 0 };
        size_t shaderProgramCount               { 0 };
        size_t locationsDescriptorCount         { 0 };
        size_t renderBatchCount                 { 0 };
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

        DynamicSpan<unsigned int, IsConst> shaders  = memoryView.template read_contiguous_array<unsigned int>(cursor.getOffset());
        graphicsMemory.shaders                      = shaders;
        cursor.step_array<unsigned int>(shaders.size());

        DynamicSpan<unsigned int, IsConst> shaderPrograms   = memoryView.template read_contiguous_array<unsigned int>(cursor.getOffset());
        graphicsMemory.shaderPrograms                       = shaderPrograms;
        cursor.step_array<unsigned int>(shaderPrograms.size());

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

        DynamicSpan<LocationsDescriptor, IsConst> locationsDescriptors  = memoryView.template read_contiguous_array<LocationsDescriptor>(cursor.getOffset());
        graphicsMemory.locationsDescriptors                             = locationsDescriptors;
        cursor.step_array<LocationsDescriptor>(locationsDescriptors.size());

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

        const uint64_t renderBatchOffset    = cursor.getOffset();
        const uint64_t renderBatchCount     = *memoryView.template read_object<uint64_t>(renderBatchOffset).data();
        cursor.step<uint64_t>();

        for (uint64_t i = 0; i < renderBatchCount; ++i)
        {
            cursor.step<RenderBatch>();

            const uint64_t entityCount = *memoryView.template read_object<uint64_t>(cursor.getOffset()).data();
            cursor.step_array<RenderEntity>(entityCount);
        }

        graphicsMemory.renderBatchSpan = allocatorSpan.subspan(renderBatchOffset, cursor.getOffset() - renderBatchOffset);

        return graphicsMemory;
    }

    
    void allocateBuffers(Allocator* allocator, size_t count);
    void allocateVertexArrays(Allocator* allocator, size_t count);
    void allocateTextures(Allocator* allocator, size_t count);
    void allocateShaders(Allocator* allocator, size_t count);
    void allocateShaderPrograms(Allocator* allocator, size_t count);
    void allocateMeshes(Allocator* allocator, size_t count, const Mesh* meshes);
    void allocateLocationsDescriptors(Allocator* allocator, size_t count);
    void allocateCamera(Allocator* allocator);
    void allocateUniformBuffer(Allocator* allocator, size_t segmentCount);
    void allocateRenderBatches(Allocator* allocator, size_t count, const size_t* entityCounts);
    void allocateGraphicsResources(Allocator* allocator, const GraphicsConfig* config);


    void generateBuffers(const MutableGraphicsMemory& memory);
    void generateVertexArrays(const MutableGraphicsMemory& memory);
    void generateTextures(const MutableGraphicsMemory& memory);
    bool generateShaders(const MutableGraphicsMemory& memory, size_t count, const Shader* shaders, const PlatformFunctions* platformFunctions);
    void generateShaderPrograms(const MutableGraphicsMemory& memory);
    bool compileShaderProgram(const MutableGraphicsMemory& memory, size_t index, size_t count, const size_t* indices);
    void generateMeshes(const MutableGraphicsMemory& memory);
    void uploadMeshes(const ConstGraphicsMemory& memory);
    void uploadMesh(const MeshSpan<true>& meshSpan);
    bool setShaderLocations(const MutableGraphicsMemory& memory, size_t programIndex, size_t descriptorIndex);
    void setCameraEye(const MutableGraphicsMemory& memory, const Eye* eye);
    void setCameraFrustum(const MutableGraphicsMemory& memory, const Frustum* frustum);
    void updateCamera(const MutableGraphicsMemory& memory);
    bool generateUniformBuffer(const MutableGraphicsMemory& memory, size_t programIndex, const char* name, const char* const* names);
    bool mapCameraUniforms(const MutableGraphicsMemory& memory);
    void uploadUniformBuffer(const ConstGraphicsMemory& memory);
    bool generateRenderBatch(const MutableGraphicsMemory& memory, size_t batchIndex, size_t vertexArrayIndex, size_t programIndex, size_t descriptorIndex);
    bool setVertexLayout(const MutableGraphicsMemory& memory, size_t batchIndex, size_t meshIndex);
    void initializeGraphicsResources(const MutableGraphicsMemory& memory, const GraphicsConfig* config, const PlatformFunctions* platformFunctions);


    void freeBuffers(const MutableGraphicsMemory& memory);
    void freeVertexArrays(const MutableGraphicsMemory& memory);
    void freeTextures(const MutableGraphicsMemory& memory);
    void freeShaders(const MutableGraphicsMemory& memory);
    void freeGraphicsResources(const MutableGraphicsMemory& memory);


    void initializeGraphicsState();
    void clearFrameBuffer();
    void renderEntity(const RenderEntity* entity, const LocationsDescriptor* descriptor, int elementCount);
    void renderBatch(const RenderBatchSpan<true>& span);
    void renderBatches(const ConstGraphicsMemory& memory);
    void render(const ConstGraphicsMemory& memory);
}
