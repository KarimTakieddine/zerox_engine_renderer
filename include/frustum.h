#pragma once

namespace renderer
{
    struct Frustum
    {
        float fov	{ 0.0f };
        float aspect{ 0.0f };
        float near	{ 0.0f };
        float far	{ 0.0f };
    };
}
