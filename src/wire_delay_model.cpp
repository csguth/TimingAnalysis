#include "include/wire_delay_model.h"

namespace  Timing_Analysis {
LinearLibertyLookupTableInterpolator WireDelayModel::interpolator;

const Transitions<double> LumpedCapacitanceWireDelayModel::simulate(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew, bool is_input_driver)
{
    Unateness unateness = NEGATIVE_UNATE;
    if(cellInfo.isSequential)
        unateness = NON_UNATE;
    _slew = WireDelayModel::interpolator.interpolate(cellInfo.timingArcs.at(input).riseTransition, cellInfo.timingArcs.at(input).fallTransition, Transitions<double>(_lumped_capacitance, _lumped_capacitance), slew, unateness);
    _delay = WireDelayModel::interpolator.interpolate(cellInfo.timingArcs.at(input).riseDelay, cellInfo.timingArcs.at(input).fallDelay, Transitions<double>(_lumped_capacitance, _lumped_capacitance), slew, unateness, is_input_driver);
    _max_slew = max(_max_slew, _slew);
    return Transitions<double>(_lumped_capacitance, _lumped_capacitance);
}

// Any fanout node has the same delay and slew
const Transitions<double> LumpedCapacitanceWireDelayModel::delay_at_fanout_node(const string fanout_node_name) const {
    return numeric_limits<Transitions<double> >::zero();
}
const Transitions<double> LumpedCapacitanceWireDelayModel::slew_at_fanout_node(const string fanout_node_name) const {
    return _max_slew;
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
    _max_slew = numeric_limits<Transitions<double> >::min();
}


RC_Tree_Wire_Delay_Model::RC_Tree_Wire_Delay_Model(const SpefNetISPD2013 & descriptor, const string rootNode, const size_t arcs_size, const bool dummyEdge) : WireDelayModel(descriptor.netLumpedCap, descriptor.total_resistance), _nodes(descriptor.nodesSize()), _nodes_names(descriptor.nodesSize()), _slews(arcs_size, vector<Transitions<double> >(descriptor.nodesSize())),  _delays(arcs_size, vector<Transitions<double> >(descriptor.nodesSize()))
{
    if (dummyEdge)
        return;

    // criar um vetor de fanouts com referência para os timing points de seus fanouts

    const int rootIndex = descriptor.getNodeIndex(rootNode);
    const SpefNetISPD2013::Node & root = descriptor.getNode(rootIndex);
    queue<NodeAndResistor> q;
    vector<bool> added(descriptor.resistorsSize(), false);
    vector<bool> nodes_added(_nodes.size(), false);

    for (unsigned i = 0; i < root.resistors.size(); i++)
    {
        const int resistorIndex = root.resistors[i];
        const SpefNetISPD2013::Resistor & resistor = descriptor.getResistor(resistorIndex);
        q.push(NodeAndResistor(resistor.getOtherNode(rootIndex), resistorIndex));
        nodes_added.at(resistor.getOtherNode(rootIndex)) = true;
        added.at(resistorIndex) = true;
    }
    int neighbourhood;
    vector<int> topology(_nodes.size(), -1);
    vector<int> reverseTopology(topology);

    _nodes[0].nodeCapacitance.set(root.capacitance, root.capacitance);
    topology[0] = rootIndex;
    _nodes_names[0] = rootNode;
    reverseTopology[rootIndex] = 0;
    int counter = 1;

    while (!q.empty())
    {
        const int & n = q.front().nodeIndex;
        const int & r = q.front().resistorIndex;
        q.pop();

        const SpefNetISPD2013::Node & nDescriptor = descriptor.getNode(n);
        const SpefNetISPD2013::Resistor & rDescriptor = descriptor.getResistor(r);

        _nodes_names[counter] = nDescriptor.name;
        _nodes[counter].parent = reverseTopology[rDescriptor.getOtherNode(nDescriptor.nodeIndex)];
        _nodes[counter].nodeCapacitance.set(nDescriptor.capacitance, nDescriptor.capacitance);
        _nodes[counter].resistance.set(rDescriptor.value, rDescriptor.value);

        topology[counter] = n;
        reverseTopology[n] = counter;

        neighbourhood = 0;
        for (unsigned i = 0; i < nDescriptor.resistors.size(); i++)
        {
            if (!added.at(nDescriptor.resistors[i]))
            {
                const SpefNetISPD2013::Resistor & resistor = descriptor.getResistor(nDescriptor.resistors[i]);
                if(!nodes_added.at(resistor.getOtherNode(nDescriptor.nodeIndex)))
                {
                    q.push(NodeAndResistor(resistor.getOtherNode(nDescriptor.nodeIndex), nDescriptor.resistors[i]));
                    nodes_added.at(resistor.getOtherNode(nDescriptor.nodeIndex)) = true;
                }
                added[nDescriptor.resistors[i]] = true;
                neighbourhood++;
            }
        }

        _nodes[counter].sink = (neighbourhood == 0);

        _node_name_to_node_number[nDescriptor.name] = counter;

        counter++;
    }

    IBM_update_downstream_capacitances();
    IBM_initialize_effective_capacitances();
}


const Transitions<double> Ceff_Elmore_Slew_Degradation_PURI::simulate(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew, bool is_input_driver)
{
    Transitions<double> ceff = run_IBM_algorithm(cellInfo, input, slew, is_input_driver);
    //    for(int i = 1; i < _delays.at(input).size(); i++)
    //    {
    //        _delays.at(input).at(i) *= log(2);
    //    }
    return ceff;
}


void RC_Tree_Wire_Delay_Model::IBM_update_slews(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew, bool is_input_driver)
{
    _nodes[0].delay.set(0.0f, 0.0f);

    _delays.at(input).front() = _nodes[0].delay;
    _slews.at(input).front() = _nodes[0].slew;

    for (size_t i = 1; i < _nodes.size(); i++)
    {
        Transitions<double> & t_0_to_1 = _nodes[i].delay;
        Transitions<double> & s_0 = _nodes[_nodes[i].parent].slew;
        Transitions<double> & r_1 = _nodes[i].resistance;
        Transitions<double> & ceff_1 = _nodes[i].effectiveCapacitance;
        Transitions<double> & s_1 = _nodes[i].slew;

        t_0_to_1 = _nodes[_nodes[i].parent].delay + r_1 * ceff_1;

        const Transitions<double> x = r_1 * ceff_1 / s_0;
        s_1 = s_0 / ( 1 - x * ( 1 - exp( -1/x ) ) );


        _delays.at(input).at(i) = t_0_to_1;
        _slews.at(input).at(i) = s_1;
    }

}

void RC_Tree_Wire_Delay_Model::IBM_update_effective_capacitances()
{
    vector<bool> initialized(_nodes.size(), false);
    for (int j = _nodes.size() - 1; j > 0; j--)
    {
        RC_Tree_Wire_Delay_Model::Node & node_j = _nodes.at(j);
        if (node_j.sink)
            node_j.effectiveCapacitance = node_j.nodeCapacitance;

        const Transitions<double> & c_tot_j = node_j.totalCapacitance;
        const Transitions<double> & ceff_j = node_j.effectiveCapacitance;
        const Transitions<double> & r_j = node_j.resistance;
        const Transitions<double> & s_i = _nodes[node_j.parent].slew;


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
            _nodes.at(node_j.parent).effectiveCapacitance = _nodes.at(node_j.parent).nodeCapacitance;
            initialized.at(node_j.parent) = true;
        }

        _nodes.at(node_j.parent).effectiveCapacitance += shielding_factor * c_tot_j;
    }
}

