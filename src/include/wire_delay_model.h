#ifndef WIREDELAYMODEL_H_
#define WIREDELAYMODEL_H_

#include <iostream>
using std::cout;
using std::endl;

#include "spef_net.h"
#include "liberty_library.h"
#include "configuration.h"

#include <cassert>

class WireDelayModel
{
protected:
    double _lumped_capacitance;
	static LinearLibertyLookupTableInterpolator interpolator;

public:
    WireDelayModel(const double & lumped_capacitance) : _lumped_capacitance(lumped_capacitance){}
    virtual ~WireDelayModel(){}
    virtual const Transitions<double> simulate(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew, bool is_input_driver) = 0;
    virtual const Transitions<double> delay_at_fanout_node(const string fanout_node_name) const = 0;
    virtual const Transitions<double> slew_at_fanout_node(const string fanout_node_name) const = 0;
	virtual void setFanoutPinCapacitance(const string fanoutNameAndPin, const double pinCapacitance) = 0;


    virtual Transitions<double> root_delay(int arc_number) = 0;
    virtual Transitions<double> root_slew(int arc_number) = 0;
    virtual void clear() = 0;


    double lumped_capacitance() const;
};

class LumpedCapacitanceWireDelayModel : public WireDelayModel
{
    Transitions<double> _delay;
    Transitions<double> _slew;
public:
    LumpedCapacitanceWireDelayModel(const SpefNet & descriptor, const string root_node, const bool dummy_edge = false) : WireDelayModel(descriptor.netLumpedCap){	}
    const Transitions<double> simulate(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> _slew, bool is_input_driver);
    const Transitions<double> delay_at_fanout_node(const string fanout_node_name) const;
    const Transitions<double> slew_at_fanout_node(const string fanout_node_name) const;
    void setFanoutPinCapacitance(const string fanout_name_and_pin, const double pinCapacitance) { _lumped_capacitance += pinCapacitance; }


    Transitions<double> root_delay(int arc_number);
    Transitions<double> root_slew(int arc_number);
    void clear();


};

class RC_Tree_Wire_Delay_Model : public WireDelayModel
{
protected:

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
		Node() :
            parent(-1), nodeCapacitance(numeric_limits<Transitions<double> >::zero()), totalCapacitance(numeric_limits<Transitions<double> >::zero()), effectiveCapacitance(numeric_limits<Transitions<double> >::zero()), resistance(numeric_limits<Transitions<double> >::zero()), slew(numeric_limits<Transitions<double> >::zero()), delay(numeric_limits<Transitions<double> >::zero()), sink(false)
		{
		}
		;
	};

    vector<Node> _nodes;

	struct NodeAndResistor
	{
		int nodeIndex;
		int resistorIndex;
		NodeAndResistor(const int & node, const int & resistor) :
				nodeIndex(node), resistorIndex(resistor)
		{
		}
		;
	};

    vector<string> _nodes_names;
    vector<vector<Transitions<double> > >_slews;
    vector<vector<Transitions<double> > >_delays;

    vector<Transitions<double> > _max_delays;
    vector<Transitions<double> > _max_slews;
    map<std::string, int> _fanout_name_to_node_number;


    void IBM_update_downstream_capacitances();
    void IBM_initialize_effective_capacitances();
    void IBM_update_slews(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew, bool is_input_driver);
    void IBM_update_effective_capacitances();

protected:
    const Transitions<double> run_IBM_algorithm(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew, bool is_input_driver);
public:
    RC_Tree_Wire_Delay_Model(const SpefNetISPD2013 & descriptor, const string rootNode, const size_t arcs_size, const bool dummyEdge = false);
    virtual const Transitions<double> simulate(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew, bool is_input_driver) = 0;
    virtual const Transitions<double> delay_at_fanout_node(const string fanout_node_name) const = 0;
    virtual const Transitions<double> slew_at_fanout_node(const string fanout_node_name) const = 0;
	void setFanoutPinCapacitance(const string fanoutNameAndPin, const double pinCapacitance);


    Transitions<double> root_delay(int arc_number);
    Transitions<double> root_slew(int arc_number);
    void clear();
};



class Full: public RC_Tree_Wire_Delay_Model
{


public:
    Full(const SpefNetISPD2013 & descriptor, const string rootNode, const size_t arcs_size, const bool dummyEdge = false)
        : RC_Tree_Wire_Delay_Model(descriptor, rootNode, arcs_size, dummyEdge)
    {

    }

    const Transitions<double> simulate(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew, bool is_input_driver);
    const Transitions<double> delay_at_fanout_node(const string fanout_node_name) const;
    const Transitions<double> slew_at_fanout_node(const string fanout_node_name) const;

};

class Reduced_Pi : public RC_Tree_Wire_Delay_Model
{


    double _c1;
    double _r;
    double _c2;


    Transitions<double> _slew_fanout;
    Transitions<double> _delay_fanout;

    void reduce_to_pi_model(double & c_near, double & r, double & c_far);

public:
    Reduced_Pi(const SpefNetISPD2013 & descriptor, const string rootNode, const size_t arcs_size, const bool dummyEdge = false)
        : RC_Tree_Wire_Delay_Model(descriptor, rootNode, arcs_size, dummyEdge)
    {
        cout << rootNode << endl;
    }

    const Transitions<double> simulate(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew, bool is_input_driver);
    const Transitions<double> delay_at_fanout_node(const string fanout_node_name) const;
    const Transitions<double> slew_at_fanout_node(const string fanout_node_name) const;
};


#endif
