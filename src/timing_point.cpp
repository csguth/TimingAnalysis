#include "include/timing_point.h"

namespace Timing_Analysis {

Timing_Point::Timing_Point(std::string name, const size_t gate_number, Timing_Point_Type type): _name(name), _net(0), _arc(0), _slack(0.0f, 0.0f), _slew(0.0f, 0.0f), _arrival_time(0.0f, 0.0f), _gate_number(gate_number), _type(type), _logic_level(0)
    {
        if(type == REGISTER_INPUT)
            _slew = Transitions<double>(80.0f, 80.0f);
    }

    double Timing_Point::load() const
    {
        return _net->_wire_delay_model->lumped_capacitance();
    }

    Transitions<double> Timing_Point::ceff() const
    {
        return _ceff;
    }

    const Transitions<double> Timing_Point::update_slack(const Transitions<double> required_time)
    {
        _slack = required_time - _arrival_time;
        return _slack;
    }

    void Timing_Point::clear_timing_info()
    {
        _slack =  numeric_limits<Transitions<double> >::zero();
        _slew = numeric_limits<Transitions<double> >::zero();
        _arrival_time = numeric_limits<Transitions<double> >::zero();
    }

    std::ostream & operator<<(std::ostream &out, const Timing_Point &tp)
    {
        return out << tp._name << " slack " << tp._slack << " slew " << tp._slew << " arrival " << tp._arrival_time;
    }



}
