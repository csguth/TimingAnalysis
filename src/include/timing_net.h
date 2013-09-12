#ifndef TIMING_NET_H
#define TIMING_NET_H

#include "wire_delay_model.h"
#include "multi_fanout_edge.h"
#include "timing_point.h"

namespace Timing_Analysis {

    class Timing_Point;
    class Timing_Net : public Multi_Fanout_Edge<Timing_Point>
    {
        friend class Timing_Analysis;
        friend class Timing_Point;
        string _name;
        WireDelayModel * _wire_delay_model;

    public:
        Timing_Net(const string & name, Timing_Point * from, WireDelayModel * wire_delay_model)
        :Multi_Fanout_Edge<Timing_Point>(from), _name(name),_wire_delay_model(wire_delay_model)
        {
        }
        virtual ~Timing_Net(){}

        const string name() const;
        friend ostream & operator<<(ostream & out, const Timing_Net & tn);



       WireDelayModel * wire_delay_model() {return _wire_delay_model;}
    };
}

#endif // TIMING_NET_H
