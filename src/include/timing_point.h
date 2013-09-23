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

    public:
	/** @brief Timing_Point default constructor
	*
	* @param string name, const size_t gate_number, Timing_Point_Type type
	*
	*/
        Timing_Point(string name, const size_t gate_number, Timing_Point_Type type);
	/** @brief Empty Timing_Point destructor
	*
	*/
        virtual ~Timing_Point(){}

        // GETTERS

	/** @brief Returns  **********
	*
	* @return double
	*/
        double load() const;
        Transitions<double> ceff() const;
	/** @brief Returns Timing_Net name
	*
	* @return const string
	*/
        const string name() const { return _name; }
	/** @brief Returns Timing_Net gate number
	*
	* @return int
	*/
        int gate_number() const  { return _gate_number; }

	/** @brief Returns Timing_Point slack time
	*
	* @return const Transitions<double>
	*/
        const Transitions<double> slack() const { return _slack; }
	/** @brief Returns Timing_Point slew time
	*
	* @return const Transitions<double>
	*/
        const Transitions<double> slew() const { return _slew; }
	/** @brief Returns Timing_Point arrical time
	*
	* @return const Transitions<double>
	*/
        const Transitions<double> arrival_time() const { return _arrival_time; }
	/** @brief Returns Timing_Point required time
	*
	* @return const Transitions<double>
	*/
        const Transitions<double> required_time() const {return _slack + _arrival_time;}
	/** @brief Returns reference to Timing_Net net
	*
	* @return Timing_Net &
	*/
        Timing_Net & net() { return *_net; }
	/** @brief Returns reference to Timing_Arc arc
	*
	* @return Timing_Arc &
	*/
        Timing_Arc & arc () const { return *_arc; }

	/** @brief Sets the effective capacitance of the Timing_Net
	*
	* @param Transitions<double> & ceff
	*/
        void ceff(Transitions<double> & ceff) { _ceff = ceff; }
	/** @brief Sets the slack time of the Timing_Net
	*
	* @param Transitions<double> & slack
	*/
        void slack(const Transitions<double> & slack ) { _slack = slack; }
	/** @brief Sets the slew time of the Timing_Net
	*
	* @param Transitions<double> & slew
	*/
        void slew(const Transitions<double> & slew ) { _slew = slew; }
	/** @brief Sets the arrival time of the Timing_Net
	*
	* @param Transitions<double> & arrival_time
	*/
        void arrival_time(const Transitions<double> & arrival_time ) { _arrival_time = arrival_time; }
	/** @brief Sets pointer to Timing_Net
	*
	* @param Timing_Net * net
	*/
        void net(Timing_Net * net) { _net = net; }
	/** @brief Sets pointer to Timing_Arc
	*
	* @param Timing_Arc * arc
	*/
        void arc(Timing_Arc * arc) { _arc = arc; }
        // TIMING ANALYSIS
	/** @brief Returns updated slack. Slack = required_time - arrival_time
	*
	* @param const Transitions<double> required_time
	*
	* @return const Transitions<double>
	*/
        const Transitions<double> update_slack(const Transitions<double> required_time);
	/** @brief Slack, slew and arrival_time parameters are set to zero
	*
	* @return void
	*/
        void clear_timing_info();

	/** @brief Returns true if caller is a primary ouput
	*
	* @return bool
	*/
        bool is_PO() const { return _type == PO; }
	/** @brief Returns true if caller is a primary input
	*
	* @return bool
	*/
        bool is_PI() const { return _type == PI; }
	/** @brief Returns true if caller is a input pin
	*
	* @return bool
	*/
        bool is_input_pin() const { return _type == INPUT; }
	/** @brief Returns true if caller is a ouput pin
	*
	* @return bool
	*/
        bool is_output_pin() const { return _type == OUTPUT; }
	/** @brief Returns true if caller is a primary_input input
	*
	* @return bool
	*/
        bool is_PI_input() const { return _type == PI_INPUT; }
	/** @brief Returns true if caller is a register input
	*
	* @return bool
	*/
        bool is_reg_input() const { return _type == REGISTER_INPUT; }
	/** @brief Redefinition of << operator. Inserts formatted description including arrival, required and slack times
	*
	*/
        friend ostream & operator<<(ostream & out, const Timing_Point & tp);
    };

}

#endif // TIMING_POINT_H
