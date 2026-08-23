#pragma once

namespace renderer
{
    enum class WrapMode : unsigned
    {
        UNKNOWN         = 0,
        REPEAT          = 1,
        CLAMP_TO_EDGE   = 2,
        CLAMP_TO_BORDER = 3
    };

    enum class Filter : unsigned
    {
        UNKNOWN = 0,
        NEAREST = 1,
        LINEAR  = 2
    };

    struct Texture
    {
        const char* path    { nullptr };
        WrapMode wrapModeS  { WrapMode::UNKNOWN };
        WrapMode wrapModeT  { WrapMode::UNKNOWN };
        Filter minFilter    { Filter::UNKNOWN };
        Filter magFilter    { Filter::UNKNOWN };
    };
}