void RC_Tree_Wire_Delay_Model::IBM_update_downstream_capacitances()
{
    for (size_t i = 0; i < _nodes.size(); i++)
    {
        Node & node = _nodes[i];
        node.totalCapacitance = node.nodeCapacitance;
    }
    for (size_t i = _nodes.size() - 1; i > 0; i--)
    {
        Node & node = _nodes[i];
        _nodes[node.parent].totalCapacitance += node.totalCapacitance;
    }
}

void RC_Tree_Wire_Delay_Model::IBM_initialize_effective_capacitances()
{
    for (size_t i = 0; i < _nodes.size(); i++)
    {
        Node & node = _nodes[i];
        node.effectiveCapacitance = node.totalCapacitance;
    }
}

const Transitions<double> RC_Tree_Wire_Delay_Model::run_IBM_algorithm(const LibertyCellInfo &cellInfo, const int input, const Transitions<double> slew, bool is_input_driver)
{
    IBM_update_downstream_capacitances();
    IBM_initialize_effective_capacitances();
    //forwardIterate();
    Transitions<double> error;

    Transitions<double> current_source_slew, old_source_slew;
    Transitions<double> current_source_ceff, old_source_ceff;
    int i = 0;

    _nodes[0].slew = RC_Tree_Wire_Delay_Model::interpolator.interpolate(cellInfo.timingArcs.at(input).riseTransition, cellInfo.timingArcs.at(input).fallTransition, _nodes[0].totalCapacitance, slew, (cellInfo.isSequential?NON_UNATE:NEGATIVE_UNATE));
    current_source_slew = _nodes[0].slew;
    current_source_ceff = _nodes[0].totalCapacitance;

    do
    {


        IBM_update_slews(cellInfo, input, slew, is_input_driver);
        IBM_update_effective_capacitances();
        _nodes[0].slew = RC_Tree_Wire_Delay_Model::interpolator.interpolate(cellInfo.timingArcs.at(input).riseTransition, cellInfo.timingArcs.at(input).fallTransition, _nodes[0].effectiveCapacitance, slew, (cellInfo.isSequential?NON_UNATE:NEGATIVE_UNATE));

        old_source_slew = current_source_slew;
        current_source_slew = _nodes[0].slew;


        old_source_ceff = current_source_ceff;
        current_source_ceff = _nodes[0].effectiveCapacitance;


        i++;
        error = abs(old_source_slew - current_source_slew) / max(abs(old_source_slew), abs(current_source_slew));
        //        error = abs(old_source_ceff - current_source_ceff) / max(abs(old_source_ceff), abs(current_source_ceff));


    }
    while (error.getRise() > Traits::STD_THRESHOLD || error.getFall() > Traits::STD_THRESHOLD);

    return _nodes.front().effectiveCapacitance;
}




