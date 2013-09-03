#ifndef EDGE_H
#define EDGE_H

#include <vector>
using std::vector;

#include <cassert>

#include <cstdlib>

namespace Timing_Analysis {

    template<typename T>
    class Edge {

    protected:
        Edge(T * from) : _from(from)
        {

        }

        T * _from;
        vector<T *> _to;

        void set_fanout(const int i, T *tp)
        {
            if(_to.empty() && !i)
                _to.push_back(tp);
            else
                _to[i] = tp;
            assert(_to.size() == 1);
        }

        void add_fanout(T *tp)
        {
            _to.push_back(tp);
        }


    public:
        T * from() const
        {
            return _from;
        }

        size_t fanouts_size() const
        {
            return _to.size();
        }

    };

}

#endif // EDGE_H
