#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include "camera.h"
#include "eye.h"
#include "frustum.h"
#include "locations_descriptor.h"
#include "mesh.hpp"
#include "renderer.hpp"
#include "render_frame.h"
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

    void renderBatches(const MutableGraphicsMemory& memory, const glm::vec2 pixelsPerUnit)
    {
        const auto batchCount = *memory.renderBatchCount.data();

        MutableMemoryView renderBatchView(memory.renderBatchSpan);
        MemoryCursor<MEMORY_ALIGNMENT> renderBatchCursor;

        for (uint64_t i = 0; i < batchCount; ++i)
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
                renderEntity(
                    entities.data() + j,
                    memory.locationsDescriptors.data() + batch.data()->descriptorIndex,
                    batch.data()->drawMode, 
                    batch.data()->elementCount,
                    pixelsPerUnit);
            }
        }
    }

    void render(const MutableGraphicsMemory& memory, const glm::vec2& pixelsPerUnit)
    {
        uploadUniformBuffer(freezeGraphicsMemory(memory));
        renderBatches(memory, pixelsPerUnit);
    }

    void processCommand(Allocator* allocator, MutableGraphicsMemory& memory, const Command* command)
    {
        const auto batchCount = *memory.renderBatchCount.data();

        MutableMemoryView renderBatchView(memory.renderBatchSpan);
        MemoryCursor<MEMORY_ALIGNMENT> batchCursor;

        FixedSpan<std::byte, true, Command::DATA_SECTION_SIZE> commandSpan(command->data, Command::DATA_SECTION_SIZE);
        ConstMemoryView commandView(commandSpan);

        switch (command->type)
        {
            case CommandType::ALLOCATE_RENDER_BATCH:
            {
                const auto* shaderProgramIndex  = commandView.read_object<size_t>(0).data();
                const auto* vertexArrayIndex    = commandView.read_object<size_t>(sizeof(size_t)).data();
                const auto* locationsIndex      = commandView.read_object<uint64_t>(2 * sizeof(size_t)).data();
                const auto* meshIndex           = commandView.read_object<size_t>(2 * sizeof(size_t) + sizeof(uint64_t)).data();
                const auto* entityCount         = commandView.read_object<size_t>(3 * sizeof(size_t) + sizeof(uint64_t)).data();
                const auto* drawMode            = commandView.read_object<DrawMode>(4 * sizeof(size_t) + sizeof(uint64_t)).data();

                const auto shaderPrograms   = memory.shaderPrograms;
                const auto vertexArrays     = memory.vertexArrayObjects;

                auto* renderBatch = allocator->requestMemory<RenderBatch>(RenderBatch{
                    .descriptorIndex    = *locationsIndex,
                    .shaderProgram      = *shaderProgramIndex >= shaderPrograms.size() ? 0 : shaderPrograms.data()[*shaderProgramIndex],
                    .vertexArray        = *vertexArrayIndex >= vertexArrays.size() ? 0 : vertexArrays.data()[*vertexArrayIndex],
                    .drawMode           = *drawMode
                });

                allocator->requestMemory<uint64_t>(*entityCount);
                auto* entities = allocator->requestMemoryArray<RenderEntity>(*entityCount);

                size_t* batchCount      = memory.renderBatchCount.data();
                const size_t batchIndex = *batchCount;

                ++(*batchCount);

                memory = readGraphicsMemory(allocator);

                setVertexLayout(memory, batchIndex, *meshIndex);

                break;
            }
            case CommandType::UPDATE_ENTITY_TRANSFORM:
            {
                memory = readGraphicsMemory(allocator);

                const auto* batchIndex  = commandView.read_object<size_t>(0).data();
                const auto* entityIndex = commandView.read_object<size_t>(sizeof(size_t)).data();
                const auto* transform   = commandView.read_object<Transform>(2 * sizeof(size_t)).data();

                if (*batchIndex >= *memory.renderBatchCount.data())
                {
                    break;
                }

                for (size_t i = 0; i < *batchIndex; ++i)
                {
                    batchCursor.step<RenderBatch>();
                    const auto entities = renderBatchView.read_contiguous_array<RenderEntity>(batchCursor.getOffset());
                    batchCursor.step_array<RenderEntity>(entities.size());
                }

                batchCursor.step<RenderBatch>();
                const auto entities = renderBatchView.read_contiguous_array<RenderEntity>(batchCursor.getOffset());

                RenderEntity* entity    = entities.data() + *entityIndex;
                entity->transform       = *transform;

                break;
            }
            case CommandType::UPDATE_ENTITY_MATERIAL:
            {
                const auto* batchIndex  = commandView.read_object<size_t>(0).data();
                const auto* entityIndex = commandView.read_object<size_t>(sizeof(size_t)).data();
                const auto* material    = commandView.read_object<Material>(2 * sizeof(size_t)).data();

                if (*batchIndex >= *memory.renderBatchCount.data())
                {
                    break;
                }

                for (size_t i = 0; i < *batchIndex; ++i)
                {
                    batchCursor.step<RenderBatch>();
                    const auto entities = renderBatchView.read_contiguous_array<RenderEntity>(batchCursor.getOffset());
                    batchCursor.step_array<RenderEntity>(entities.size());
                }

                batchCursor.step<RenderBatch>();
                const auto entities = renderBatchView.read_contiguous_array<RenderEntity>(batchCursor.getOffset());

                RenderEntity* entity    = entities.data() + *entityIndex;
                entity->material        = *material;

                break;
            }
            case CommandType::SET_CAMERA_ASPECT_RATIO:
            {
                const auto* width   = commandView.read_object<int>(0).data();
                const auto* height  = commandView.read_object<int>(sizeof(int)).data();
                
                setViewportArea(*width, *height);
                setCameraAspectRatio(memory, *width, *height);

                break;
            }
            case CommandType::SET_TEXTURE_LAYER:
            {
                const auto* batchIndex  = commandView.read_object<size_t>(0).data();
                const auto* entityIndex = commandView.read_object<size_t>(sizeof(size_t)).data();
                const auto* layer       = commandView.read_object<uint64_t>(2 * sizeof(size_t)).data();

                if (*batchIndex >= *memory.renderBatchCount.data())
                {
                    break;
                }

                for (size_t i = 0; i < *batchIndex; ++i)
                {
                    batchCursor.step<RenderBatch>();
                    const auto entities = renderBatchView.read_contiguous_array<RenderEntity>(batchCursor.getOffset());
                    batchCursor.step_array<RenderEntity>(entities.size());
                }

                batchCursor.step<RenderBatch>();
                const auto entities = renderBatchView.read_contiguous_array<RenderEntity>(batchCursor.getOffset());

                RenderEntity* entity    = entities.data() + *entityIndex;
                entity->layer           = *layer;

                break;
            }
            default:
                break;
        }
    }

    void processFrame(Allocator* allocator, MutableGraphicsMemory& memory, const Frame* frame)
    {
        for (const auto& command : frame->renderCommands)
        {
            processCommand(allocator, memory, &command);
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