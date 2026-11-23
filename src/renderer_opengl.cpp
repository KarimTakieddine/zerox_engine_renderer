#include <string>

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include <memory_cursor.hpp>
#include <memory_view.hpp>

#include <platform.h>
#include <zerox_render_entity.h>

#include "opengl_allocator.h"
#include "render_buffer.h"
#include "renderer.hpp"

namespace
{
    constexpr size_t MAX_SHADER_FILE_SIZE = 1 << 10;

    using renderer::DrawMode;
    using ZeroXBridge::TextureFilter;
    using ZeroXBridge::TextureWrapMode;

    constexpr GLenum drawModeToGLenum(DrawMode drawMode)
    {
        switch (drawMode)
        {
        case DrawMode::TRIANGLES:   return GL_TRIANGLES;
        case DrawMode::LINES:       return GL_LINES;
        default:                    return GL_NONE;
        }
    }

    constexpr GLenum wrapModeToGLenum(TextureWrapMode wrapMode)
    {
        switch (wrapMode)
        {
        case TextureWrapMode::REPEAT:          return GL_REPEAT;
        case TextureWrapMode::CLAMP_TO_EDGE:   return GL_CLAMP_TO_EDGE;
        case TextureWrapMode::CLAMP_TO_BORDER: return GL_CLAMP_TO_BORDER;
        default:                                return GL_NONE;
        }
    }