void RC_Tree_Wire_Delay_Model::setFanoutPinCapacitance(const string fanoutNameAndPin, const double pinCapacitance)
{
    _nodes.at(_node_name_to_node_number.at(fanoutNameAndPin)).nodeCapacitance += pinCapacitance;
    _nodes.at(_node_name_to_node_number.at(fanoutNameAndPin)).sink = true;
    _lumped_capacitance += pinCapacitance;
}

Transitions<double> RC_Tree_Wire_Delay_Model::root_delay(int arc_number)
{
    return _delays.at(arc_number).front();
}

Transitions<double> RC_Tree_Wire_Delay_Model::root_slew(int arc_number)
{
    return _slews.at(arc_number).front();
}

void RC_Tree_Wire_Delay_Model::clear()
{
    //    std::fill(_max_delays.begin(), _max_delays.end(), numeric_limits<Transitions<double> >::zero());
    //    std::fill(_max_slews.begin(), _max_slews.end(), numeric_limits<Transitions<double> >::zero());
}

void Reduced_Pi::reduce_to_pi_model(double &c_near, double &r, double &c_far)
{
    const int number_of_nodes = _nodes.size();

    std::vector<double> y1(number_of_nodes, 0.0);
    std::vector<double> y2(number_of_nodes, 0.0);
    std::vector<double> y3(number_of_nodes, 0.0);

    // Compute pi-model of the RC Tree.
    for ( int n = number_of_nodes - 1; n > 0; n-- ) { // n > 0 skips root node
        const Node &node = _nodes[n];

        const double C = node.nodeCapacitance.getMax();
        const double R = node.resistance.getMax();

        const double yD1 = y1[n];
        const double yD2 = y2[n];
        const double yD3 = y3[n];

        const double yU1 = yD1 + C;
        const double yU2 = yD2 - R * (pow(yD1,2.0) + C*yD1 + (1.0/3.0)*pow(C, 2.0));
        const double yU3 = yD3 - R * (2*yD1*yD2 + C*yD2) +
                pow(R,2.0)*( pow(yD1,3.0) + (4.0/3.0)*C*pow(yD1,2.0) + (2.0/3.0)*pow(C,2.0)*yD1 + (2.0/15.0)*pow(C,3.0) );

        y1[node.parent] += yU1;
        y2[node.parent] += yU2;
        y3[node.parent] += yU3;

        //cout << "Resistor: " << clsNodeNames[node.propParent] << " -> " << clsNodeNames[n] << "\n";

    } // end for

    c_near = pow(y2[0],2.0) / y3[0];
    c_far = y1[0] - c_near;
    r = -pow(y3[0],2.0)/pow(y2[0],3.0);
}

