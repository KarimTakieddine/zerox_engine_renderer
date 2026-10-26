#pragma once

#include "material.h"
#include "transform.h"

namespace renderer
{
    struct alignas(16) RenderEntity
    {
        Transform transform { };
        Material material   { };
        uint64_t layer      { 0 };
        uint64_t flags      { 0 };
    };
}
