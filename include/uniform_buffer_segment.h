#pragma once

#include <cstddef>
#include <cstdint>

namespace renderer
{
    struct UniformBufferSegment
    {
        const void* data    { nullptr };
        size_t stride       { 0 };
        intptr_t offset     { 0 };
    };
}
