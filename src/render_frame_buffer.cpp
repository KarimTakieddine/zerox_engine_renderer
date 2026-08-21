#include "render_frame_buffer.h"
#include <stdexcept>

namespace renderer
{
    void FrameBuffer::allocate(size_t capacity)
    {
        if (capacity == 0 || (capacity & (capacity - 1)) != 0)
            throw std::runtime_error("Capacity must be a power of 2!");

        m_data.resize(capacity);

        m_capacityMask = capacity - 1;
    }

    bool FrameBuffer::push(const Frame& frame) 
    {
        const size_t current_tail = m_tail.load(std::memory_order_relaxed);
        const size_t next_tail    = (current_tail + 1) & m_capacityMask;

        if (next_tail == m_head.load(std::memory_order_acquire)) {
            return false;
        }

        m_data[current_tail] = frame;

        m_tail.store(next_tail, std::memory_order_release);

        return true;
    }

    bool FrameBuffer::pop(Frame& outFrame)
    {
        const size_t current_head = m_head.load(std::memory_order_relaxed);

        if (current_head == m_tail.load(std::memory_order_acquire)) {
            return false;
        }

        outFrame = m_data[current_head];

        m_head.store((current_head + 1) & m_capacityMask, std::memory_order_release);

        return true;
    }
}