double WireDelayModel::lumped_capacitance() const
{
    return _lumped_capacitance;
}

double WireDelayModel::total_resistance() const
{
    return _total_resistance;
}


const Transitions<double> Reduced_Pi::simulate(const LibertyCellInfo &cellInfo, const int input, const Transitions<double> slew, bool is_input_driver)
{

    reduce_to_pi_model(_c1, _r, _c2);

    //    assert(_c1 + _c2 == _lumped_capacitance);
    std::vector<Node> nodes(2);

    Node & C1 = nodes.front();
    Node & C2 = nodes.back();

    C1.parent = -1;
    C1.nodeCapacitance.set(_c1, _c1);
    C1.totalCapacitance.set(_c1+_c2, _c1+_c2);
    C1.effectiveCapacitance = C1.totalCapacitance;
    C1.resistance.set(0.0f, 0.0f);


    const Transitions<double> driver_delay = RC_Tree_Wire_Delay_Model::interpolator.interpolate(cellInfo.timingArcs.at(input).riseTransition, cellInfo.timingArcs.at(input).fallTransition, C1.totalCapacitance, slew, (cellInfo.isSequential?NON_UNATE:NEGATIVE_UNATE));
    const Transitions<double> driver_resistance = driver_delay / C1.totalCapacitance;
    C1.effectiveCapacitance = _c1 + (driver_resistance / (driver_resistance + _r)) * _c2;

    C1.slew = RC_Tree_Wire_Delay_Model::interpolator.interpolate(cellInfo.timingArcs.at(input).riseTransition, cellInfo.timingArcs.at(input).fallTransition, C1.totalCapacitance, slew, (cellInfo.isSequential?NON_UNATE:NEGATIVE_UNATE));
    C1.delay.set(0.0f, 0.0f);

    C2.parent = 0;
    C2.nodeCapacitance.set(_c2, _c2);
    C2.totalCapacitance = C2.nodeCapacitance;
    C2.effectiveCapacitance = C2.totalCapacitance;
    C2.resistance.set(_r, _r);
    C2.slew.set(0.0f, 0.0f);
    C2.delay.set(0.0f, 0.0f);



    bool converged;
    Transitions<double> prev_source_slew, current_source_slew;
    current_source_slew = C1.slew;
    do
    {
        // UPDATE SLEWS
        Transitions<double> & t_0_to_1 = C2.delay;
        Transitions<double> & s_0 = C1.slew;
        Transitions<double> & r_1 = C2.resistance;
        Transitions<double> & ceff_1 = C2.effectiveCapacitance;
        Transitions<double> & s_1 = C2.slew;
        t_0_to_1 = C1.delay + r_1 * ceff_1;
        const Transitions<double> x = r_1 * ceff_1 / s_0;
        s_1 = s_0 / ( 1 - x * ( 1 - exp( -1/x ) ) );

        // UPDATE CEFF
        C2.effectiveCapacitance = C2.nodeCapacitance;

        const Transitions<double> & c_tot_j = C2.totalCapacitance;
        const Transitions<double> & ceff_j = C2.effectiveCapacitance;
        const Transitions<double> & r_j = C2.resistance;
        const Transitions<double> & s_i = C1.slew;

        const Transitions<double> z = 2 * r_j * ceff_j / s_i;
        const Transitions<double> y = 1 - exp(-1/z);

        const Transitions<double> shielding_factor = 1 - z * y;

        assert(shielding_factor.getRise() > 0.0f && shielding_factor.getRise() < 1.0f);
        assert(shielding_factor.getFall() > 0.0f && shielding_factor.getFall() < 1.0f);

        C1.effectiveCapacitance = C1.nodeCapacitance;
        C1.effectiveCapacitance += shielding_factor * c_tot_j;

        prev_source_slew = current_source_slew;
        C1.slew = RC_Tree_Wire_Delay_Model::interpolator.interpolate(cellInfo.timingArcs.at(input).riseTransition, cellInfo.timingArcs.at(input).fallTransition, C1.effectiveCapacitance, slew, (cellInfo.isSequential?NON_UNATE:NEGATIVE_UNATE));
        current_source_slew = C1.slew;

        const Transitions<double> error =  abs(prev_source_slew - current_source_slew) / max(abs(prev_source_slew), abs(current_source_slew));
        converged = error.getMin() < 0.01f;
    } while(!converged);

    _slews.at(input).front() = C1.slew;
    _slew_fanout = C2.slew;
    _delay_fanout = C2.delay;

    return C1.effectiveCapacitance;

}

