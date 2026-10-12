#pragma once

namespace renderer
{
    struct Frustum
    {
        float fov	    { 0.0f };
        float aspect    { 0.0f };
        float nearPlane	{ 0.0f };
        float farPlane	{ 0.0f };
    };
}
