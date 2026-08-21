#pragma once

namespace renderer
{
    struct LocationsDescriptor
    {
        int positionLocation        { -1 };
        int colorLocation           { -1 };
        int uvLocation              { -1 };
        int transformLocation       { -1 };
        int materialColorLocation   { -1 };
    };
}
