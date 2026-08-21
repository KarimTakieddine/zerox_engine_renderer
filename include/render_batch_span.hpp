#pragma once

#include "spans.hpp"
#include "render_batch.h"
#include "render_entity.h"

namespace renderer
{
    template<bool IsConst = false>
    struct RenderBatchSpan
    {
        DynamicSpan<RenderBatch, IsConst> renderBatch;
        DynamicSpan<RenderEntity, IsConst> entities;
    };
}
