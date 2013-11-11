#ifndef TIMING_POINT_H
#define TIMING_POINT_H

#include "timing_net.h"
#include "timing_arc.h"
#include "timing_analysis.h"

namespace Timing_Analysis {

    enum Timing_Point_Type
    {
        INPUT, OUTPUT, PI_INPUT, REGISTER_INPUT, PI, PO
    };

    class Timing_Arc;
    class Timing_Net;
    class Timing_Point
    {
//        friend class Timing_Analysis;
        friend class Timing_Net;
        string _name;
        Timing_Net * _net;
        Timing_Arc * _arc;
        Transitions<double> _slack;
        Transitions<double> _slew;
        Transitions<double> _arrival_time;
        size_t _gate_number;
        Timing_Point_Type _type;

        Transitions<double> _ceff;
        int _logic_level;




    public:
        Timing_Point(string name, const size_t gate_number, Timing_Point_Type type);
        virtual ~Timing_Point(){}

        // GETTERS
        double load() const;
        Transitions<double> ceff() const;
        const string name() const { return _name; }
        int gate_number() const  { return _gate_number; }

        const Transitions<double> slack() const { return _slack; }
        const Transitions<double> slew() const { return _slew; }
        const Transitions<double> arrival_time() const { return _arrival_time; }
        const Transitions<double> required_time() const {return _slack + _arrival_time;}
        Timing_Net & net() const { return *_net; }
        Timing_Arc & arc () const { return *_arc; }
        int logic_level() const { return _logic_level; }

        void ceff(const Transitions<double> & ceff) { _ceff = ceff; }
        void slack(const Transitions<double> & slack ) { _slack = slack; }
        void slew(const Transitions<double> & slew ) { _slew = slew; }
        void arrival_time(const Transitions<double> & arrival_time ) { _arrival_time = arrival_time; }
        void net(Timing_Net * net) { _net = net; }
        void arc(Timing_Arc * arc) { _arc = arc; }
        void logic_level(int level) { _logic_level = level; }


        // TIMING ANALYSIS
        const Transitions<double> update_slack(const Transitions<double> required_time);
        void clear_timing_info();

        bool is_PO() const { return _type == PO; }
        bool is_PI() const { return _type == PI; }
        bool is_input_pin() const { return _type == INPUT; }
        bool is_output_pin() const { return _type == OUTPUT; }
        bool is_PI_input() const { return _type == PI_INPUT; }
        bool is_reg_input() const { return _type == REGISTER_INPUT; }

        friend ostream & operator<<(ostream & out, const Timing_Point & tp);
    };

}

#endif // TIMING_POINT_H
