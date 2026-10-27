#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include <memory_cursor.hpp>
#include <memory_view.hpp>

#include <platform.h>

#include "locations_descriptor.h"
#include "opengl_allocator.h"
#include "render_buffer.h"
#include "render_entity.h"
#include "renderer.hpp"
#include "shader.h"
#include "texture.h"
#include "uniform_buffer_segment.h"

namespace
{
    constexpr size_t MAX_SHADER_FILE_SIZE = 1 << 10;

    using renderer::DrawMode;
    using renderer::Filter;
    using renderer::WrapMode;

    constexpr GLenum drawModeToGLenum(DrawMode drawMode)
    {
        switch (drawMode)
        {
        case DrawMode::TRIANGLES:   return GL_TRIANGLES;
        case DrawMode::LINES:       return GL_LINES;
        default:                    return GL_NONE;
        }
    }
    
    constexpr GLenum wrapModeToGLenum(WrapMode wrapMode)
    {
        switch (wrapMode)
        {
        case WrapMode::REPEAT:          return GL_REPEAT;
        case WrapMode::CLAMP_TO_EDGE:   return GL_CLAMP_TO_EDGE;
        case WrapMode::CLAMP_TO_BORDER: return GL_CLAMP_TO_BORDER;
        default:                        return GL_NONE;
        }
    }

    constexpr GLenum filterToGLenum(Filter filter)
    {
        switch (filter)
        {
        case Filter::NEAREST:           return GL_NEAREST;
        case Filter::LINEAR:            return GL_LINEAR;
        default:                        return GL_NONE;
        }
    }

    constexpr GLenum imageFormatToGLenum(platform::Image::Format format)
    {
        switch (format)
        {
        case platform::Image::Format::RGB:  return GL_RGB;
        case platform::Image::Format::RGBA: return GL_RGBA;
        default:                            return GL_NONE;
        }
    }
}

namespace renderer
{
    void generateBuffers(const MutableGraphicsMemory& memory)
    {
        const auto bufferObjects = memory.bufferObjects;
        GLuint* bufferObjectData = bufferObjects.data();

        OpenGLAllocator bufferAllocator(glGenBuffers, glDeleteBuffers, bufferObjectData, bufferObjectData);

        bufferAllocator.allocate(static_cast<GLsizei>(bufferObjects.size()));
    }

    void generateVertexArrays(const MutableGraphicsMemory& memory)
    {
        const auto vertexArrayObjects = memory.vertexArrayObjects;
        GLuint* vertexArrayObjectData = vertexArrayObjects.data();

        OpenGLAllocator vertexArrayAllocator(glGenVertexArrays, glDeleteVertexArrays, vertexArrayObjectData, vertexArrayObjectData);

        vertexArrayAllocator.allocate(static_cast<GLsizei>(vertexArrayObjects.size()));
    }

    void generateTextures(const MutableGraphicsMemory& memory)
    {
        const auto textureObjects = memory.textures;
        GLuint* textureObjectData = textureObjects.data();

        OpenGLAllocator textureAllocator(glGenTextures, glDeleteTextures, textureObjectData, textureObjectData);

        textureAllocator.allocate(static_cast<GLsizei>(textureObjects.size()));
    }

    bool uploadTextures(const ConstGraphicsMemory& memory, size_t count, const Texture* textures)
    {
        const auto textureObjects = memory.textures;

        const size_t objectCount = textureObjects.size();

        if (count > objectCount)
        {
            count = objectCount;
        }

        for (size_t i = 0; i < count; ++i)
        {
            const auto* texture = textures + i;

            auto image = platform::loadImage(texture->path, false);

            if (!image->isLoaded())
            {
                return false;
            }

            const auto& textureData = texture->data;

            setTextureParameters(*(textureObjects.data() + i), GL_TEXTURE_2D, &textureData);

            const GLenum format = ::imageFormatToGLenum(image->getFormat());

			glTexImage2D(
                GL_TEXTURE_2D, 0, format, image->getWidth(), image->getHeight(),
                0, format, GL_UNSIGNED_BYTE, image->getData());
        }

        return true;
    }

