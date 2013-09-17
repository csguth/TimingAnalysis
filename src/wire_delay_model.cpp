#include "include/wire_delay_model.h"
LinearLibertyLookupTableInterpolator WireDelayModel::interpolator;

const Transitions<double> LumpedCapacitanceWireDelayModel::simulate(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew, bool is_input_driver)
{
    Unateness unateness = NEGATIVE_UNATE;
    if(cellInfo.isSequential)
        unateness = NON_UNATE;
    this->_slew = WireDelayModel::interpolator.interpolate(cellInfo.timingArcs.at(input).riseTransition, cellInfo.timingArcs.at(input).fallTransition, Transitions<double>(_lumped_capacitance, _lumped_capacitance), slew, unateness);
    this->_delay = WireDelayModel::interpolator.interpolate(cellInfo.timingArcs.at(input).riseDelay, cellInfo.timingArcs.at(input).fallDelay, Transitions<double>(_lumped_capacitance, _lumped_capacitance), slew, unateness, is_input_driver);
    return Transitions<double>(_lumped_capacitance, _lumped_capacitance);
}

// Any fanout node has the same delay and slew
const Transitions<double> LumpedCapacitanceWireDelayModel::delay_at_fanout_node(const string fanout_node_name) const {
    return numeric_limits<Transitions<double> >::zero();
}
const Transitions<double> LumpedCapacitanceWireDelayModel::slew_at_fanout_node(const string fanout_node_name) const {
    return numeric_limits<Transitions<double> >::zero();
}

Transitions<double> LumpedCapacitanceWireDelayModel::root_delay(int arc_number)
{
    return _delay;
}

Transitions<double> LumpedCapacitanceWireDelayModel::root_slew(int arc_number)
{
    return _slew;
}

void LumpedCapacitanceWireDelayModel::clear()
{
    // NÃO FAZ NADA
}


RCTreeWireDelayModel::RCTreeWireDelayModel(const SpefNetISPD2013 & descriptor, const string rootNode, const size_t arcs_size, const bool dummyEdge) : WireDelayModel(descriptor.netLumpedCap), nodes(descriptor.nodesSize()), nodesNames(descriptor.nodesSize()), _slews(arcs_size, vector<Transitions<double> >(descriptor.nodesSize())),  _delays(arcs_size, vector<Transitions<double> >(descriptor.nodesSize())), _max_delays(descriptor.nodesSize()), _max_slews(descriptor.nodesSize())
{
	if (dummyEdge)
		return;

	// criar um vetor de fanouts com referência para os timing points de seus fanouts

	const int rootIndex = descriptor.getNodeIndex(rootNode);
	const SpefNetISPD2013::Node & root = descriptor.getNode(rootIndex);
	queue<NodeAndResistor> q;
	vector<bool> added(descriptor.resistorsSize(), false);

	for (unsigned i = 0; i < root.resistors.size(); i++)
	{
		const int resistorIndex = root.resistors[i];
		const SpefNetISPD2013::Resistor & resistor = descriptor.getResistor(resistorIndex);
		q.push(NodeAndResistor(resistor.getOtherNode(rootIndex), resistorIndex));
		added[resistorIndex] = true;
	}
	int neighbourhood;
	vector<int> topology(nodes.size(), -1);
	vector<int> reverseTopology(topology);

    nodes[0].nodeCapacitance.set(root.capacitance, root.capacitance);
	topology[0] = rootIndex;
	nodesNames[0] = rootNode;
	reverseTopology[rootIndex] = 0;
	int counter = 1;

	while (!q.empty())
	{
		const int & n = q.front().nodeIndex;
		const int & r = q.front().resistorIndex;
		q.pop();

		const SpefNetISPD2013::Node & nDescriptor = descriptor.getNode(n);
		const SpefNetISPD2013::Resistor & rDescriptor = descriptor.getResistor(r);

		nodesNames[counter] = nDescriptor.name;
		nodes[counter].parent = reverseTopology[rDescriptor.getOtherNode(nDescriptor.nodeIndex)];
        nodes[counter].nodeCapacitance.set(nDescriptor.capacitance, nDescriptor.capacitance);
        nodes[counter].resistance.set(rDescriptor.value, rDescriptor.value);

		topology[counter] = n;
		reverseTopology[n] = counter;

		neighbourhood = 0;
		for (unsigned i = 0; i < nDescriptor.resistors.size(); i++)
		{
			if (!added[nDescriptor.resistors[i]])
			{
				const SpefNetISPD2013::Resistor & resistor = descriptor.getResistor(nDescriptor.resistors[i]);
				q.push(NodeAndResistor(resistor.getOtherNode(nDescriptor.nodeIndex), nDescriptor.resistors[i]));
				added[nDescriptor.resistors[i]] = true;
				neighbourhood++;
			}
		}

		nodes[counter].sink = (neighbourhood == 0);
		if (nodes[counter].sink)
		{
			fanoutNameToNodeNumber[nDescriptor.name] = counter;
		}

		counter++;
	}

	updateDownstreamCapacitances();
	initializeEffectiveCapacitances();
}


