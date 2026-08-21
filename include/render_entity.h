#pragma once

#include "material.h"
#include "transform.h"

namespace renderer
{
    struct RenderEntity
    {
        Transform transform { };
        Material material   { };
    };
}
