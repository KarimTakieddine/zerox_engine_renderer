#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <memory_cursor.hpp>
#include <memory_view.hpp>

#include <platform.h>

#include "locations_descriptor.h"
#include "opengl_allocator.h"
#include "renderer.hpp"
#include "shader.h"
#include "texture.h"
#include "uniform_buffer_segment.h"

namespace
{
    constexpr size_t MAX_SHADER_FILE_SIZE = 1 << 10;
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

            auto image = platform::loadImage(texture->path);

            if (!image->isLoaded())
            {
                return false;
            }

            const GLuint textureObject = *(textureObjects.data() + i);

            glBindTexture(GL_TEXTURE_2D, textureObject);

            switch (texture->wrapModeS)
			{
			case WrapMode::REPEAT:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				break;
			case WrapMode::CLAMP_TO_EDGE:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				break;
			case WrapMode::CLAMP_TO_BORDER:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				break;
			default:
				break;
			};

			switch (texture->wrapModeT)
			{
			case WrapMode::REPEAT:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				break;
			case WrapMode::CLAMP_TO_EDGE:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				break;
			case WrapMode::CLAMP_TO_BORDER:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
				break;
			default:
				break;
			};

			switch (texture->minFilter)
			{
			case Filter::NEAREST:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				break;
			case Filter::LINEAR:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				break;
			default:
				break;
			};

			switch (texture->magFilter)
			{
			case Filter::NEAREST:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				break;
			case Filter::LINEAR:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				break;
			default:
				break;
			};

			GLenum format = GL_NONE;

			switch (image->getFormat())
			{
			case platform::Image::Format::RGB:
				format = GL_RGB;
				break;
			case platform::Image::Format::RGBA:
				format = GL_RGBA;
				break;
			default:
				break;
			}

			glTexImage2D(
                GL_TEXTURE_2D, 0, format, image->getWidth(), image->getHeight(),
                0, format, GL_UNSIGNED_BYTE, image->getData());
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
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

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

    bool generateRenderBatch(const MutableGraphicsMemory& memory, size_t batchIndex, size_t vertexArrayIndex, size_t programIndex, size_t descriptorIndex)
    {
        const auto shaderPrograms = memory.shaderPrograms;
        if (programIndex >= shaderPrograms.size())
        {
            return false;
        }

        const GLuint shaderProgram = shaderPrograms.data()[programIndex];

        auto vertexArrays = memory.vertexArrayObjects;
        if (vertexArrayIndex >= vertexArrays.size())
        {
            return false;
        }

        const GLuint vertexArray = vertexArrays.data()[vertexArrayIndex];

        MutableMemoryView batchMemoryView(memory.renderBatchSpan);
        MemoryCursor<MEMORY_ALIGNMENT> batchCursor;

        const auto batchCount = batchMemoryView.read_object<uint64_t>(batchCursor.getOffset());

        if (batchIndex >= *batchCount.data())
        {
            return false;
        }

        batchCursor.step<uint64_t>();

        for (size_t i = 0; i < batchIndex; ++i)
        {
            batchCursor.step<RenderBatch>();

            auto entities = batchMemoryView.read_contiguous_array<RenderEntity>(batchCursor.getOffset());
            batchCursor.step_array<RenderEntity>(entities.size());
        }

        auto* batch = batchMemoryView.read_object<RenderBatch>(batchCursor.getOffset()).data();

        batch->descriptorIndex  = descriptorIndex;
        batch->vertexArray      = vertexArray;
        batch->shaderProgram    = shaderProgram;

        return true;
    }

    bool setVertexLayout(const MutableGraphicsMemory& memory, size_t batchIndex, size_t meshIndex)
    {
        ConstMemoryView meshView(memory.meshSpan);

        MemoryCursor<MEMORY_ALIGNMENT> meshCursor;
        const auto meshCount = meshView.read_object<uint64_t>(meshCursor.getOffset());

        if (meshIndex >= *meshCount.data())
        {
            return false;
        }

        meshCursor.step<uint64_t>();

        for (size_t i = 0; i < meshIndex; ++i)
        {
            meshCursor.step<MeshBufferIndices>();

            const auto vertices = meshView.read_contiguous_array<Vertex>(meshCursor.getOffset());
            meshCursor.step_array<Vertex>(vertices.size());

            const auto triangles = meshView.read_contiguous_array<GLuint>(meshCursor.getOffset());
            meshCursor.step_array<GLuint>(triangles.size());
        }

        const auto* bufferIndices = meshView.read_object<MeshBufferIndices>(meshCursor.getOffset()).data();
        meshCursor.step<MeshBufferIndices>();

        const auto vertices = meshView.read_contiguous_array<Vertex>(meshCursor.getOffset());
        meshCursor.step_array<Vertex>(vertices.size());

        const auto triangles = meshView.read_contiguous_array<GLuint>(meshCursor.getOffset());

        MutableMemoryView renderBatchView(memory.renderBatchSpan);
        MemoryCursor<MEMORY_ALIGNMENT> batchCursor;

        const auto batchCount = renderBatchView.read_object<uint64_t>(batchCursor.getOffset());

        if (batchIndex >= *batchCount.data())
        {
            return false;
        }

        batchCursor.step<uint64_t>();

        for (size_t i = 0; i < batchIndex; ++i)
        {
            batchCursor.step<RenderBatch>();

            auto entities = renderBatchView.read_contiguous_array<RenderEntity>(batchCursor.getOffset());
            batchCursor.step_array<RenderEntity>(entities.size());
        }

        auto* batch = renderBatchView.read_object<RenderBatch>(batchCursor.getOffset()).data();

        batch->elememtCount = static_cast<int>(triangles.size());

        const auto locationsDescriptors = memory.locationsDescriptors;
        const size_t descriptorIndex    = batch->descriptorIndex;

        if (descriptorIndex >= locationsDescriptors.size())
        {
            return false;
        }

        const auto* locationsDescriptor = locationsDescriptors.data() + descriptorIndex;

        glUseProgram(batch->shaderProgram);
        glBindVertexArray(batch->vertexArray);
        glBindBuffer(GL_ARRAY_BUFFER, bufferIndices->vertex);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferIndices->triangle);

        glEnableVertexAttribArray(locationsDescriptor->positionLocation);
        glVertexAttribPointer(locationsDescriptor->positionLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);

        glEnableVertexAttribArray(locationsDescriptor->colorLocation);
        glVertexAttribPointer(locationsDescriptor->colorLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(sizeof(glm::vec3)));

        glEnableVertexAttribArray(locationsDescriptor->uvLocation);
        glVertexAttribPointer(locationsDescriptor->uvLocation, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(2 * sizeof(glm::vec3)));

        return true;
    }

    void renderBatch(const RenderBatchSpan<true>& span)
    {
        const auto* renderBatch = span.renderBatch.data();

        glUseProgram(renderBatch->shaderProgram);
        glBindVertexArray(renderBatch->vertexArray);
    }

    void renderEntity(const RenderEntity* entity, const LocationsDescriptor* descriptor, int elementCount)
    {
        glUniformMatrix4fv(descriptor->transformLocation, 3, GL_FALSE, glm::value_ptr(entity->transform.localToWorld));
        glUniform4fv(descriptor->materialColorLocation, 1, glm::value_ptr(entity->material.color));
        glUniform2fv(descriptor->uvScaleLocation, 1, glm::value_ptr(entity->material.uvScale));
        glUniform2fv(descriptor->textureOffsetLocation, 1, glm::value_ptr(entity->material.textureOffset));

        glDrawElements(GL_TRIANGLES, elementCount, GL_UNSIGNED_INT, nullptr);
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