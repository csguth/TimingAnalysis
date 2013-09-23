#ifndef EDGE_H
#define EDGE_H

#include <vector>
using std::vector;

#include <cassert>

#include <cstdlib>

namespace Timing_Analysis {

    template<typename T>
	/** @brief Template class which represents an edge of the graph model. An edge in graph theory is a connection between vertices.
	*
	*/
    class Edge {

    protected:
	/** @brief Edge constructor
	*
	*@param T * from
	*
	*/
        Edge(T * from) : _from(from)
        {

        }

        T * _from;
        vector<T *> _to;

	/** @brief Inserts T pointer at index i
	*
	*@param const int i, T *tp
	*
	* @return void
	*/
        void set_fanout(const int i, T *tp)
        {
            if(_to.empty() && !i)
                _to.push_back(tp);
            else
                _to[i] = tp;
            assert(_to.size() == 1);
        }

	/** @brief Inserts T pointer
	*
	*@param T *tp
	*
	* @return void
	*/
        void add_fanout(T *tp)
        {
            _to.push_back(tp);
        }


    public:

	/** @brief Returns _from*******
	*
	* @return T *
	*/
        T * from() const
        {
            return _from;
        }

	/** @brief Returns fan out
	*
	* @return size_t
	*/
        size_t fanouts_size() const
        {
            return _to.size();
        }

    };
}

#endif // EDGE_H
