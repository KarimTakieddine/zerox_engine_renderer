#pragma once

#include <cstdint>

namespace renderer
{
    struct Shader
    {
        enum class Type : unsigned
        {
            UNKNOWN     = 0,
            VERTEX      = 1,
            FRAGMENT    = 2,
            GEOMETRY    = 3
        };

        enum class CompileStatus : int
        {
            UNCOMPILED  = 0,
            COMPILED    = 1
        };

        unsigned int id             { 0 };
        Type type                   { Type::UNKNOWN };
        CompileStatus compileStatus { CompileStatus::UNCOMPILED };
        unsigned int refCount       { 0 };
    };

    inline constexpr bool isActive(const Shader* shader)
    {
        return ( shader->type != Shader::Type::UNKNOWN ) && ( shader->id != 0 );
    }

    inline constexpr bool isCompiled(const Shader* shader)
    {
        return ( shader->compileStatus == Shader::CompileStatus::COMPILED );
    }

    struct ShaderLoadInfo
    {
        Shader::Type type   { Shader::Type::UNKNOWN };
        const char* content { nullptr };
    };
}