    bool uploadTextureArray(const ConstGraphicsMemory& memory, const TextureData& data, size_t textureIndex, size_t textureCount, const char* const* imageFiles)
    {
        const auto textures = memory.textures;
        if (textureIndex >= textures.size())
        {
            return false;
        }

        setTextureParameters(*(textures.data() + textureIndex), GL_TEXTURE_2D_ARRAY, &data);

        glTexStorage3D(
            GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8,
            data.width, data.height,
            static_cast<GLsizei>(textureCount));

        for (size_t i = 0; i < textureCount; ++i)
        {
            auto image = platform::loadImage(*(imageFiles + i), false);

            if (!image->isLoaded())
            {
                return false;
            }

            if (image->getWidth() != data.width || image->getHeight() != data.height)
            {
                return false;
            }

            glTexSubImage3D(
                GL_TEXTURE_2D_ARRAY, 0,
                0, 0, i,
                data.width, data.height, 1,
                ::imageFormatToGLenum(image->getFormat()), GL_UNSIGNED_BYTE, image->getData());
        }

        return true;
    }

    void generateMeshes(const MutableGraphicsMemory& memory)
    {
        const auto bufferObjects    = memory.bufferObjects;
        auto* bufferObjectData      = bufferObjects.data();

        OpenGLAllocator bufferAllocator(nullptr, nullptr, bufferObjectData, bufferObjectData + bufferObjects.size());

        MutableMemoryView meshMemoryView(memory.meshSpan);

        MemoryCursor<MEMORY_ALIGNMENT> meshCursor;

        const auto meshCount = meshMemoryView.read_object<uint64_t>(meshCursor.getOffset());
        meshCursor.step<uint64_t>();

        for (uint64_t i = 0; i < *meshCount.data(); ++i)
        {
            auto* bufferIndices = meshMemoryView.read_object<MeshBufferIndices>(meshCursor.getOffset()).data();
            meshCursor.step<MeshBufferIndices>();

            bufferIndices->vertex   = bufferAllocator.get();
            bufferIndices->triangle = bufferAllocator.get();

            const auto vertices = meshMemoryView.read_contiguous_array<Vertex>(meshCursor.getOffset());
            meshCursor.step_array<Vertex>(vertices.size());

            const auto triangles = meshMemoryView.read_contiguous_array<unsigned int>(meshCursor.getOffset());
            meshCursor.step_array<unsigned int>(triangles.size());
        }
    }

    void uploadMeshes(const ConstGraphicsMemory& memory)
    {
        ConstMemoryView meshMemoryView(memory.meshSpan);

        MemoryCursor<MEMORY_ALIGNMENT> meshCursor;

        const auto meshCount = meshMemoryView.read_object<uint64_t>(meshCursor.getOffset());
        meshCursor.step<uint64_t>();

        for (uint64_t i = 0; i < *meshCount.data(); ++i)
        {
            MeshSpan<true> meshSpan;

            meshSpan.bufferIndices = meshMemoryView.read_object<MeshBufferIndices>(meshCursor.getOffset());
            meshCursor.step<MeshBufferIndices>();

            const auto vertices = meshMemoryView.read_contiguous_array<Vertex>(meshCursor.getOffset());
            meshSpan.vertices   = vertices;
            meshCursor.step_array<Vertex>(vertices.size());

            const auto triangles    = meshMemoryView.read_contiguous_array<unsigned int>(meshCursor.getOffset());
            meshSpan.triangles      = triangles;
            meshCursor.step_array<unsigned int>(triangles.size());

            uploadMesh(meshSpan);
        }
    }

    void uploadMesh(const MeshSpan<true>& meshSpan)
    {
        const auto* bufferIndices   = meshSpan.bufferIndices.data();
        const auto vertices         = meshSpan.vertices;
        const auto triangles        = meshSpan.triangles;

        glBindBuffer(GL_ARRAY_BUFFER, bufferIndices->vertex);
        glBufferData(GL_ARRAY_BUFFER, vertices.size_bytes(), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferIndices->triangle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles.size_bytes(), triangles.data(), GL_STATIC_DRAW);
    }

