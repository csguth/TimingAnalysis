#ifndef TIMING_ARC_H
#define TIMING_ARC_H

#include "single_fanout_edge.h"
#include "transitions.h"
#include "timing_point.h"

namespace Timing_Analysis
{

    class Timing_Point;
    class Timing_Arc : public Single_Fanout_Edge<Timing_Point>
    {
//        friend class Timing_Analysis;
        Transitions<double> _delay;
        Transitions<double> _slew;
        int _arc_number;
        int _gate_number;

    public:
        Timing_Arc(Timing_Point * from, Timing_Point * to, const int arcNumber, const int gate_number) : Single_Fanout_Edge<Timing_Point>(from, to), _delay(0.0f, 0.0f), _slew(0.0f, 0.0f), _arc_number(arcNumber), _gate_number(gate_number) {}
        virtual ~Timing_Arc(){}


        // GETTERS
        Transitions<double> delay() const { return _delay; }
        Transitions<double> slew() const { return _slew; }

        void delay(const Transitions<double> & delay) { _delay = delay; }
        void slew(const Transitions<double> & slew) { _slew = slew; }

        int arc_number() const { return _arc_number; }
        int gate_number() const { return _gate_number; }


        friend ostream & operator<<(ostream & out, const Timing_Arc & ta);

    };

}

#endif // TIMING_ARC_H
