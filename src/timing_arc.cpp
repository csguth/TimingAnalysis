#include "include/timing_arc.h"

namespace Timing_Analysis
{

    std::ostream &operator<<(std::ostream &out, const Timing_Arc &ta)
    {
        return out << ta.from()->name() << " -> " << ta.to().name();
    }

}
