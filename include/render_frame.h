#pragma once

#include <vector>

#include "render_command.h"

namespace renderer
{
    struct Frame
    {
        std::vector<RenderCommand> renderCommands;
    };
}
