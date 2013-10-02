#ifndef TIMING_ARC_H
#define TIMING_ARC_H

#include "single_fanout_edge.h"
#include "transitions.h"
#include "timing_point.h"

namespace Timing_Analysis
{

    class Timing_Point;
	/** @brief Describes a timing arc of the timing graph model. Vertex which connects one timing point to another timing point only
	*
	*/
    class Timing_Arc : public Single_Fanout_Edge<Timing_Point>
    {
//        friend class Timing_Analysis;
        Transitions<double> _delay;
        Transitions<double> _slew;
        int _arc_number;
        int _gate_number;

    public:
		/** @brief Timing_Arc constructor
		*
		* @param Timing_Point * from, Timing_Point * to, const int arcNumber, const int gate_number) : Single_Fanout_Edge<Timing_Point>(from, to), _delay(0.0f, 0.0f), _slew(0.0f, 0.0f), _arc_number 
        * (arcNumber), _gate_number(gate_number)
		*/
        Timing_Arc(Timing_Point * from, Timing_Point * to, const int arcNumber, const int gate_number) : Single_Fanout_Edge<Timing_Point>(from, to), _delay(0.0f, 0.0f), _slew(0.0f, 0.0f), _arc_number(arcNumber), _gate_number(gate_number) {}
		/** @brief Timing_Arc destructor
		*
		*/
        virtual ~Timing_Arc(){}

		/** @brief Sets delay and slew time to zero
		*
		*@return void
		*/
        void clear();


        // GETTERS
		/** @brief Returns delay time
		*
		*@return Transitions<double>
		*/
        Transitions<double> delay() const { return _delay; }
		/** @brief Returns slew time
		*
		*@return Transitions<double>
		*/
        Transitions<double> slew() const { return _slew; }

		/** @brief Sets delay time
		*
		*@return void
		*/
        void delay(const Transitions<double> & delay) { _delay = delay; }
		/** @brief Sets slew time
		*
		*@return void
		*/
        void slew(const Transitions<double> & slew) { _slew = slew; }

		/** @brief Returns arc number
		*
		*@return int
		*/
        int arc_number() const { return _arc_number; }
		/** @brief Returns gate number
		*
		*@return void
		*/
        int gate_number() const { return _gate_number; }

		/** @brief Inserts formatted description of timing arc, including the origin vertex and destiny vertex
		*
		*/
        friend ostream & operator<<(ostream & out, const Timing_Arc & ta);

    };

}

#endif // TIMING_ARC_H
