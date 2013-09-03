#ifndef DESIGNCONSTRAINTS_H_
#define DESIGNCONSTRAINTS_H_

#include <string>
using std::string;

#include "transitions.h"

#include <map>
using std::map;

#include <utility>
using std::make_pair;

#include <iostream>
using std::endl;
using std::cout;

class Design_Constraints
{
    double _clock;
    map<string, string> _driving_cells;
    map<string, Transitions<double> > _input_transitions;
    map<string, Transitions<double> > _input_delays;
    map<string, Transitions<double> > _output_delays;
    map<string, double> _output_loads;
public:

    void clock(const double clock);
    bool input_delay(const string input_name, const Transitions<double> delay);
    bool output_delay(const string output_name, const Transitions<double> delay);
    bool output_load(const string output_name, const double output_load);
    bool driving_cell(const string input_name, const string driving_cell);
    bool input_transition(const string input_name, const Transitions<double> transition);

    double clock() const;
    const Transitions<double> input_delay(const string input_name) const;
    const Transitions<double> output_delay(const string output_name) const;
    double output_load(const string output_name) const;
    size_t output_loads_size() const;
    const string driving_cell(const string input_name) const;
    const Transitions<double> input_transition(const string input_name) const;
	/* data */
};

#endif
