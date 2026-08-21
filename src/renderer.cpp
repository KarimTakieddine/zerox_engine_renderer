#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"
#include "eye.h"
#include "frustum.h"
#include "locations_descriptor.h"
#include "mesh.hpp"
#include "renderer.hpp"
#include "shader.h"
#include "uniform_buffer_segment.h"

namespace renderer
{
    void allocateBuffers(Allocator* allocator, size_t count)
    {
        allocator->requestMemory<uint64_t>(count);
        allocator->requestMemoryArray<unsigned int>(count);
    }

    void allocateVertexArrays(Allocator* allocator, size_t count)
    {
        allocator->requestMemory<uint64_t>(count);
        allocator->requestMemoryArray<unsigned int>(count);
    }

    void allocateTextures(Allocator* allocator, size_t count)
    {
        allocator->requestMemory<uint64_t>(count);
        allocator->requestMemoryArray<unsigned int>(count);
    }

    void allocateMeshes(Allocator* allocator, size_t count, const Mesh* meshes)
    {
        auto* meshCount = allocator->requestMemory<uint64_t>(count);
        
        for (size_t i = 0; i < *meshCount; ++i)
        {
            const Mesh& mesh = meshes[i];

            auto* bufferIndices = allocator->requestMemory<MeshBufferIndices>();

            auto* vertexCount   = allocator->requestMemory<uint64_t>(mesh.vertexCount);
            auto* vertices      = allocator->requestMemoryArray<Vertex>(*vertexCount);
            for (uint64_t j = 0; j < *vertexCount; ++j)
            {
                vertices[j] = mesh.vertices[j];
            }

            auto* triangleCount = allocator->requestMemory<uint64_t>(mesh.triangleCount);
            auto* triangles     = allocator->requestMemoryArray<unsigned int>(*triangleCount);
            for (uint64_t j = 0; j < *triangleCount; ++j)
            {
                triangles[j] = mesh.triangles[j]; 
            }
        }
    }

    void allocateShaders(Allocator* allocator, size_t count)
    {
        allocator->requestMemory<uint64_t>(count);
        allocator->requestMemoryArray<unsigned int>(count);
    }

    void allocateShaderPrograms(Allocator* allocator, size_t count)
    {
        allocator->requestMemory<uint64_t>(count);
        allocator->requestMemoryArray<unsigned int>(count);
    }

    void allocateLocationsDescriptors(Allocator* allocator, size_t count)
    {
        allocator->requestMemory<uint64_t>(count);
        allocator->requestMemoryArray<LocationsDescriptor>(count);
    }

    void allocateCamera(Allocator* allocator)
    {
        allocator->requestMemory<Eye>();
        allocator->requestMemory<Frustum>();
        allocator->requestMemory<Camera>();
    }

    void setCameraEye(const MutableGraphicsMemory& memory, const Eye* eye)
    {
        *( memory.cameraEye.data() ) = *eye;
    }

    void setCameraFrustum(const MutableGraphicsMemory& memory, const Frustum* frustum)
    {
        *( memory.cameraFrustum.data() ) = *frustum;
    }

    void updateCamera(const MutableGraphicsMemory& memory)
    {
        const auto* eye     = memory.cameraEye.data();
        const auto* frustum = memory.cameraFrustum.data();
        auto* camera        = memory.camera.data();

        camera->projection  = glm::perspective(glm::radians(frustum->fov), frustum->aspect, frustum->near, frustum->far);
        camera->view        = glm::lookAtRH(eye->position, eye->target, eye->up);
    }

    void allocateUniformBuffer(Allocator* allocator, size_t segmentCount)
    {
        allocator->requestMemory<unsigned int>();
        allocator->requestMemory<uint64_t>(segmentCount);
        allocator->requestMemoryArray<UniformBufferSegment>(segmentCount);
    }

    bool mapCameraUniforms(const MutableGraphicsMemory& memory)
    {
        const auto segments = memory.uniformBufferSegments;

        if (segments.size() != 4)
        {
            return false;
        }

        auto* segmentData = segments.data();

        const auto* camera = memory.camera.data();

        segmentData[0].data = &camera->projection;
        segmentData[1].data = &camera->localToWorld;
        segmentData[2].data = &camera->localRotation;
        segmentData[3].data = &camera->view;

        return true;
    }

