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
        SET_BATCH_VERTEX_LAYOUT = 2,
        UPDATE_ENTITY_MATERIAL  = 3,
        UPDATE_ENTITY_TRANSFORM = 4,
        SET_CAMERA_ASPECT_RATIO = 5
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
