#ifndef MULTI_FANOUT_EDGE_H
#define MULTI_FANOUT_EDGE_H
#include "edge.h"
#include "timing_point.h"

namespace Timing_Analysis
{

    template <class T>
	/** @brief Inherits from Edge. An multi fanout edge is a group of edges which connect a vertex to more than one vertexes
	*
	*/
    class Multi_Fanout_Edge : public Edge<T>
    {
    public:
	/** @brief Multi_Fanout_Edge constructor
	*
	*/
        Multi_Fanout_Edge(T * from) : Edge<T>(from) {}
	/** @brief Returns Edge<T> at index i
	*
	*@param const int i
	*
	* @return T &
	*/
        T & to(const int i) const { return *Edge<T>::_to.at(i);  }
    };

}

#endif // MULTI_FANOUT_EDGE_H
