#ifndef SINGLE_FANOUT_EDGE_H
#define SINGLE_FANOUT_EDGE_H

#include "edge.h"

namespace Timing_Analysis {

    template<class T>
    class Single_Fanout_Edge : public Edge<T>
    {
    public:
        Single_Fanout_Edge(T * from, T * to) : Edge<T>(from) {
            Edge<T>::set_fanout(0, to);
        }
        T & to(void) const { return *Edge<T>::_to.at(0);  }

    };

}

#endif // SINGLE_FANOUT_EDGE_H
