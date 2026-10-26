#pragma once

#include <cstddef>
#include <cstdint>
#include <stdalign.h>

namespace renderer
{
    enum class CommandType : unsigned char
    {
        UNKNOWN                 = 0,
        ALLOCATE_RENDER_BATCH   = 1,
        UPDATE_ENTITY_TRANSFORM = 2,
        UPDATE_ENTITY_MATERIAL  = 3,
        SET_CLIENT_AREA         = 4,
        SET_TEXTURE_LAYER       = 5,
        LAST                    = SET_TEXTURE_LAYER
    };

    enum class CommandFlags : unsigned char
    {
        NONE = 0
    };

    struct alignas(256) Command
    {
        static constexpr size_t DATA_SECTION_SIZE = 254;

        std::byte data[DATA_SECTION_SIZE];

        CommandType type    { CommandType::UNKNOWN };
        CommandFlags flags  { CommandFlags::NONE };
    };
}
