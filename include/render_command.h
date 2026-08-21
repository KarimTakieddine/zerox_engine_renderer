#pragma once

#include <cstddef>
#include <cstdint>
#include <stdalign.h>

namespace renderer
{
    enum class CommandType : unsigned char
    {
        UNKNOWN                 = 0,
        UPDATE_ENTITY_MATERIAL  = 1,
        UPDATE_ENTITY_TRANSFORM = 2
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