    constexpr GLenum filterToGLenum(TextureFilter filter)
    {
        switch (filter)
        {
        case TextureFilter::NEAREST:    return GL_NEAREST;
        case TextureFilter::LINEAR:     return GL_LINEAR;
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

    using renderer::Shader;

    constexpr GLenum shaderTypeToGLenum(Shader::Type type)
    {
        switch(type)
        {
        case Shader::Type::VERTEX:      return GL_VERTEX_SHADER;
        case Shader::Type::FRAGMENT:    return GL_FRAGMENT_SHADER;
        case Shader::Type::GEOMETRY:    return GL_GEOMETRY_SHADER;
        default:                        return GL_NONE;
        }
    }
    
    constexpr renderer::GPUDataType glEnumToVariableType(GLenum type)
    {
        switch (type)
        {
        case(GL_INT):               return renderer::GPUDataType::INT;
        case(GL_UNSIGNED_INT):      return renderer::GPUDataType::UINT;
        case(GL_FLOAT):             return renderer::GPUDataType::FLOAT;
        case(GL_UNSIGNED_INT_VEC2): return renderer::GPUDataType::UVEC_2;
        case(GL_FLOAT_VEC2):        return renderer::GPUDataType::VEC_2;
        case(GL_FLOAT_VEC3):        return renderer::GPUDataType::VEC_3;
        case(GL_FLOAT_VEC4):        return renderer::GPUDataType::VEC_4;
        case(GL_FLOAT_MAT4):        return renderer::GPUDataType::MAT_4;
        case(GL_SAMPLER_2D):        return renderer::GPUDataType::SAMPLER_2D;
        case(GL_SAMPLER_2D_ARRAY):  return renderer::GPUDataType::SAMPLER_2D_ARRAY;
        default:                    return renderer::GPUDataType::UNKNOWN;
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

    bool uploadTextures(const ConstGraphicsMemory& memory, size_t count, const char* const* paths, const ZeroXBridge::Texture* textures)
    {
        if (!paths)
        {
            return false;
        }

        const auto textureObjects = memory.textures;

        const size_t objectCount = textureObjects.size();

        if (count > objectCount)
        {
            count = objectCount;
        }

        for (size_t i = 0; i < count; ++i)
        {
            const auto* texture = textures + i;

            auto image = platform::loadImage(paths[i], false);

            if (!image->isLoaded())
            {
                return false;
            }

            const ZeroXBridge::Texture* textureData = textures + i;

            setTextureParameters(*(textureObjects.data() + i), GL_TEXTURE_2D, textureData);

            const GLenum format = ::imageFormatToGLenum(image->getFormat());

			glTexImage2D(
                GL_TEXTURE_2D, 0, format, image->getWidth(), image->getHeight(),
                0, format, GL_UNSIGNED_BYTE, image->getData());
        }

        return true;
    }

    bool uploadTextureArray(const ConstGraphicsMemory& memory, const ZeroXBridge::Texture& texture, size_t textureIndex, size_t textureCount, const char* const* imageFiles)
    {
        const auto textures = memory.textures;
        if (textureIndex >= textures.size())
        {
            return false;
        }

        setTextureParameters(*(textures.data() + textureIndex), GL_TEXTURE_2D_ARRAY, &texture);

        glTexStorage3D(
            GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8,
            texture.width, texture.height,
            static_cast<GLsizei>(textureCount));

        for (size_t i = 0; i < textureCount; ++i)
        {
            auto image = platform::loadImage(*(imageFiles + i), false);

            if (!image->isLoaded())
            {
                return false;
            }

            if (image->getWidth() != texture.width || image->getHeight() != texture.height)
            {
                return false;
            }

            glTexSubImage3D(
                GL_TEXTURE_2D_ARRAY, 0,
                0, 0, i,
                texture.width, texture.height, 1,
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

    bool createShader(Shader* shader, Shader::Type type)
    {
        unsigned int& refCount = shader->refCount;

        if (isActive(shader) || refCount != 0)
        {
            return false;
        }

        *shader = {
            .id     = glCreateShader(::shaderTypeToGLenum(type)),
            .type   = type
        };

        if (!isActive(shader))
        {
            return false;
        }

        refCount = 1;

        return true;
    }

    bool compileShader(Shader* shader, const char* content)
    {
        if (!isActive(shader))
        {
            return false;
        }

        const GLuint id = static_cast<GLuint>(shader->id);

        glShaderSource(id, 1, &content, nullptr);
        glCompileShader(id);

        GLint compileStatus{ GL_FALSE };
        glGetShaderiv(id, GL_COMPILE_STATUS, &compileStatus);

        shader->compileStatus = static_cast<Shader::CompileStatus>(compileStatus);

        return ( compileStatus == GL_TRUE );
    }

    bool loadShader(Shader* shader, const ShaderLoadInfo* loadInfo)
    {
        return
            createShader(shader, loadInfo->type) &&
            compileShader(shader, loadInfo->content);
    }

    bool loadShaders(const MutableGraphicsMemory& memory, const ShaderLoadInfo* loadInfo, size_t count)
    {
        if (!loadInfo)
        {
            return false;
        }

        const auto shaders = memory.shaders;

        const size_t shaderCount = shaders.size();

        if (count > shaderCount)
        {
            count = shaderCount;
        }

        bool allLoaded{ true };
        for (size_t i = 0; i < count; ++i)
        {
            Shader* shader                          = shaders.data() + i;
            const ShaderLoadInfo* shaderLoadInfo    = loadInfo + i;

            if (!loadShader(shader, loadInfo + i))
            {
                allLoaded = false;
            }
        }

        return allLoaded;
    }

    void unloadShader(Shader* shader)
    {
        unsigned int& refCount = shader->refCount;

        if (refCount == 0)
        {
            return;
        }

        --refCount;

        if (refCount == 0)
        {
            if (!isActive(shader))
            {
                return;
            }

            glDeleteShader(shader->id);
       
            shader->id              = 0;
            shader->type            = Shader::Type::UNKNOWN;
            shader->compileStatus   = static_cast<Shader::CompileStatus>(GL_FALSE);
        }
    }

    void unloadShaders(Shader* shaders, size_t count)
    {
        assert(shaders);

        size_t i = count;

        while (i --> 0)
        {
            unloadShader(shaders + i);
        }
    }

    void unloadShaders(const MutableGraphicsMemory& memory)
    {
        const auto shaders = memory.shaders;

        unloadShaders(shaders.data(), shaders.size());
    }

    bool createShaderProgram(ShaderProgram* program)
    {
        if (isActive(program) || program->refCount != 0)
        {
            return false;
        }

        program->id = glCreateProgram();

        if (!isActive(program))
        {
            return false;
        }

        program->refCount = 1;

        return true;
    }

    bool linkShaderProgram(const MutableGraphicsMemory& memory, ShaderProgram* program, const size_t* indices, size_t count)
    {
        if (!indices)
        {
            return false;
        }

        const auto shaders = memory.shaders;

        const size_t shaderCount = shaders.size();
        
        if (count > shaderCount)
        {
            count = shaderCount;
        }

        const GLuint programId = program->id;

        for (size_t i = 0; i < count; ++i)
        {
            Shader* shader = shaders.data() + indices[i];

            glAttachShader(programId, shader->id);

            ++shader->refCount;
        }

        program->shaderIndices  = indices;
        program->shaderCount    = count;

        glLinkProgram(programId);

        GLint linkStatus{ GL_FALSE };
        glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);
        program->linkStatus = static_cast<ShaderProgram::LinkStatus>(linkStatus);

        return ( linkStatus == GL_TRUE );
    }

    bool loadShaderProgram(const MutableGraphicsMemory& memory, ShaderProgram* program, const ShaderCompileStep* shaderCompileStep)
    {
        return
            createShaderProgram(program) &&
            linkShaderProgram(memory, program, shaderCompileStep->shaderIndices, shaderCompileStep->shaderCount);
    }

    bool loadShaderPrograms(const MutableGraphicsMemory& memory, const ShaderCompileStep* shaderCompileSteps, size_t compileStepCount)
    {
        const auto shaderPrograms = memory.shaderPrograms;

        const size_t shaderProgramCount = shaderPrograms.size();

        if (compileStepCount > shaderProgramCount)
        {
            compileStepCount = shaderProgramCount;
        }

        bool allLinked{ true };
        for (size_t i = 0; i < compileStepCount; ++i)
        {
            const ShaderCompileStep* step = shaderCompileSteps + i;

            if (!loadShaderProgram(memory, shaderPrograms.data() + i, shaderCompileSteps + i))
            {
                allLinked = false;
            }
        }

        return allLinked;
    }

    void unloadShaderProgram(const MutableGraphicsMemory& memory, ShaderProgram* program)
    {
        unsigned int& refCount = program->refCount;

        if (refCount == 0)
        {
            return;
        }

        --refCount;

        if (refCount == 0)
        {
            if (!isActive(program))
            {
                return;
            }

            const GLuint programId = program->id;

            size_t shaderIndex = program->shaderCount;

            while (shaderIndex --> 0)
            {
                Shader* shader = memory.shaders.data() + program->shaderIndices[shaderIndex];

                glDetachShader(programId, shader->id);

                --shader->refCount;
            }

            glDeleteProgram(programId);
        }
    }

    void unloadShaderPrograms(const MutableGraphicsMemory& memory, ShaderProgram* programs, size_t count)
    {
        size_t index = count;

        while (index-- > 0)
        {
            unloadShaderProgram(memory, programs + index);
        }
    }

    void unloadShaderPrograms(const MutableGraphicsMemory& memory)
    {
        const auto shaderPrograms = memory.shaderPrograms;

        unloadShaderPrograms(memory, shaderPrograms.data(), shaderPrograms.size());
    }

    bool loadShaderUniforms(const ShaderProgram* program, size_t* count, char* nameBuffer, ShaderVariable* buffer)
    {
        if (!isActive(program))
        {
            return false;
        }
        
        const GLuint programId = program->id;

        GLint activeUniformCount{ 0 };
        glGetProgramiv(programId, GL_ACTIVE_UNIFORMS, &activeUniformCount);

        if (activeUniformCount <= 0)
        {
            return false;
        }

        if (count)
        {
            *count = static_cast<size_t>(activeUniformCount);
        }

        if (!nameBuffer || !buffer)
        {
            return true;
        }

        bool allLoaded{ true };
        for (GLint i = 0; i < activeUniformCount; ++i)
        {
            ShaderVariable* variable = buffer + i;

            GLchar* uniformName = nameBuffer + (i * MAX_VARIABLE_NAME_LENGTH);

            GLsizei uniformNameLength   { 0 };
            GLenum uniformType          { GL_NONE };
            GLint uniformCount          { 0 };
            glGetActiveUniform(
                programId,
                static_cast<GLuint>(i),
                static_cast<GLsizei>(MAX_VARIABLE_NAME_LENGTH),
                &uniformNameLength,
                &uniformCount,
                &uniformType,
                uniformName);

            if (uniformNameLength > static_cast<GLsizei>(MAX_VARIABLE_NAME_LENGTH) - 1)
            {
                allLoaded = false;
            }

            *variable = {
                .name       = uniformName,
                .location   = glGetUniformLocation(programId, uniformName),
                .type       = ::glEnumToVariableType(uniformType),
                .count      = static_cast<uint16_t>(uniformCount)
            };
        }

        return allLoaded;
    }

    // TODO(Karim): These need moving to generic ::renderer functionality

    bool loadShaderUniforms(Allocator* allocator, ShaderProgram* programs, size_t count)
    {
        size_t totalUniformCount{ 0 };

        for (size_t i = 0; i < count; ++i)
        {
            size_t uniformCount{ 0 };
            if (loadShaderUniforms(programs + i, &uniformCount, nullptr, nullptr))
            {
                totalUniformCount += uniformCount;
            }
        }

        if (totalUniformCount == 0)
        {
            return false;
        }

        const size_t nameBufferSize = static_cast<size_t>(totalUniformCount * MAX_VARIABLE_NAME_LENGTH);
        allocator->requestMemory<uint64_t>(nameBufferSize);
        auto* uniformNameBuffer = allocator->requestMemoryArray<GLchar>(nameBufferSize, '\0');

        const size_t uniformBufferSize = static_cast<size_t>(totalUniformCount + 1);
        allocator->requestMemory<uint64_t>(uniformBufferSize);
        auto* uniformBuffer = allocator->requestMemoryArray<ShaderVariable>(uniformBufferSize);

        bool allLoaded{ true };
        for (size_t i = 0; i < count; ++i)
        {
            ShaderProgram* program = programs + i;

            size_t uniformCount{ 0 };
            if (!loadShaderUniforms(program, &uniformCount, nullptr, nullptr))
            {
                allLoaded = false;
                continue;
            }

            if (!loadShaderUniforms(program, nullptr, uniformNameBuffer, uniformBuffer))
            {
                allLoaded = false;
                continue;
            }

            program->uniforms       = uniformBuffer;
            program->uniformCount   = uniformCount;

            uniformBuffer       += uniformCount;
            uniformNameBuffer   += uniformCount * MAX_VARIABLE_NAME_LENGTH;
        }

        return allLoaded;
    }

    bool loadShaderUniforms(Allocator* allocator, const MutableGraphicsMemory& memory)
    {
        const auto shaderPrograms = memory.shaderPrograms;

        return loadShaderUniforms(allocator, shaderPrograms.data(), shaderPrograms.size());
    }

    bool loadShaderAttributes(const ShaderProgram* program, size_t* count, char* nameBuffer, ShaderVariable* buffer)
    {
        if (!program || !isActive(program))
        {
            return false;
        }

        const GLuint programId = program->id;

        GLint attributeCount{ 0 };
        glGetProgramiv(programId, GL_ACTIVE_ATTRIBUTES, &attributeCount);

        if (count)
        {
            *count = static_cast<size_t>(attributeCount);
        }

        if (!nameBuffer || !buffer)
        {
            return true;
        }

        bool allLoaded{ true };
        for (GLint i = 0; i < attributeCount; ++i)
        {
            GLchar* attributeName = nameBuffer + ( i * MAX_VARIABLE_NAME_LENGTH );

            GLsizei nameLength  { 0 };
            GLint count         { 0 };
            GLenum attributeType{ GL_NONE };

            glGetActiveAttrib(
                programId,
                static_cast<GLuint>(i),
                static_cast<GLsizei>(MAX_VARIABLE_NAME_LENGTH),
                &nameLength,
                &count,
                &attributeType,
                attributeName);

            if (nameLength > static_cast<GLsizei>(MAX_VARIABLE_NAME_LENGTH) - 1)
            {
                allLoaded = false;
            }

            *(buffer + i) = {
                .name       = attributeName,
                .location   = glGetAttribLocation(programId, attributeName),
                .type       = ::glEnumToVariableType(attributeType),
                .count      = static_cast<uint16_t>(count)
            };
        }

        return allLoaded;
    }

    bool loadShaderAttributes(Allocator* allocator, ShaderProgram* programs, size_t count)
    {
        size_t totalAttributeCount{ 0 };

        for (size_t i = 0; i < count; ++i)
        {
            size_t attributeCount{ 0 };
            if (loadShaderAttributes(programs + i, &attributeCount, nullptr, nullptr))
            {
                totalAttributeCount += attributeCount;
            }
        }

        const size_t namesBufferSize{ totalAttributeCount * MAX_VARIABLE_NAME_LENGTH };
        allocator->requestMemory<uint64_t>(namesBufferSize);
        auto* namesBuffer = allocator->requestMemoryArray<GLchar>(namesBufferSize, '\0');

        if (totalAttributeCount == 0)
        {
            return false;
        }

        const size_t bufferSize = totalAttributeCount + 1;
        allocator->requestMemory<uint64_t>(bufferSize);
        auto* buffer = allocator->requestMemoryArray<ShaderVariable>(bufferSize);

        bool allLoaded{ true };
        for (size_t i = 0; i < count; ++i)
        {
            ShaderProgram* shaderProgram = programs + i;

            size_t attributeCount{ 0 };
            if (!loadShaderAttributes(shaderProgram, &attributeCount, nullptr, nullptr))
            {
                allLoaded = false;
                continue;
            }

            if (!loadShaderAttributes(shaderProgram, &attributeCount, namesBuffer, buffer))
            {
                allLoaded = false;
                continue;
            }

            shaderProgram->attributes       = buffer;
            shaderProgram->attributeCount   = attributeCount;

            buffer      += attributeCount;
            namesBuffer += attributeCount * MAX_VARIABLE_NAME_LENGTH;
        }

        return allLoaded;
    }

    bool loadShaderAttributes(Allocator* allocator, const MutableGraphicsMemory& memory)
    {
        const auto shaderPrograms = memory.shaderPrograms;

        return loadShaderAttributes(allocator, shaderPrograms.data(), shaderPrograms.size());
    }

    bool loadEntityBindings(const ShaderProgram* program, EntityBindings* entityBindings)
    {
        if (!isActive(program))
        {
            return false;
        }

        for (size_t i = 0; i < program->attributeCount; ++i)
        {
            const ShaderVariable* attribute = program->attributes + i;

            switch (attribute->type)
            {
            case GPUDataType::VEC_2:
                if (attribute->count == 1)
                {
                    entityBindings->uvAttribute = attribute;
                }
                break;
            case GPUDataType::VEC_3:
                if (attribute->count == 1)
                {
                    const std::string name(attribute->name);

                    if (name == "color")
                    {
                        entityBindings->colorAttribute = attribute;
                    }
                    else if (name == "position")
                    {
                        entityBindings->positionAttribute = attribute;
                    }
                }
                break;
            case GPUDataType::VEC_4:
                if (attribute->count == 1)
                {
                    entityBindings->colorAttribute = attribute;
                }
                break;
            default:
                break;
            }
        }

        for (size_t i = 0; i < program->uniformCount; ++i)
        {
            const ShaderVariable* uniform = program->uniforms + i;

            switch (uniform->type)
            {
            case GPUDataType::MAT_4:
                if (uniform->count == 3)
                {
                    entityBindings->transformUniform = uniform;
                }
                break;
            case GPUDataType::VEC_2:
                if (uniform->count == 2)
                {
                    entityBindings->uvUniform = uniform;
                }
                break;
            case GPUDataType::VEC_4:
                if (uniform->count == 1)
                {
                    entityBindings->materialColorUniform = uniform;
                }
                break;
            case GPUDataType::UINT:
                if (uniform->count == 1)
                {
                    entityBindings->layerUniform = uniform;
                }
                break;
            case GPUDataType::UVEC_2:
                if (uniform->count == 1)
                {
                    entityBindings->tileSizeUniform = uniform;
                }
                break;
            default:
                break;
            }
        }

        return isValid(entityBindings);
    }

    bool loadEntityBindings(Allocator* allocator, const ConstGraphicsMemory& memory, const ShaderProgram* programs, size_t count)
    {
        if (!programs)
        {
            return false;
        }

        allocator->requestMemory<uint64_t>(static_cast<uint64_t>(count));
        auto* entityBindings = allocator->requestMemoryArray<EntityBindings>(count);

        bool allLoaded{ true };
        for (size_t i = 0; i < count; ++i)
        {
            if (!loadEntityBindings(programs + i, entityBindings))
            {
                allLoaded = false;

                const auto shaderAttributes = memory.shaderAttributes;
                const auto shaderUniforms   = memory.shaderUniforms;

                if (!entityBindings->positionAttribute)
                {
                    entityBindings->positionAttribute = ( shaderAttributes.data() + shaderAttributes.size() - 1 );
                }

                if (!entityBindings->colorAttribute)
                {
                    entityBindings->colorAttribute = ( shaderAttributes.data() + shaderAttributes.size() - 1 );
                }

                if (!entityBindings->uvAttribute)
                {
                    entityBindings->uvAttribute = ( shaderAttributes.data() + shaderAttributes.size() - 1 );
                }

                if (!entityBindings->transformUniform)
                {
                    entityBindings->transformUniform = ( shaderUniforms.data() + shaderUniforms.size() - 1 );
                }

                if (!entityBindings->uvUniform)
                {
                    entityBindings->uvUniform = ( shaderUniforms.data() + shaderUniforms.size() - 1 );
                }

                if (!entityBindings->materialColorUniform)
                {
                    entityBindings->materialColorUniform = ( shaderUniforms.data() + shaderUniforms.size() - 1 );
                }

                if (!entityBindings->tileSizeUniform)
                {
                    entityBindings->tileSizeUniform = ( shaderUniforms.data() + shaderUniforms.size() - 1 );
                }

                if (!entityBindings->layerUniform)
                {
                    entityBindings->layerUniform = ( shaderUniforms.data() + shaderUniforms.size() - 1 );
                }
            }

            ++entityBindings;
        }

        return allLoaded;
    }

    bool loadEntityBindings(Allocator* allocator, const ConstGraphicsMemory& memory)
    {
        const auto shaderPrograms = memory.shaderPrograms;

        return loadEntityBindings(allocator, memory, shaderPrograms.data(), shaderPrograms.size());
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

        const GLuint shaderProgram = shaderPrograms.data()[programIndex].id;

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

    void setVertexLayout(const ShaderProgram* program, const EntityBindings* entityBindings, unsigned int vertexArray, const MeshBufferIndices* buffers)
    {
        glUseProgram(program->id);
        glBindVertexArray(vertexArray);
        glBindBuffer(GL_ARRAY_BUFFER, buffers->vertex);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers->triangle);

        const ShaderVariable* positionAttribute = entityBindings->positionAttribute;
        glEnableVertexAttribArray(positionAttribute->location);
        glVertexAttribPointer(positionAttribute->location, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);

        const ShaderVariable* colorAttribute = entityBindings->colorAttribute;
        glEnableVertexAttribArray(colorAttribute->location);
        glVertexAttribPointer(colorAttribute->location, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(sizeof(glm::vec3)));

        const ShaderVariable* uvAttribute = entityBindings->uvAttribute;
        glEnableVertexAttribArray(uvAttribute->location);
        glVertexAttribPointer(uvAttribute->location , 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(2 * sizeof(glm::vec3)));
    }

    void setTextureParameters(unsigned int texture, unsigned int target, const ZeroXBridge::Texture* parameters, bool unbind /* = false */)
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

    void renderBatch(const ShaderProgram* shaderProgram, unsigned int vertexArray)
    {
        glUseProgram(shaderProgram->id);
        glBindVertexArray(vertexArray);
    }

    void renderEntity(const ZeroXBridge::RenderEntity* entity, const EntityBindings* entityBindings, DrawMode drawMode, int elementCount, const glm::vec2& pixelsPerUnit)
    {
        ZeroXBridge::Transform entityTransform = entity->transform;

        glm::vec4& position = entityTransform.localToWorld[3];

        position.x = glm::round(position.x * pixelsPerUnit.x) / pixelsPerUnit.x;
        position.y = glm::round(position.y * pixelsPerUnit.y) / pixelsPerUnit.y;

        const ShaderVariable* transformUniform = entityBindings->transformUniform;
        glUniformMatrix4fv(transformUniform->location, static_cast<GLsizei>(transformUniform->count), GL_FALSE, glm::value_ptr(entityTransform.localToWorld));

        const ZeroXBridge::Material& material = entity->material;

        const ShaderVariable* colorUniform = entityBindings->materialColorUniform;
        glUniform4fv(colorUniform->location, static_cast<GLsizei>(colorUniform->count), glm::value_ptr(material.color));

        const ShaderVariable* uvUniform = entityBindings->uvUniform;
        glUniform2fv(uvUniform->location, static_cast<GLsizei>(uvUniform->count), glm::value_ptr(material.uvScale));

        glUniform2ui(entityBindings->tileSizeUniform->location, static_cast<GLuint>(material.tileSizeX), static_cast<GLuint>(material.tileSizeY));
        glUniform1ui(entityBindings->layerUniform->location, entity->layer);

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
        unloadShaderPrograms(memory);
        unloadShaders(memory);
    }
}