    bool generateShaders(const MutableGraphicsMemory& memory, size_t count, const Shader* shaders)
    {
        const auto shadersView = memory.shaders;

        auto shaderCount = shadersView.size();

        if (count > shaderCount)
        {
            count = shaderCount;
        }

        auto* shadersData = shadersView.data();

        for (size_t i = 0; i < count; ++i)
        {
            const Shader& shader = shaders[i];

            GLenum shaderType = GL_NONE;

            switch (shader.type)
            {
            case Shader::Type::VERTEX:
                shaderType = GL_VERTEX_SHADER;
                break;
            case Shader::Type::FRAGMENT:
                shaderType = GL_FRAGMENT_SHADER;
                break;
            default:
                break;
            }

            if (shaderType == GL_NONE)
            {
                return false;
            }

            const GLuint shaderObject = glCreateShader(shaderType);

            *(shadersData + i) = shaderObject;

            const size_t fileSize = platform::getFileSize(shader.path);
            if (fileSize == 0)
                return false;

            char* fileBuffer = new char[fileSize + 1]();
            std::memset(fileBuffer, 0, fileSize + 1);
            if (!platform::readFile(shader.path, fileBuffer, fileSize))
            {
                delete[] fileBuffer;
                return false;
            }

            glShaderSource(shaderObject, 1, &fileBuffer, nullptr);
            glCompileShader(shaderObject);

            GLint compileStatus{ GL_FALSE };
            glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &compileStatus);
            if (compileStatus == GL_FALSE)
            {
                delete[] fileBuffer;
                return false;
            }

            delete[] fileBuffer;
        }