const Transitions<double> Reduced_Pi::delay_at_fanout_node(const string fanout_node_name)  const
{
    return _delay_fanout;
}

const Transitions<double> Reduced_Pi::slew_at_fanout_node(const string fanout_node_name) const
{
    return _slew_fanout;
}


const Transitions<double> Lumped_Elmore_Slew_Degradation::simulate(const LibertyCellInfo &cellInfo, const int input, const Transitions<double> slew, bool is_input_driver)
{
    IBM_update_downstream_capacitances();
    IBM_initialize_effective_capacitances();
    _nodes[0].slew = RC_Tree_Wire_Delay_Model::interpolator.interpolate(cellInfo.timingArcs.at(input).riseTransition, cellInfo.timingArcs.at(input).fallTransition, _nodes[0].totalCapacitance, slew, (cellInfo.isSequential?NON_UNATE:NEGATIVE_UNATE));
    IBM_update_slews(cellInfo, input, slew, is_input_driver);

    //    for(int i = 1; i < _delays.at(input).size(); i++)
    //    {
    //        _delays.at(input).at(i) *= log(2);
    //    }
    return _nodes.front().effectiveCapacitance;
}

const Transitions<double> RC_Tree_Wire_Delay_Model::delay_at_fanout_node(const string fanout_node_name) const
{
    int fanout_index = _node_name_to_node_number.at(fanout_node_name);

    Transitions<double> max_delay = _delays.front().at(fanout_index);
    for(int i = 1; i < _delays.size(); i++)
    {
        max_delay = max(max_delay, _delays.at(i).at(fanout_index));
    }
    return max_delay;
}
const Transitions<double> RC_Tree_Wire_Delay_Model::slew_at_fanout_node(const string fanout_node_name) const
{
    int fanout_index = _node_name_to_node_number.at(fanout_node_name);
    Transitions<double> max_slew = _slews.front().at(fanout_index);
    for(int i = 1; i < _slews.size(); i++)
    {
        max_slew = max(max_slew, _slews.at(i).at(fanout_index));
    }
    return max_slew;
}


const Transitions<double> Lumped_Elmore_No_Slew_Degradation::simulate(const LibertyCellInfo &cellInfo, const int input, const Transitions<double> slew, bool is_input_driver)
{
    IBM_update_downstream_capacitances();
    IBM_initialize_effective_capacitances();
    _nodes[0].slew = RC_Tree_Wire_Delay_Model::interpolator.interpolate(cellInfo.timingArcs.at(input).riseTransition, cellInfo.timingArcs.at(input).fallTransition, _nodes[0].totalCapacitance, slew, (cellInfo.isSequential?NON_UNATE:NEGATIVE_UNATE));
    IBM_update_slews(cellInfo, input, slew, is_input_driver);

    std::fill(_slews.at(input).begin(), _slews.at(input).end(), _slews.at(input).front());

    return _nodes.front().effectiveCapacitance;
}


