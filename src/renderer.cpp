#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include "camera.h"
#include "eye.h"
#include "frustum.h"
#include "locations_descriptor.h"
#include "mesh.hpp"
#include "render_command_types.h"
#include "render_entity.h"
#include "render_frame.h"
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
        allocateShaders(allocator, config->shaderCount);
        allocateShaderPrograms(allocator, config->shaderProgramCount);
        allocateMeshes(allocator, config->meshCount, config->meshes);
        allocateLocationsDescriptors(allocator, config->locationsDescriptorCount);
        allocateCamera(allocator);
        allocateUniformBuffer(allocator, 4);
        allocateRenderBatchCount(allocator);
    }

    void initializeGraphicsResources(const MutableGraphicsMemory& memory, const GraphicsConfig* config)
    {
        generateBuffers(memory);
        generateVertexArrays(memory);
        generateTextures(memory);
        generateShaders(memory, config->shaderCount, config->shaders);
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

        auto graphicsMemory = freezeGraphicsMemory(memory);

        uploadTextures(graphicsMemory, config->textureCount, config->textures);
        
        for (size_t i = 0; i < config->textureArrayCount; ++i)
        {
            const TextureArray* textureArray = config->textureArrays + i;

            uploadTextureArray(
                graphicsMemory,
                textureArray->data,
                config->textureCount + i,
                textureArray->imageCount,
                textureArray->imageFiles);
        }

        uploadMeshes(graphicsMemory);

        for (size_t i = 0; i < config->locationsLinkCount; ++i)
        {
            const ShaderLocationsLink* link = config->shaderLocationsLinks + i;

            setShaderLocations(memory, link->shaderProgramIndex, link->locationsIndex);
        }

        setCameraEye(memory, config->cameraEye);
        setCameraFrustum(memory, config->cameraFrustum);
        updateCamera(memory);

        generateUniformBuffer(memory, 0, config->cameraUniformBuffer, config->cameraUniformNames);
        mapCameraUniforms(memory);
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
            const auto batch = renderBatchView.read_object<RenderBatch>(renderBatchCursor.getOffset());
            renderBatchCursor.step<RenderBatch>();

            const auto entities = renderBatchView.read_contiguous_array<RenderEntity>(renderBatchCursor.getOffset());
            renderBatchCursor.step_array<RenderEntity>(entities.size());

            renderBatch(batch.data());

            for (size_t j = 0; j < entities.size(); ++j)
            {
                renderEntity(
                    entities.data() + j,
                    memory.locationsDescriptors.data() + batch.data()->descriptorIndex,
                    batch.data()->drawMode, 
                    batch.data()->elementCount,
                    pixelsPerUnit);
            }
        }
    }

    void render(const ConstGraphicsMemory& memory, const glm::vec2& pixelsPerUnit)
    {
        uploadUniformBuffer(memory);
        renderBatches(memory, pixelsPerUnit);
    }

    void handleAllocateBatchCommand(Allocator* allocator, const MutableGraphicsMemory& memory, const ConstMemoryView& commandView)
    {
        const auto* command = commandView.read_object<AllocateBatchCommand>(0).data();

        auto* renderBatch = allocator->requestMemory<RenderBatch>(RenderBatch{
            .descriptorIndex    = command->locationsIndex,
            .shaderProgram      = memory.shaderPrograms.data()[command->shaderProgramIndex],
            .vertexArray        = memory.vertexArrayObjects.data()[command->vertexArrayIndex],
            .drawMode           = static_cast<DrawMode>(command->drawMode)
        });

        allocator->requestMemory<uint64_t>(command->entityCount);
        allocator->requestMemoryArray<RenderEntity>(command->entityCount);

        ConstMemoryView meshView(memory.meshSpan);
        MemoryCursor<16> meshCursor;

        const auto* meshCount = meshView.read_object<uint64_t>(meshCursor.getOffset()).data();
        meshCursor.step<uint64_t>();

        for (size_t i = 0; i < command->meshIndex; ++i)
        {
            meshCursor.step<MeshBufferIndices>();

            const auto vertices = meshView.read_contiguous_array<Vertex>(meshCursor.getOffset());
            meshCursor.step_array<Vertex>(vertices.size());

            const auto triangles = meshView.read_contiguous_array<unsigned int>(meshCursor.getOffset());
            meshCursor.step_array<unsigned int>(triangles.size());
        }

        const auto* buffers = meshView.read_object<MeshBufferIndices>(meshCursor.getOffset()).data();
        meshCursor.step<MeshBufferIndices>();

        const auto vertices = meshView.read_contiguous_array<Vertex>(meshCursor.getOffset());
        meshCursor.step_array<Vertex>(vertices.size());

        const auto triangles = meshView.read_contiguous_array<unsigned int>(meshCursor.getOffset());

        renderBatch->elementCount = triangles.size();

        setVertexLayout(
            renderBatch,
            memory.locationsDescriptors.data() + command->locationsIndex,
            buffers);

        ++(*memory.renderBatchCount.data());
    }

    void handleUpdateTransformCommand(Allocator* allocator, const MutableGraphicsMemory& memory, const ConstMemoryView& commandView)
    {
        MutableMemoryView renderBatchView(memory.renderBatchSpan);
        MemoryCursor<16> batchCursor;

        const auto* command = commandView.read_object<UpdateTransformCommand>(0).data();

        for (size_t i = 0; i < command->batchIndex; ++i)
        {
            batchCursor.step<RenderBatch>();
            const auto entities = renderBatchView.read_contiguous_array<RenderEntity>(batchCursor.getOffset());
            batchCursor.step_array<RenderEntity>(entities.size());
        }

        batchCursor.step<RenderBatch>();
        const auto entities = renderBatchView.read_contiguous_array<RenderEntity>(batchCursor.getOffset());

        (entities.data() + command->entityIndex)->transform = command->transform;
    }

    void handleUpdateMaterialCommand(Allocator* allocator, const MutableGraphicsMemory& memory, const ConstMemoryView& commandView)
    {
        MutableMemoryView renderBatchView(memory.renderBatchSpan);
        MemoryCursor<16> batchCursor;

        const auto* command = commandView.read_object<UpdateMaterialCommand>(0).data();

        for (size_t i = 0; i < command->batchIndex; ++i)
        {
            batchCursor.step<RenderBatch>();
            const auto entities = renderBatchView.read_contiguous_array<RenderEntity>(batchCursor.getOffset());
            batchCursor.step_array<RenderEntity>(entities.size());
        }

        batchCursor.step<RenderBatch>();
        const auto entities = renderBatchView.read_contiguous_array<RenderEntity>(batchCursor.getOffset());

        (entities.data() + command->entityIndex)->material = command->material;
    }

    void handleSetClientAreaCommand(Allocator* allocator, const MutableGraphicsMemory& memory, const ConstMemoryView& commandView)
    {
        const auto* command = commandView.read_object<SetClientAreaCommand>(0).data();

        const int width     = static_cast<int>(command->width);
        const int height    = static_cast<int>(command->height);

        setViewportArea(width, height);
        setCameraAspectRatio(memory, width, height);
    }

    void handleSetTextureLayerCommand(Allocator* allocator, const MutableGraphicsMemory& memory, const ConstMemoryView& commandView)
    {
        MutableMemoryView renderBatchView(memory.renderBatchSpan);
        MemoryCursor<16> batchCursor;

        const auto* command = commandView.read_object<SetTextureLayerCommand>(0).data();

        for (size_t i = 0; i < command->batchIndex; ++i)
        {
            batchCursor.step<RenderBatch>();
            const auto entities = renderBatchView.read_contiguous_array<RenderEntity>(batchCursor.getOffset());
            batchCursor.step_array<RenderEntity>(entities.size());
        }

        batchCursor.step<RenderBatch>();
        const auto entities = renderBatchView.read_contiguous_array<RenderEntity>(batchCursor.getOffset());

        (entities.data() + command->entityIndex)->layer = command->layer;
    }

    void processCommand(Allocator* allocator, const Command* command)
    {
        FixedSpan<std::byte, true, Command::DATA_SECTION_SIZE> commandSpan(command->data, Command::DATA_SECTION_SIZE);
        commandDispatchTable[static_cast<size_t>(command->type)](allocator, readGraphicsMemory(allocator), ConstMemoryView(commandSpan));
    }

    void processFrame(Allocator* allocator, const Frame* frame)
    {
        for (const auto& command : frame->renderCommands)
        {
            processCommand(allocator, &command);
        }
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