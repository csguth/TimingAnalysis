#ifndef MULTI_FANOUT_EDGE_H
#define MULTI_FANOUT_EDGE_H
#include "edge.h"
#include "timing_point.h"

namespace Timing_Analysis
{

    template <class T>
    class Multi_Fanout_Edge : public Edge<T>
    {
    public:
        Multi_Fanout_Edge(T * from) : Edge<T>(from) {}
        const T & to(const int i) const { return *Edge<T>::_to.at(i);  }
        T & to(const int i) { return *Edge<T>::_to.at(i);  }
    };

}

#endif // MULTI_FANOUT_EDGE_H