        return true;
    }

    void generateShaderPrograms(const MutableGraphicsMemory& memory)
    {
        const auto shaderPrograms = memory.shaderPrograms;

        for (size_t i = 0; i < shaderPrograms.size(); ++i)
        {
            *(shaderPrograms.data() + i) = glCreateProgram();
        }
    }

    bool compileShaderProgram(const MutableGraphicsMemory& memory, size_t index, size_t count, const size_t* indices)
    {
        const auto shaderPrograms = memory.shaderPrograms;
        if (index >= shaderPrograms.size())
        {
            return false;
        }

        const auto shaders          = memory.shaders;
        const size_t shaderCount    = shaders.size();

        const GLuint program = shaderPrograms.data()[index];

        for (size_t i = 0; i < count; ++i)
        {
            const size_t shaderIndex = indices[i];

            if (shaderIndex >= shaderCount)
            {
                return false;
            }

            glAttachShader(program, *(shaders.data() + shaderIndex));
        }

        glLinkProgram(program);

        GLint linkStatus{ GL_FALSE };
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

        return linkStatus == GL_TRUE;
    }

    void initializeGraphicsState()
    {
        glDepthFunc(GL_LEQUAL);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);

        // Hot pink!
        glClearColor(1.0f, 0.4118f, 0.7059f, 1.0f);
    }

    void clearFrameBuffer()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    bool setShaderLocations(const MutableGraphicsMemory& memory, size_t programIndex, size_t descriptorIndex)
    {
        const auto shaderPrograms = memory.shaderPrograms;
        if (programIndex >= shaderPrograms.size())
        {
            return false;
        }

        const GLuint shaderProgram = shaderPrograms.data()[programIndex];

        const auto locationsDescriptors = memory.locationsDescriptors;
        if (descriptorIndex >= locationsDescriptors.size())
        {
            return false;
        }

        auto* locationsDescriptor = locationsDescriptors.data() + descriptorIndex;

        locationsDescriptor->positionLocation       = glGetAttribLocation(shaderProgram, "position");
        locationsDescriptor->colorLocation          = glGetAttribLocation(shaderProgram, "color");
        locationsDescriptor->uvLocation             = glGetAttribLocation(shaderProgram, "uv");
        locationsDescriptor->uvScaleLocation        = glGetUniformLocation(shaderProgram, "uvScale");
        locationsDescriptor->transformLocation      = glGetUniformLocation(shaderProgram, "transform");
        locationsDescriptor->materialColorLocation  = glGetUniformLocation(shaderProgram, "materialColor");
        locationsDescriptor->textureOffsetLocation  = glGetUniformLocation(shaderProgram, "textureOffset");
        locationsDescriptor->textureLayerLocation   = glGetUniformLocation(shaderProgram, "layer");

        return true;
    }

    bool generateUniformBuffer(const MutableGraphicsMemory& memory, size_t programIndex, const char* name, const char* const* names)
    {
        const auto* meshCount       = reinterpret_cast<const uint64_t*>(memory.meshSpan.data());
        const auto bufferObjects    = memory.bufferObjects;
        auto* bufferObjectData      = bufferObjects.data();

        OpenGLAllocator bufferAllocator(nullptr, nullptr, bufferObjectData + *meshCount * 2, bufferObjectData + bufferObjects.size());

        const auto uniformBuffer            = memory.uniformBuffer;
        const GLuint uniformBufferObject    = bufferAllocator.get();
        *uniformBuffer.data()               = uniformBufferObject;

        const auto segments = memory.uniformBufferSegments;

        const auto shaderPrograms = memory.shaderPrograms;
        if (programIndex >= shaderPrograms.size())
        {
            return false;
        }

        const GLuint shaderProgram = shaderPrograms.data()[programIndex];

        const GLuint blockIndex = glGetUniformBlockIndex(shaderProgram, name);
        if (blockIndex == GL_INVALID_INDEX)
        {
            return false;
        }

        GLint uniformCount = 0;
        glGetActiveUniformBlockiv(shaderProgram, blockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &uniformCount);
        if (uniformCount < 0)
        {
            return false;
        }

        const size_t segmentCount = static_cast<size_t>(uniformCount);

        if (segmentCount != segments.size())
        {
            return false;
        }

        GLuint* indices	= new GLuint[segmentCount]();
        GLint* offsets	= new GLint[segmentCount]();
        GLint* strides	= new GLint[segmentCount]();
        GLint* types	= new GLint[segmentCount]();

        glGetUniformIndices(shaderProgram, uniformCount, names, indices);
        glGetActiveUniformsiv(shaderProgram, uniformCount, indices, GL_UNIFORM_OFFSET, offsets);
        glGetActiveUniformsiv(shaderProgram, uniformCount, indices, GL_UNIFORM_SIZE, strides);
        glGetActiveUniformsiv(shaderProgram, uniformCount, indices, GL_UNIFORM_TYPE, types);

        GLsizeiptr sizeInBytes = 0;
        for (size_t i = 0; i < segmentCount; ++i)
        {
            UniformBufferSegment* segment = segments.data() + i;

            switch (types[i])
            {
            case GL_FLOAT_MAT4:
                segment->stride = strides[i] * sizeof(glm::mat4);
                break;
            default:
                break;
            }

            segment->offset = offsets[i];

            sizeInBytes += segment->stride;
        }

        delete[] types;
        delete[] strides;
        delete[] offsets;
        delete[] indices;

        glBindBuffer(GL_UNIFORM_BUFFER, uniformBufferObject);
        glBufferData(GL_UNIFORM_BUFFER, sizeInBytes, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBufferRange(GL_UNIFORM_BUFFER, 0, uniformBufferObject, 0, sizeInBytes);

        return true;
    }

    void setViewportArea(int width, int height)
    {
        glViewport(0, 0, width, height);
    }

    void uploadUniformBuffer(const ConstGraphicsMemory& memory)
    {
        const auto uniformBuffer = memory.uniformBuffer;

        glBindBuffer(GL_UNIFORM_BUFFER, *uniformBuffer.data());

        const auto segments = memory.uniformBufferSegments;

        for (size_t i = 0; i < segments.size(); ++i)
        {
            const UniformBufferSegment* segment = segments.data() + i;

            glBufferSubData(GL_UNIFORM_BUFFER, segment->offset, segment->stride, segment->data);
        }

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void setVertexLayout(const RenderBatch* batch, const LocationsDescriptor* descriptor, const MeshBufferIndices* buffers)
    {
        glUseProgram(batch->shaderProgram);
        glBindVertexArray(batch->vertexArray);
        glBindBuffer(GL_ARRAY_BUFFER, buffers->vertex);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers->triangle);

        glEnableVertexAttribArray(descriptor->positionLocation);
        glVertexAttribPointer(descriptor->positionLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);

        glEnableVertexAttribArray(descriptor->colorLocation);
        glVertexAttribPointer(descriptor->colorLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(sizeof(glm::vec3)));

        glEnableVertexAttribArray(descriptor->uvLocation);
        glVertexAttribPointer(descriptor->uvLocation, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(2 * sizeof(glm::vec3)));
    }

    void setTextureParameters(unsigned int texture, unsigned int target, const TextureData* parameters, bool unbind /* = false */)
    {
        glBindTexture(target, texture);
        glTexParameteri(target, GL_TEXTURE_WRAP_S, ::wrapModeToGLenum(parameters->wrapModeS));
        glTexParameteri(target, GL_TEXTURE_WRAP_T, ::wrapModeToGLenum(parameters->wrapModeT));
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, ::filterToGLenum(parameters->minFilter));
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, ::filterToGLenum(parameters->magFilter));

        if (unbind)
        {
            glBindTexture(target, 0);
        }
    }

    void initializeRenderBuffer(RenderBuffer* buffer)
    {
        glGenTextures(1, &buffer->textureHandle);
        glGenFramebuffers(1, &buffer->frameBufferHandle);
    }

    void destroyRenderBuffer(const RenderBuffer* buffer)
    {
        glDeleteFramebuffers(1, &buffer->frameBufferHandle);
        glDeleteTextures(1, &buffer->textureHandle);
    }

    void bindRenderBuffer(const RenderBuffer* buffer)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, buffer->frameBufferHandle);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer->textureHandle, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void renderBatch(const RenderBatch* batch)
    {
        glUseProgram(batch->shaderProgram);
        glBindVertexArray(batch->vertexArray);
    }

    void renderEntity(const RenderEntity* entity, const LocationsDescriptor* descriptor, DrawMode drawMode, int elementCount, const glm::vec2& pixelsPerUnit)
    {
        const Transform& entityTransform = entity->transform;

        glm::mat4 entityLocalToWorld    = entityTransform.localToWorld;
        glm::vec4& position             = entityLocalToWorld[3];

        position.x = glm::round(position.x * pixelsPerUnit.x) / pixelsPerUnit.x;
        position.y = glm::round(position.y * pixelsPerUnit.y) / pixelsPerUnit.y;

        const glm::mat4 transformMatrices[3] = {
            entityLocalToWorld,
            entityTransform.localRotation,
            entityTransform.localScale
        };

        glUniformMatrix4fv(descriptor->transformLocation, 3, GL_FALSE, &transformMatrices[0][0].x);
        glUniform4fv(descriptor->materialColorLocation, 1, glm::value_ptr(entity->material.color));
        glUniform2fv(descriptor->uvScaleLocation, 1, glm::value_ptr(entity->material.uvScale));
        glUniform2fv(descriptor->textureOffsetLocation, 1, glm::value_ptr(entity->material.textureOffset));
        glUniform1ui(descriptor->textureLayerLocation, entity->layer);

        glDrawElements(::drawModeToGLenum(drawMode), elementCount, GL_UNSIGNED_INT, nullptr);
    }

    void freeBuffers(const MutableGraphicsMemory& memory)
    {
        const auto bufferObjects = memory.bufferObjects;
        GLuint* bufferObjectData = bufferObjects.data();

        const size_t bufferCount = bufferObjects.size();
        
        OpenGLAllocator bufferAllocator(glGenBuffers, glDeleteBuffers, bufferObjectData, bufferObjectData + bufferCount);
        bufferAllocator.free(static_cast<GLsizei>(bufferCount));
    }

    void freeVertexArrays(const MutableGraphicsMemory& memory)
    {
        const auto vertexArrayObjects = memory.bufferObjects;
        GLuint* vertexArrayObjectData = vertexArrayObjects.data();

        const size_t vertexArrayCount = vertexArrayObjects.size();
        
        OpenGLAllocator vertexArrayAllocator(glGenVertexArrays, glDeleteVertexArrays, vertexArrayObjectData, vertexArrayObjectData + vertexArrayCount);
        vertexArrayAllocator.free(static_cast<GLsizei>(vertexArrayCount));
    }

    void freeTextures(const MutableGraphicsMemory& memory)
    {
        const auto textures = memory.textures;
        GLuint* textureData = textures.data();

        const size_t textureCount = textures.size();
        
        OpenGLAllocator textureAllocator(glGenTextures, glDeleteTextures, textureData, textureData + textureCount);
        textureAllocator.free(static_cast<GLsizei>(textureCount));
    }

    void freeShaders(const MutableGraphicsMemory& memory)
    {
        const auto shaderPrograms = memory.shaderPrograms;

        for (size_t i = 0; i < shaderPrograms.size(); ++i)
        {
            const GLuint program = *(shaderPrograms.data() + i);

            GLint attachedShaderCount{ 0 };
            glGetProgramiv(program, GL_ATTACHED_SHADERS, &attachedShaderCount);

            if (attachedShaderCount > 0)
            {
                GLuint* attachedShaders = new GLuint[attachedShaderCount];
                glGetAttachedShaders(program, attachedShaderCount, &attachedShaderCount, attachedShaders);
                
                for (GLint j = 0; j < attachedShaderCount; ++j)
                {
                    const GLuint shader = attachedShaders[j];

                    glDetachShader(program, shader);
                    glDeleteShader(shader);
                }

                delete[] attachedShaders;
            }

            glDeleteProgram(program);
        }
    }
}