    void allocateRenderBatches(Allocator* allocator, size_t count, const size_t* entityCounts)
    {
        allocator->requestMemory<uint64_t>(count);

        for (size_t i = 0; i < count; ++i)
        {
            allocator->requestMemory<RenderBatch>();

            const size_t entityCount = entityCounts[i];
            allocator->requestMemory<uint64_t>(entityCount);
            allocator->requestMemoryArray<RenderEntity>(entityCount);
        }
    }

    void allocateGraphicsResources(Allocator* allocator, const GraphicsConfig* config)
    {
        allocator->allocate(ALLOCATOR_SIZE);

        allocateBuffers(allocator, config->bufferCount);
        allocateVertexArrays(allocator, config->vertexArrayCount);
        allocateTextures(allocator, config->textureCount);
        allocateShaders(allocator, config->shaderCount);
        allocateShaderPrograms(allocator, config->shaderProgramCount);
        allocateMeshes(allocator, config->meshCount, config->meshes);
        allocateLocationsDescriptors(allocator, config->locationsDescriptorCount);
        allocateCamera(allocator);
        allocateUniformBuffer(allocator, 4);
        allocateRenderBatches(allocator, config->renderBatchCount, config->renderEntityCounts);
    }

    void initializeGraphicsResources(const MutableGraphicsMemory& memory, const GraphicsConfig* config, const PlatformFunctions* platformFunctions)
    {
        generateBuffers(memory);
        generateVertexArrays(memory);
        generateTextures(memory);
        generateShaders(memory, config->shaderCount, config->shaders, platformFunctions);
        generateShaderPrograms(memory);
        generateMeshes(memory);

        for (size_t i = 0; i < config->compileStepCount; ++i)
        {
            const ShaderCompileStep* compileStep = config->shaderCompileSteps + i;

            compileShaderProgram(
                memory,
                compileStep->programIndex,
                compileStep->shaderCount,
                compileStep->shaderIndices);
        }

        uploadMeshes(freezeGraphicsMemory(memory));

        setShaderLocations(memory, 0, 0);

        setCameraEye(memory, config->cameraEye);
        setCameraFrustum(memory, config->cameraFrustum);
        updateCamera(memory);

        generateUniformBuffer(memory, 0, config->cameraUniformBuffer, config->cameraUniformNames);
        mapCameraUniforms(memory);

        generateRenderBatch(memory, 0, 0, 0, 0);
        setVertexLayout(memory, 0, 0);
    }

    void freeGraphicsResources(const MutableGraphicsMemory& memory)
    {
        freeShaders(memory);
        freeTextures(memory);
        freeVertexArrays(memory);
        freeBuffers(memory);
    }

    void renderBatches(const ConstGraphicsMemory& memory)
    {
        ConstMemoryView renderBatchView(memory.renderBatchSpan);

        MemoryCursor<MEMORY_ALIGNMENT> renderBatchCursor;

        const auto batchCount = renderBatchView.read_object<uint64_t>(renderBatchCursor.getOffset());
        renderBatchCursor.step<uint64_t>();

        for (uint64_t i = 0; i < *batchCount.data(); ++i)
        {
            RenderBatchSpan<true> batchSpan;

            const auto batch        = renderBatchView.read_object<RenderBatch>(renderBatchCursor.getOffset()); 
            batchSpan.renderBatch   = batch;
            renderBatchCursor.step<RenderBatch>();

            const auto entities = renderBatchView.read_contiguous_array<RenderEntity>(renderBatchCursor.getOffset());
            batchSpan.entities  = entities;
            renderBatchCursor.step_array<RenderEntity>(entities.size());

            renderBatch(batchSpan);

            for (size_t j = 0; j < entities.size(); ++j)
            {
                renderEntity(entities.data() + j, memory.locationsDescriptors.data() + batch.data()->descriptorIndex, batch.data()->elememtCount);
            }
        }
    }

    void render(const ConstGraphicsMemory& memory)
    {
        uploadUniformBuffer(memory);
        renderBatches(memory);
    }
}