#ifndef WIREDELAYMODEL_H_
#define WIREDELAYMODEL_H_

#include <iostream>
using std::cout;
using std::endl;

#include "spef_net.h"
#include "liberty_library.h"
#include "configuration.h"

#include <cassert>

/** @brief Describes parasitc elements present in wires in a circuit which cause significant delay and are taken into account in static timing analysis
*/
class WireDelayModel
{
protected:

    double _lumped_capacitance;
    static LinearLibertyLookupTableInterpolator interpolator;

public:
	/** @brief WireDelayModel default constructor
	*
	* @param const double & lumped_capacitance
	*/
    WireDelayModel(const double & lumped_capacitance) : _lumped_capacitance(lumped_capacitance){}
	/** @brief Empty WideDelayModel destructor
	*/
    virtual ~WireDelayModel(){}
	/** @brief 
	*
	* @param const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew, bool is_input_driver
	*
	* @return Transitions<double>
	*/
    virtual const Transitions<double> simulate(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew, bool is_input_driver) = 0;
	/** @brief Returns Transitions with rise and fall delay time values set to zero
	*
	* @param const string fanout_node_name
	*
	* @return Transitions<double>
	*/
    virtual const Transitions<double> delay_at_fanout_node(const string fanout_node_name) const = 0;
	/** @brief Returns Transitions with rise and fall slew time values set to zero
	*
	* @param const string fanout_node_name
	*
	* @return Transitions<double>
	*/
    virtual const Transitions<double> slew_at_fanout_node(const string fanout_node_name) const = 0;
	/** @brief Increments lumped capacitance of the wire
	*
	* @param const string fanoutNameAndPin, const double capacitance
	*
	* @return void
	*/
    virtual void setFanoutPinCapacitance(const string fanoutNameAndPin, const double pinCapacitance) = 0;

	/** @brief Virtual. Must be implemented by child class. Returns Transitions<double> representing delay of the wire
	*
	* @param int arc_number
	*
	* @return Transitions<double>
	*/
    virtual Transitions<double> root_delay(int arc_number) = 0;
	/** @brief Virtual. Must be implemented by child class. Returns Transitions<double> representing delay of the wire
	*
	* @param int arc_number
	*
	* @return Transitions<double>
	*/
    virtual Transitions<double> root_slew(int arc_number) = 0;
	/** @brief Virtual. Must be implemented by child class. Clears attributes
	*
	* @return void
	*/
    virtual void clear() = 0;

	/** @brief Returns lumped capacitance of the wire
	*
	* @return double
	*/
    double lumped_capacitance() const;
};

/** @brief This model describes the parasitic capacitances distributed along a wire as a single circuit element. Does not take resistors into account
*/
class LumpedCapacitanceWireDelayModel : public WireDelayModel
{
    Transitions<double> _delay;
    Transitions<double> _slew;
public:
	/** @brief LumpedCapacitanceWireDelayModel default constructor
	*
	*@param const SpefNet & descriptor, const string root_node, const bool dummy_edge(default false)
	*/
    LumpedCapacitanceWireDelayModel(const SpefNet & descriptor, const string root_node, const bool dummy_edge = false) : WireDelayModel(descriptor.netLumpedCap){}
	/** @brief Updates delay and slew times and returns Transitions with lumped capacitance values
	*
	* @param const LibertyCellInfo & cellInfo, const int input, const Transitions<double> _slew, bool is_input_driver
	*
	* @return const Transitions<double>
	*/
    const Transitions<double> simulate(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> _slew, bool is_input_driver);
	/** @brief Returns Transitions with rise and fall delay time values set to zero
	*
	* @param const string fanout_node_name
	*
	* @return const Transitions<double>
	*/
    const Transitions<double> delay_at_fanout_node(const string fanout_node_name) const;
	/** @brief Returns Transitions with rise and fall slew time values set to zero
	*
	* @param const string fanout_node_name
	*
	* @return const Transitions<double>
	*/
    const Transitions<double> slew_at_fanout_node(const string fanout_node_name) const;
	/** @brief Increments total lumped capacitance with pinCapacitance
	*
	* @param const string fanout_name_and_pin, const double pinCapacitance
	*
	* @return void
	*/
    void setFanoutPinCapacitance(const string fanout_name_and_pin, const double pinCapacitance) { _lumped_capacitance += pinCapacitance; }