const Transitions<double> RCTreeWireDelayModel::simulate(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew, bool is_input_driver)
{
    updateDownstreamCapacitances();
	initializeEffectiveCapacitances();
	//forwardIterate();
	Transitions<double> error;

    Transitions<double> current_source_slew, old_source_slew;
	int i = 0;

    nodes[0].slew = RCTreeWireDelayModel::interpolator.interpolate(cellInfo.timingArcs.at(input).riseTransition, cellInfo.timingArcs.at(input).fallTransition, nodes[0].totalCapacitance, slew, (cellInfo.isSequential?NON_UNATE:NEGATIVE_UNATE));
    current_source_slew = nodes[0].slew;

	do
	{


        updateSlews(cellInfo, input, slew, is_input_driver);
		updateEffectiveCapacitances();
        nodes[0].slew = RCTreeWireDelayModel::interpolator.interpolate(cellInfo.timingArcs.at(input).riseTransition, cellInfo.timingArcs.at(input).fallTransition, nodes[0].effectiveCapacitance, slew, (cellInfo.isSequential?NON_UNATE:NEGATIVE_UNATE));

        old_source_slew = current_source_slew;
        current_source_slew = nodes[0].slew;

		i++;
        error = abs(old_source_slew - current_source_slew) / max(abs(current_source_slew), abs(old_source_slew));

	}
    while (error.getRise() > Traits::STD_THRESHOLD/100 || error.getFall() > Traits::STD_THRESHOLD/100);
    for(int j = 0; j < _delays[input].size(); j++)
    {
        _max_delays[j] = max(_max_delays[j], _delays[input][j]);
        _max_slews[j] = max(_max_slews[j], _slews[input][j]);
    }

    return nodes.front().effectiveCapacitance;
}


void RCTreeWireDelayModel::updateSlews(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew, bool is_input_driver)
{
    nodes[0].delay.set(0.0f, 0.0f);


//    _max_slews[0] = max(_max_slews[0], nodes[0].slew);
//    _max_delays[0] = max(_max_delays[0], nodes[0].delay);

    _delays[input][0] = nodes[0].delay;
    _slews[input][0] = nodes[0].slew;
    for (size_t i = 1; i < nodes.size(); i++)
	{
        Transitions<double> & t_0_to_1 = nodes[i].delay;
        Transitions<double> & s_0 = nodes[nodes[i].parent].slew;
        Transitions<double> & r_1 = nodes[i].resistance;
        Transitions<double> & ceff_1 = nodes[i].effectiveCapacitance;
        Transitions<double> & s_1 = nodes[i].slew;


        t_0_to_1 = nodes[nodes[i].parent].delay + r_1 * ceff_1;

        const Transitions<double> x = r_1 * ceff_1 / s_0;
        s_1 = s_0 / ( 1 - x * ( 1 - exp( -1/x ) ) );

		if (nodes[i].sink)
		{
            _slews[input][fanoutNameToNodeNumber[nodesNames[i]]] = s_1;
            _delays[input][fanoutNameToNodeNumber[nodesNames[i]]] = t_0_to_1;

//            _max_slews[fanoutNameToNodeNumber[nodesNames[i]]] = max(_max_slews[fanoutNameToNodeNumber[nodesNames[i]]], _slews[input][fanoutNameToNodeNumber[nodesNames[i]]]);
//            _max_delays[fanoutNameToNodeNumber[nodesNames[i]]] = max(_max_delays[fanoutNameToNodeNumber[nodesNames[i]]], _delays[input][fanoutNameToNodeNumber[nodesNames[i]]]);
		}



	}
	
}



