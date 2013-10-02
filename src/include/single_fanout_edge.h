#ifndef SINGLE_FANOUT_EDGE_H
#define SINGLE_FANOUT_EDGE_H

#include "edge.h"

namespace Timing_Analysis {

    template<class T>
	/** @brief Inherits from Edge. A single fanout edge is an edge which connects one vertex to only one other
	*
	*/
    class Single_Fanout_Edge : public Edge<T>
    {
    public:
	/** @brief Single_Fanout_Edge constructor
	*
	*/
        Single_Fanout_Edge(T * from, T * to) : Edge<T>(from) {
            Edge<T>::set_fanout(0, to);
        }
	/** @brief Returns Edge<T>
	*
	* @return T &
	*/
        T & to(void) const { return *Edge<T>::_to.at(0);  }

    };

}

#endif // SINGLE_FANOUT_EDGE_H
