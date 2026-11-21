#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include <zerox_render_entity.h>

#include "camera.h"
#include "eye.h"
#include "frustum.h"
#include "mesh.hpp"
#include "render_command_types.h"
#include "renderer.hpp"
#include "shader.h"
#include "shader_program.h"
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
        allocator->requestMemoryArray<Shader>(count);
    }

    void allocateShaderPrograms(Allocator* allocator, size_t count)
    {
        allocator->requestMemory<uint64_t>(count);
        allocator->requestMemoryArray<ShaderProgram>(count);
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

        camera->projection = glm::perspective(glm::radians(frustum->fov), frustum->aspect, frustum->nearPlane, frustum->farPlane);
        camera->view        = glm::lookAtRH(eye->position, eye->target, eye->up);
    }

    void setCameraAspectRatio(const MutableGraphicsMemory& memory, int width, int height)
    {
        memory.cameraFrustum.data()->aspect = static_cast<float>(width) / height;

        updateCamera(memory);
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

    void allocateRenderBatchCount(Allocator* allocator)
    {
        allocator->requestMemory<uint64_t>();
    }

    void allocateGraphicsResources(Allocator* allocator, const GraphicsConfig* config)
    {
        allocator->allocate(ALLOCATOR_SIZE);

        allocateBuffers(allocator, config->bufferCount);
        allocateVertexArrays(allocator, config->vertexArrayCount);
        allocateTextures(allocator, config->textureCount + config->textureArrayCount);
        allocateMeshes(allocator, config->meshCount, config->meshes);
        allocateShaders(allocator, config->shaderCount);
        allocateShaderPrograms(allocator, config->shaderProgramCount);
        allocateCamera(allocator);
        allocateUniformBuffer(allocator, 4);
    }

    void initializeGraphicsResources(Allocator* allocator, const GraphicsConfig* config)
    {
        MutableGraphicsMemory globalMemory = readGraphicsMemory(allocator);

        generateBuffers(globalMemory);
        generateVertexArrays(globalMemory);
        generateTextures(globalMemory);
        generateMeshes(globalMemory);
        loadShaders(globalMemory, config->shaders, config->shaderCount);
        loadShaderPrograms(globalMemory, config->shaderCompileSteps, config->compileStepCount);
        setCameraEye(globalMemory, config->cameraEye);
        setCameraFrustum(globalMemory, config->cameraFrustum);
        updateCamera(globalMemory);
        generateUniformBuffer(globalMemory, 0, config->cameraUniformBuffer, config->cameraUniformNames);
        mapCameraUniforms(globalMemory);
        loadShaderAttributes(allocator, globalMemory);
        loadShaderUniforms(allocator, globalMemory);

        auto graphicsMemory = freezeGraphicsMemory(readGraphicsMemory(allocator));

        loadEntityBindings(allocator, graphicsMemory);
        allocateRenderBatchCount(allocator);

        uploadMeshes(graphicsMemory);
        uploadTextures(graphicsMemory, config->textureCount, config->textures);

        for (size_t i = 0; i < config->textureArrayCount; ++i)
        {
            const TextureArray* textureArray = config->textureArrays + i;

            uploadTextureArray(
                graphicsMemory,
                textureArray->data,
                config->textureCount + i,
                textureArray->textureCount,
                textureArray->paths);
        }
    }

    void freeGraphicsResources(const MutableGraphicsMemory& memory)
    {
        freeShaders(memory);
        freeTextures(memory);
        freeVertexArrays(memory);
        freeBuffers(memory);
    }

    void renderBatches(const ConstGraphicsMemory& memory, const glm::vec2& pixelsPerUnit)
    {
        const auto batchCount = *memory.renderBatchCount.data();

        ConstMemoryView renderBatchView(memory.renderBatchSpan);
        MemoryCursor<MEMORY_ALIGNMENT> renderBatchCursor;

        for (uint64_t i = 0; i < batchCount; ++i)
        {
            const auto* batch = renderBatchView.read_object<RenderBatch>(renderBatchCursor.getOffset()).data();
            renderBatchCursor.step<RenderBatch>();

            const auto entities = renderBatchView.read_contiguous_array<ZeroXBridge::RenderEntity>(renderBatchCursor.getOffset());
            renderBatchCursor.step_array<ZeroXBridge::RenderEntity>(entities.size());



            /*const ShaderProgram* shaderProgram      = memory.shaderPrograms.data() + shaderProgramIndex;
            const EntityBindings* entityBindings    = memory.renderEntityBindings.data() + shaderProgramIndex;

            renderBatch(shaderProgram, *(memory.vertexArrayObjects.data() + static_cast<size_t>(batch->vertexArrayIndex)));

            for (size_t j = 0; j < entities.size(); ++j)
            {
                renderEntity(entities.data() + j, entityBindings, batch->drawMode, batch->elementCount, pixelsPerUnit);
            }*/
        }
    }

    void render(const ConstGraphicsMemory& memory, const glm::vec2& pixelsPerUnit)
    {
        uploadUniformBuffer(memory);

        renderBatches(memory, pixelsPerUnit);
    }

    void handleAllocateBatchCommand(Allocator* allocator, const MutableGraphicsMemory& memory, const AllocateBatchCommand& command)
    {
        auto* batch = allocator->requestMemory<RenderBatch>(RenderBatch{
            .entityCount        = command.entityCount,
            .drawMode           = command.drawMode,
            .meshIndex          = command.meshIndex,
            .shaderProgramIndex = command.shaderProgramIndex,
            .vertexArrayIndex   = command.vertexArrayIndex,
            .elementCount       = command.elementCount
        });

        const size_t entityCount = batch->entityCount;

        allocator->requestMemory<uint64_t>(entityCount);
        allocator->requestMemoryArray<ZeroXBridge::RenderEntity>(entityCount);

        ConstMemoryView meshView(memory.meshSpan);
        MemoryCursor<16> meshCursor;

        const auto* meshCount = meshView.read_object<uint64_t>(meshCursor.getOffset()).data();
        meshCursor.step<uint64_t>();

        for (size_t i = 0; i < batch->meshIndex; ++i)
        {
            meshCursor.step<MeshBufferIndices>();

            const auto vertices = meshView.read_contiguous_array<Vertex>(meshCursor.getOffset());
            meshCursor.step_array<Vertex>(vertices.size());

            const auto triangles = meshView.read_contiguous_array<unsigned int>(meshCursor.getOffset());
            meshCursor.step_array<unsigned int>(triangles.size());
        }

        const auto* buffers = meshView.read_object<MeshBufferIndices>(meshCursor.getOffset()).data();
        meshCursor.step<MeshBufferIndices>();

        const ShaderProgram* shaderProgram  = memory.shaderPrograms.data() + static_cast<size_t>(batch->shaderProgramIndex);
        const unsigned int vertexArray      = *(memory.vertexArrayObjects.data() + static_cast<size_t>(batch->vertexArrayIndex));

        renderBatch(shaderProgram, vertexArray);
        setVertexLayout(
            shaderProgram,
            memory.renderEntityBindings.data() + static_cast<size_t>(batch->shaderProgramIndex),
            vertexArray,
            buffers);

        ++(*memory.renderBatchCount.data());
    }

    void handleUpdateTransformCommand(Allocator* allocator, const MutableGraphicsMemory& memory, const UpdateTransformCommand& command)
    {
        MutableMemoryView renderBatchView(memory.renderBatchSpan);
        MemoryCursor<16> batchCursor;

        for (size_t i = 0; i < command.batchIndex; ++i)
        {
            batchCursor.step<RenderBatch>();
            const auto entities = renderBatchView.read_contiguous_array<ZeroXBridge::RenderEntity>(batchCursor.getOffset());
            batchCursor.step_array<ZeroXBridge::RenderEntity>(entities.size());
        }

        batchCursor.step<RenderBatch>();
        const auto entities = renderBatchView.read_contiguous_array<ZeroXBridge::RenderEntity>(batchCursor.getOffset());

        (entities.data() + command.entityIndex)->transform = command.transform;
    }

    void handleUpdateMaterialCommand(Allocator* allocator, const MutableGraphicsMemory& memory, const UpdateMaterialCommand& command)
    {
        MutableMemoryView renderBatchView(memory.renderBatchSpan);
        MemoryCursor<16> batchCursor;

        for (size_t i = 0; i < command.batchIndex; ++i)
        {
            batchCursor.step<RenderBatch>();
            const auto entities = renderBatchView.read_contiguous_array<ZeroXBridge::RenderEntity>(batchCursor.getOffset());
            batchCursor.step_array<ZeroXBridge::RenderEntity>(entities.size());
        }

        batchCursor.step<RenderBatch>();
        const auto entities = renderBatchView.read_contiguous_array<ZeroXBridge::RenderEntity>(batchCursor.getOffset());

        (entities.data() + command.entityIndex)->material = command.material;
    }

    void handleSetClientAreaCommand(Allocator* allocator, const MutableGraphicsMemory& memory, const SetClientAreaCommand& command)
    {
        const int width     = static_cast<int>(command.width);
        const int height    = static_cast<int>(command.height);

        setViewportArea(width, height);
        setCameraAspectRatio(memory, width, height);
    }

    void handleSetTextureLayerCommand(Allocator* allocator, const MutableGraphicsMemory& memory, const SetTextureLayerCommand& command)
    {
        MutableMemoryView renderBatchView(memory.renderBatchSpan);
        MemoryCursor<16> batchCursor;

        for (size_t i = 0; i < command.batchIndex; ++i)
        {
            batchCursor.step<RenderBatch>();
            const auto entities = renderBatchView.read_contiguous_array<ZeroXBridge::RenderEntity>(batchCursor.getOffset());
            batchCursor.step_array<ZeroXBridge::RenderEntity>(entities.size());
        }

        batchCursor.step<RenderBatch>();
        const auto entities = renderBatchView.read_contiguous_array<ZeroXBridge::RenderEntity>(batchCursor.getOffset());

        (entities.data() + command.entityIndex)->layer = command.layer;
    }

    float getFovDegrees(const Camera* camera)
    {
        const glm::mat4 projection = camera->projection;

        return glm::degrees(2 * atan(1.0f / projection[1][1]));
    }

    glm::vec2 getViewportWorldDimensions(const Camera* camera, const Eye* eye, const Frustum* frustum)
    {
        const float worldHeight = 2 * frustum->nearPlane * eye->position.z * glm::tan(glm::radians(getFovDegrees(camera) * 0.5f));

        return { worldHeight * frustum->aspect, worldHeight };
    }

    glm::vec2 getPixelsPerUnit(int clientWidth, int clientHeight, const glm::vec2& worldDimensions)
    {
        return {
            clientWidth / worldDimensions.x,
            clientHeight / worldDimensions.y
        };
    }
}