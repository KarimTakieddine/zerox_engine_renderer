#pragma once

#include <atomic>
#include <vector>

#include "render_frame.h"

namespace renderer
{
    class FrameBuffer
    {
    public:
        void allocate(size_t capacity);
        bool push(const Frame& frame);
        bool pop(Frame& outFrame);

    private:
        std::vector<Frame> m_data;

        alignas(64) std::atomic<size_t> m_head  { 0 };
        alignas(64) std::atomic<size_t> m_tail  { 0 };
        size_t m_capacityMask                   { 0 };
    };
}
