#include "include/design_constraints.h"

// private
void Design_Constraints::clock(const double clock)
{
    _clock = clock;
}
bool Design_Constraints::input_delay(const string input_name, const Transitions<double> delay)
{
    if(_input_delays.find(input_name) != _input_delays.end())
		return false;
    _input_delays.insert(make_pair(input_name, delay));
	return true;
}
bool Design_Constraints::output_delay(const string output_name, const Transitions<double> delay)
{
    if(_output_delays.find(output_name) != _output_delays.end())
		return false;
    _output_delays.insert(make_pair(output_name, delay));
	return true;
}
bool Design_Constraints::output_load(const string output_name, const double output_load)
{
    if(_output_loads.find(output_name) != _output_loads.end())
		return false;
    _output_loads.insert(make_pair(output_name, output_load));
	return true;
}
bool Design_Constraints::driving_cell(const string input_name, const string driving_cell)
{
    if(_driving_cells.find(input_name) != _driving_cells.end())
		return false;
    _driving_cells.insert(make_pair(input_name, driving_cell));
	return true;
}

bool Design_Constraints::input_transition(const string input_name, const Transitions<double> transition)
{
    if(_input_transitions.find(input_name) != _input_transitions.end())
		return false;
    _input_transitions.insert(make_pair(input_name, transition));
	return true;
}


// public
double Design_Constraints::clock() const
{
    return _clock;
}
const Transitions<double> Design_Constraints::input_delay(const string input_name) const
{
    return _input_delays.at(input_name);
}
const Transitions<double> Design_Constraints::output_delay(const string output_name) const
{
    return _output_delays.at(output_name);
}
double Design_Constraints::output_load(const string output_name) const
{
    return _output_loads.at(output_name);
}

size_t Design_Constraints::output_loads_size() const
{
    return _output_loads.size();
}
const string Design_Constraints::driving_cell(const string input_name) const
{
    return _driving_cells.at(input_name);
}

const Transitions<double> Design_Constraints::input_transition(const string input_name) const
{
    return _input_transitions.at(input_name);
}