const Transitions<double> Ceff_Elmore_No_Slew_Degradation::simulate(const LibertyCellInfo &cellInfo, const int input, const Transitions<double> slew, bool is_input_driver)
{
    Transitions<double> ceff = run_IBM_algorithm(cellInfo, input, slew, is_input_driver);

    std::fill(_slews.at(input).begin(), _slews.at(input).end(), _slews.at(input).front());

    return ceff;
}


const Transitions<double> Ceff_Without_Wire_Delay_And_Slew_Degradation::simulate(const LibertyCellInfo &cellInfo, const int input, const Transitions<double> slew, bool is_input_driver)
{
    IBM_update_downstream_capacitances();
    IBM_initialize_effective_capacitances();
    //forwardIterate();
    Transitions<double> error;

    Transitions<double> current_source_slew, old_source_slew;
    int i = 0;

    _nodes[0].slew = RC_Tree_Wire_Delay_Model::interpolator.interpolate(cellInfo.timingArcs.at(input).riseTransition, cellInfo.timingArcs.at(input).fallTransition, _nodes[0].totalCapacitance, slew, (cellInfo.isSequential?NON_UNATE:NEGATIVE_UNATE));
    current_source_slew = _nodes[0].slew;

    do
    {
        IBM_update_slews(cellInfo, input, slew, is_input_driver);
        IBM_update_effective_capacitances();
        _nodes[0].slew = RC_Tree_Wire_Delay_Model::interpolator.interpolate(cellInfo.timingArcs.at(input).riseTransition, cellInfo.timingArcs.at(input).fallTransition, _nodes[0].effectiveCapacitance, slew, (cellInfo.isSequential?NON_UNATE:NEGATIVE_UNATE));

        old_source_slew = current_source_slew;
        current_source_slew = _nodes[0].slew;

        i++;
        error = abs(old_source_slew - current_source_slew) / max(abs(current_source_slew), abs(old_source_slew));

    }
    while (error.getRise() > Traits::STD_THRESHOLD/100 || error.getFall() > Traits::STD_THRESHOLD/100);

    std::fill(_delays.at(input).begin(), _delays.at(input).end(), numeric_limits<Transitions<double> >::zero());
    std::fill(_slews.at(input).begin(), _slews.at(input).end(), _slews.at(input).front());
    return _nodes.front().effectiveCapacitance;
}


const Transitions<double> Ceff_Elmore_Slew_Degradation::simulate(const LibertyCellInfo &cellInfo, const int input, const Transitions<double> slew, bool is_input_driver)
{
    Transitions<double> ceff = run_IBM_algorithm(cellInfo, input, slew, is_input_driver);
    Transitions<double> root_slew = _slews.at(input).front();

    vector<Transitions<double> > ceff_elmore_delays(_delays.at(input).size());

    //    for(int i = 0; i < _slews[input].size(); i++)
    //    {
    //        ceff_elmore_delays.at(i) = _delays[input][i];
    //    }


    for(int i = 1; i < _nodes.size(); i++)
    {
        Node & node = _nodes.at(i);
        Node & parent = _nodes.at(node.parent);
        node.delay = parent.delay + node.resistance * node.totalCapacitance;

        _delays[input][i] = node.delay;
    }


    //    for(int i = 0; i < _slews[input].size(); i++)
    //    {
    ////        Transitions<double> degradation = _slews.at(input).at(i);
    ////        Transitions<double> degradation = _slews[input][i] - root_slew;
    //        Transitions<double> degradation = _delays.at(input).at(i) * log(4);

    ////        if(_nodes.at(i).sink)
    ////            cout << "root slew = " << root_slew << ", leaf slew = " << _slews.at(input).at(i) << endl;

    ////        _slews.at(input).at(i) = sqrt(root_slew * root_slew + degradation * degradation);
    ////        _slews.at(input).at(i) = root_slew;

    ////          _slews[input][i] = (_slews[input][i] + root_slew) / 2;
    ////        _delays[input][i] *= log(2);

    //    }

    //    _delays[input] = ceff_elmore_delays;



    return ceff;
}

}