	/** @brief Returns delay time value
	*
	* @param int arc_number
	*
	* @return Transitions<double>
	*/
    Transitions<double> root_delay(int arc_number);
	/** @brief Returns slew time value
	*
	* @param int arc_number
	*
	* @return Transitions<double>
	*/
    Transitions<double> root_slew(int arc_number);
	/** @brief Not implemented. Included for completeness
	*
	* @return void
	*/
    void clear();


};

/** @brief This model describes the parasitic elements(capacitors and resistors) distributed along a wire as a single circuit element
*/
class RCTreeWireDelayModel : public WireDelayModel
{
	/** @brief Represents a node in the RC Tree wire delay model
	*/
	struct Node
	{
		int parent;
        Transitions<double> nodeCapacitance;
        Transitions<double> totalCapacitance;
		Transitions<double> effectiveCapacitance;
        Transitions<double> resistance;
		Transitions<double> slew;
		Transitions<double> delay;
		bool sink;
		static vector<string> nodesNames;
		/** @brief Node default constructor
		*
		*/
		Node() :
            parent(-1), nodeCapacitance(numeric_limits<Transitions<double> >::zero()), totalCapacitance(numeric_limits<Transitions<double> >::zero()), effectiveCapacitance(numeric_limits<Transitions<double> >::zero()), resistance(numeric_limits<Transitions<double> >::zero()), slew(numeric_limits<Transitions<double> >::zero()), delay(numeric_limits<Transitions<double> >::zero()), sink(false)
		{
		}
		;
	};
	/** @brief Struct which represents a node and resistor association
	*/
	struct NodeAndResistor
	{
		int nodeIndex;
		int resistorIndex;
		/** @brief NodeAndResistor default constructor
		*
		* @param const int & node, const int & resistor
		*/
		NodeAndResistor(const int & node, const int & resistor) :
				nodeIndex(node), resistorIndex(resistor)
		{
		}
		;
	};

	/** @brief Updates slew and delay time values
	*
	* @param const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew, bool is_input_driver
	*
	* @return void
	*/
    void updateSlews(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew, bool is_input_driver);
	/** @brief Updates effective capacitance value
	*
	* @return void
	*/
	void updateEffectiveCapacitances();
	/** @brief *********************
	*
	* @return void
	*/
	void updateDownstreamCapacitances();
	/** @brief Sets the effective capacitance of each node as its total capacitance
	*
	* @return void
	*/
	void initializeEffectiveCapacitances();

	vector<Node> nodes;
	vector<string> nodesNames;
    vector<vector<Transitions<double> > >_slews;
    vector<vector<Transitions<double> > >_delays;


    vector<Transitions<double> > _max_delays;
    vector<Transitions<double> > _max_slews;
	map<std::string, int> fanoutNameToNodeNumber;


public:
	/** @brief RCtreeWireDelayModel default constructor 
	*
	* @param const SpefNetISPD2013 & descriptor, const string rootNode, const size_t arcs_size, const bool dummyEdge(false default)
	*/
    RCTreeWireDelayModel(const SpefNetISPD2013 & descriptor, const string rootNode, const size_t arcs_size, const bool dummyEdge = false);
	/** @brief Calculates and returns Transitions<double> representing effective capacitance of the wire
	*
	* @param const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew, bool is_input_driver
	*
	* @return const Transitions<double>
	*/
    const Transitions<double> simulate(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew, bool is_input_driver);
	/** @brief Returns maximum delay time value at node fanout_node_name
	*
	* @param const string fanout_node_name
	*
	* @return const Transitions<double>
	*/
    const Transitions<double> delay_at_fanout_node(const string fanout_node_name) const;
	/** @brief Returns maximum slew time value at node fanout_node_name
	*
	* @param const string fanout_node_name
	*
	* @return const Transitions<double>
	*/
    const Transitions<double> slew_at_fanout_node(const string fanout_node_name) const;
	/** @brief Increments effective capacitance of fanoutNameAndPin by pinCapacitance
	*
	* @param const string fanoutNameAndPin, const double pinCapacitance
	*
	* @return void
	*/
	void setFanoutPinCapacitance(const string fanoutNameAndPin, const double pinCapacitance);

	/** @brief Returns Transitions<double> representing delay time value at arc_number
	*
	* @param int arc_number
	*
	* @return Transitions<double>
	*/
    Transitions<double> root_delay(int arc_number);
	/** @brief Returns Transitions<double> representing slew time value at arc_number
	*
	* @param int arc_number
	*
	* @return Transitions<double>
	*/
    Transitions<double> root_slew(int arc_number);
	/** @brief Sets slew and delay time values to zero
	*
	* @return void
	*/
    void clear();
};


#endif
