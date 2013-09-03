#include "include/timing_net.h"


namespace Timing_Analysis
{

    const std::string Timing_Net::name() const
    {
        return _name;
    }

    std::ostream & operator<<(std::ostream &out, const Timing_Net &tn)
    {
        out << tn._name;
        return out;
    }

}
