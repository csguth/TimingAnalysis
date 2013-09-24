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
/** @brief Configuration class where the constraints of the case in study are set
*
*/
class Design_Constraints
{
    double _clock;
    map<string, string> _driving_cells;
    map<string, Transitions<double> > _input_transitions;
    map<string, Transitions<double> > _input_delays;
    map<string, Transitions<double> > _output_delays;
    map<string, double> _output_loads;
public:
	/** @brief Sets clock period. frequency = 1 / clock_period
	*
	* @return void
	*/
    void clock(const double clock);
	/** @brief Tries to set input delay. Returns true if succeeded
	*
	*@param const string input_name, const Transitions<double> delay
	*
	* @return bool
	*/
    bool input_delay(const string input_name, const Transitions<double> delay);
	/** @brief Tries to set output delay. Returns true if succeeded
	*
	*@param const string input_name, const Transitions<double> delay
	*
	* @return bool
	*/
    bool output_delay(const string output_name, const Transitions<double> delay);
	/** @brief Tries to set output load. Returns true if succeeded
	*
	*@param const string input_name, const Transitions<double> output_load
	*
	* @return bool
	*/
    bool output_load(const string output_name, const double output_load);
	/** @brief Tries to set driving cell. Returns true if succeeded
	*
	*@param const string input_name, const string driving_cell
	*
	* @return bool
	*/
    bool driving_cell(const string input_name, const string driving_cell);
	/** @brief Tries to set input transitions. Returns true if succeeded
	*
	*@param const string input_name, const Transitions<double> transition
	*
	* @return bool
	*/
    bool input_transition(const string input_name, const Transitions<double> transition);


	/** @brief Returns clock period
	*
	*
	* @return double
	*/
    double clock() const;
	/** @brief Returns Transitions<double> input_delay with input_name name
	*
	*@param const string input_name
	*
	* @return const Transitions<double>
	*/
    const Transitions<double> input_delay(const string input_name) const;
	/** @brief Returns Transitions<double> output_delay with output_name name
	*
	*@param const string output_name
	*
	* @return const Transitions<double>
	*/
    const Transitions<double> output_delay(const string output_name) const;
	/** @brief Returns output load with output_name name
	*
	*@param const string output_name
	*
	* @return double
	*/
    double output_load(const string output_name) const;
	/** @brief Returns output loads size
	*
	* @return size_t
	*/
    size_t output_loads_size() const;
	/** @brief Returns driving cell with input_name name
	*
	*@param const string input_name
	*
	* @return const string
	*/
    const string driving_cell(const string input_name) const;
	/** @brief Returns Transition<double> input_transition with input_name name
	*
	*@param const string input_name
	*
	* @return const Transition<double>
	*/
    const Transitions<double> input_transition(const string input_name) const;
	/* data */
};

#endif