void RCTreeWireDelayModel::updateEffectiveCapacitances()
{
	vector<bool> initialized(nodes.size(), false);
    for (int j = nodes.size() - 1; j > 0; j--)
	{
        RCTreeWireDelayModel::Node & node_j = nodes.at(j);
        if (node_j.sink)
            node_j.effectiveCapacitance = node_j.nodeCapacitance;

        const Transitions<double> & c_tot_j = node_j.totalCapacitance;
        const Transitions<double> & ceff_j = node_j.effectiveCapacitance;
        const Transitions<double> & r_j = node_j.resistance;
        const Transitions<double> & s_i = nodes[node_j.parent].slew;


        const Transitions<double> x = 2 * r_j * ceff_j / s_i;
        const Transitions<double> y = 1 - exp(-1/x);


        // x = 2 * r_j * ceff_j / s_i
        // y = 1-e^(-1/x)
        // shielding_factor = 1-x*y
        const Transitions<double> shielding_factor = 1 - x * y;

        assert(shielding_factor.getRise() > 0.0f && shielding_factor.getRise() < 1.0f);
        assert(shielding_factor.getFall() > 0.0f && shielding_factor.getFall() < 1.0f);

        if (!initialized.at(node_j.parent))
		{
            nodes.at(node_j.parent).effectiveCapacitance = nodes.at(node_j.parent).nodeCapacitance;
            initialized.at(node_j.parent) = true;
		}

        nodes.at(node_j.parent).effectiveCapacitance += shielding_factor * c_tot_j;
	}
}

void RCTreeWireDelayModel::updateDownstreamCapacitances()
{
    for (size_t i = 0; i < nodes.size(); i++)
	{
		Node & node = nodes[i];
		node.totalCapacitance = node.nodeCapacitance;
	}
    for (size_t i = nodes.size() - 1; i > 0; i--)
	{
		Node & node = nodes[i];
		nodes[node.parent].totalCapacitance += node.totalCapacitance;
	}
}

void RCTreeWireDelayModel::initializeEffectiveCapacitances()
{
    for (size_t i = 0; i < nodes.size(); i++)
	{
		Node & node = nodes[i];
        node.effectiveCapacitance = node.totalCapacitance;
	}
}

const Transitions<double> RCTreeWireDelayModel::delay_at_fanout_node(const string fanout_node_name) const
{
    if(fanout_node_name == nodesNames.front())
        return _max_delays.front();
    assert(_max_delays.at(fanoutNameToNodeNumber.at(fanout_node_name)).getRise() > _max_delays.front().getRise());
    assert(_max_delays.at(fanoutNameToNodeNumber.at(fanout_node_name)).getFall() > _max_delays.front().getFall());
    return _max_delays.at(fanoutNameToNodeNumber.at(fanout_node_name));
}	
const Transitions<double> RCTreeWireDelayModel::slew_at_fanout_node(const string fanout_node_name) const
{
    if(fanout_node_name == nodesNames.front())
        return _max_slews.front();
    assert(_max_slews.at(fanoutNameToNodeNumber.at(fanout_node_name)).getRise() > _max_slews.front().getRise());
    assert(_max_slews.at(fanoutNameToNodeNumber.at(fanout_node_name)).getFall() > _max_slews.front().getFall());
    return _max_slews.at(fanoutNameToNodeNumber.at(fanout_node_name)) - _max_slews.front();
}


void RCTreeWireDelayModel::setFanoutPinCapacitance(const string fanoutNameAndPin, const double pinCapacitance)
{
    nodes.at(fanoutNameToNodeNumber.at(fanoutNameAndPin)).nodeCapacitance += pinCapacitance;
    _lumped_capacitance += pinCapacitance;
}

Transitions<double> RCTreeWireDelayModel::root_delay(int arc_number)
{
    return _delays.at(arc_number).front();
}

Transitions<double> RCTreeWireDelayModel::root_slew(int arc_number)
{
    return _slews.at(arc_number).front();
}

void RCTreeWireDelayModel::clear()
{
    std::fill(_max_delays.begin(), _max_delays.end(), numeric_limits<Transitions<double> >::zero());
    std::fill(_max_slews.begin(), _max_slews.end(), numeric_limits<Transitions<double> >::zero());
}

double WireDelayModel::lumped_capacitance() const
{
    return _lumped_capacitance;
}
