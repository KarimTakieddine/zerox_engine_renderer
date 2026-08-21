#pragma once

#include <cstddef>
#include <cstdint>

#include <functional>

namespace renderer
{
    using OpenGLFunc = std::function<void(int, unsigned int*)>;

    class OpenGLAllocator
    {
    public:
        explicit OpenGLAllocator(OpenGLFunc genFunc, OpenGLFunc delFunc, unsigned int* data, unsigned int* bounds) :
            m_genFunc(std::move(genFunc)), m_delFunc(std::move(delFunc)), m_data(data), m_next(data), m_bounds(bounds) { };

        void allocate(int count)
        {
            m_genFunc(count, m_bounds);

            m_bounds += count;
        }

        void free(int count)
        {
            auto* nextBounds = m_bounds - count;

            if (nextBounds < m_data)
            {
                return;
            }

            m_delFunc(count, nextBounds);

            if (m_next > nextBounds)
            {
                m_next = nextBounds;
            }

            m_bounds = nextBounds;
        }

        unsigned int get()
        {
            unsigned int* next = m_next + 1;

            if (next > m_bounds)
            {
                return 0;
            }

            const unsigned int result = *m_next;

            m_next = next;

            return result;
        }

        uint64_t getLiveObjectCount() const
        {
            return m_bounds - m_data;
        }

        uint64_t getInUseObjectCount() const
        {
            return m_next - m_data;
        }
    
    private:
        OpenGLFunc m_genFunc    { nullptr };
        OpenGLFunc m_delFunc    { nullptr };

        unsigned int* m_data    { nullptr };
        unsigned int* m_next    { nullptr };
        unsigned int* m_bounds  { nullptr };
    };
}
