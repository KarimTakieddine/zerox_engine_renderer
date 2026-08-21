#pragma once

namespace renderer
{
    struct Shader
    {
        enum class Type : unsigned
        {
            UNKNOWN     = 0,
            VERTEX      = 1,
            FRAGMENT    = 2
        };

        const char* path    { nullptr };
        Type type           { Type::UNKNOWN };
    };
}
