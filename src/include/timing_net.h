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
	/** @brief Timing_Net constructor
	*
	* @param const string & name, Timing_Point * from, WireDelayModel * wire_delay_model
	*/
        Timing_Net(const string & name, Timing_Point * from, WireDelayModel * wire_delay_model)
        :Multi_Fanout_Edge<Timing_Point>(from), _name(name),_wire_delay_model(wire_delay_model)
        {
        }
	/** @brief Empty Timing_Net destructor
	*
	*/
        virtual ~Timing_Net(){}

	/** @brief Returns Timing_Net name
	*
	* @return const string
	*/
        const string name() const;
	/** @brief Redefinition of << operator. Inserts formatted description including Timing_Net name
	*
	*/
        friend ostream & operator<<(ostream & out, const Timing_Net & tn);
	/** @brief Returns pointer to WireDelayModel
	*
	* @return WireDelayModel *
	*/
        WireDelayModel * wire_delay_model() {return _wire_delay_model;}
    };
}

#endif // TIMING_NET_H
