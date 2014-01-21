#include "include/timing_arc.h"

namespace Timing_Analysis
{

    std::ostream &operator<<(std::ostream &out, const Timing_Arc &ta)
    {
        return out << ta.from().name() << " -> " << ta.to().name();
    }

    void Timing_Arc::clear()
    {
        _delay = numeric_limits<Transitions<double> >::zero();
        _slew = numeric_limits<Transitions<double> >::zero();
    }

}
