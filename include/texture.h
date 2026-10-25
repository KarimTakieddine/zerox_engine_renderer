#pragma once

#include "texture_data.h"

namespace renderer
{
    struct Texture
    {
        TextureData data{ };
        const char* path{ nullptr };
    };
}
