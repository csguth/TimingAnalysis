#include "include/circuit_netlist.h"

// CIRCUIT NETLIST
void Circuit_Netlist::updateTopology()
{
    vector<size_t> num_of_visited_inputs(gates.size(), 0);
    vector<bool> inserted_gate(gates.size(), false);
    vector<bool> inserted_net(nets.size(), false);

    queue<int> gates_queue;
    queue<int> primary_output_queue;
	for(size_t i = 0; i < gates.size(); i++)
	{

		if(gates[i].inputDriver)
		{
            gates_queue.push(i);
            inserted_gate[i] = true;
		}
	}

	topology.clear();
	netTopology.clear();
	
	inverseTopology.resize(gates.size(), -1);
	inverseNetTopology.resize(nets.size(), -1);

    while(!gates_queue.empty())
	{
        const int current_index = gates_queue.front();
        gates_queue.pop();

        const Logic_Gate & gate = gates.at(current_index);
        const Net & fanout_net = nets.at(gate.fanoutNetIndex);

        // insert input nets to the nets topology
        for(size_t i = 0; i < gate.inNets.size(); i++)
		{
            if(!inserted_net.at(gate.inNets.at(i)))
			{
                netTopology.push_back(gate.inNets.at(i));
                inverseNetTopology[gate.inNets.at(i)] = netTopology.size() - 1;
                inserted_net[gate.inNets.at(i)] = true;
			}
		}
        // insert output net to the nets topology
        if(!inserted_net.at(gate.fanoutNetIndex))
		{
			netTopology.push_back(gate.fanoutNetIndex);
            inverseNetTopology[gate.fanoutNetIndex] = netTopology.size() - 1;
            inserted_net[gate.fanoutNetIndex] = true;
		}
		
        // insert gate to the topolgy
        topology.push_back(current_index);
        inverseTopology[current_index] = topology.size() - 1;

        // push fanout gates to the process queue
        for(size_t i = 0; i < fanout_net.sinks.size(); i++)
		{
            const size_t fanoutIndex = fanout_net.sinks.at(i).gate;
            const Logic_Gate & fanout = gates[fanoutIndex];
            if(!inserted_gate.at(fanoutIndex))
			{
                if(++num_of_visited_inputs[fanoutIndex] == fanout.inNets.size())
				{
                    if(fanout.primary_output)
                        primary_output_queue.push(fanoutIndex);
                    else
                        gates_queue.push(fanoutIndex);
                    inserted_gate[fanoutIndex] = true;
				}
			}
		}
	}

    while(!primary_output_queue.empty())
    {
        const int PO_index = primary_output_queue.front();
        primary_output_queue.pop();
        Logic_Gate & gate = gates.at(PO_index);

        // Insert Input Nets
        for(size_t i = 0; i < gate.inNets.size(); i++)
        {
            if(!inserted_net.at(gate.inNets.at(i)))
            {
                netTopology.push_back(gate.inNets.at(i));
                inverseNetTopology[gate.inNets.at(i)] = netTopology.size() - 1;
                inserted_net[gate.inNets.at(i)] = true;
            }
        }
        // insert output nets
        if(!inserted_net.at(gate.fanoutNetIndex))
        {
            netTopology.push_back(gate.fanoutNetIndex);
            inverseNetTopology[gate.fanoutNetIndex] = netTopology.size() - 1;
            inserted_net[gate.fanoutNetIndex] = true;
        }

        topology.push_back(PO_index);
        inverseTopology[PO_index] = topology.size() - 1;


    }

    assert(netTopology.size() == inverseNetTopology.size());
    assert(topology.size() == inverseTopology.size());

}

int Circuit_Netlist::addGate(const string name, const string cellType, const int inputs, const bool isInputDriver, const bool primary_output)
{
	if(gateNameToGateIndex.find(name) != gateNameToGateIndex.end())
		return gateNameToGateIndex[name];

	//const string name, const string cellType, const unsigned inputs, int fanoutNetIndex
	gates.push_back(Logic_Gate(name, cellType, inputs, -1, isInputDriver, primary_output));
//    _timing_points++;
//    if(cellType != "__PO__" || (gates.back().sequential && !gates.back().inputDriver))
//    {
//        _timing_points += gates.back().inNets.size();
//        _timing_arcs += inputs;
//    }

	gateNameToGateIndex[name] = gates.size() - 1;

	return gateNameToGateIndex[name];
}

int Circuit_Netlist::addNet(const string name)
{
	if(netNameToNetIndex.find(name) != netNameToNetIndex.end())
		return netNameToNetIndex[name];

	nets.push_back(Net(name, -1, "o"));
	netNameToNetIndex[name] = nets.size() - 1;
	return netNameToNetIndex[name];
}

int Circuit_Netlist::addNet(const string name, const int sourceNode, const string sourcePin)
{
	if(netNameToNetIndex.find(name) != netNameToNetIndex.end())
	{
		if(nets.at(netNameToNetIndex.at(name)).sourceNode == -1)
		{
			// cout << "solved source" << endl;
			nets.at(netNameToNetIndex.at(name)).sourceNode = sourceNode;
		}

		assert(nets.at(netNameToNetIndex.at(name)).sourceNode == sourceNode);

		return netNameToNetIndex.at(name);
	} 

	nets.push_back(Net(name, sourceNode, sourcePin));
	netNameToNetIndex[name] = nets.size() - 1;
	return netNameToNetIndex[name];
}

void Circuit_Netlist::addCellInst(const string name, const string cellType, vector<pair<string, string> > inputPinPairs, const bool isSequential, const bool isInputDriver, const bool primary_output)
{
	const string outputPin = inputPinPairs.back().first;
	const string fanoutNetName = inputPinPairs.back().second;

	if(isSequential)
	{
		const int gateIndex = addGate(name, cellType, inputPinPairs.size() - 1);
		const int netIndex = addNet(name + "_PO", gateIndex, outputPin);
		Logic_Gate & gate = gates[gateIndex];
		gate.fanoutNetIndex = netIndex;
		gate.sequential = isSequential;
		for(size_t i = 0; i < inputPinPairs.size() - 1 ; i++)
		{
			const int faninNetIndex = addNet(inputPinPairs[i].second);
			Net & faninNet = nets[faninNetIndex];
			faninNet.addSink(Sink(gateIndex, inputPinPairs[i].first));
			gate.inNets[i] = faninNetIndex;
		}


		// Creates a INPUT DRIVER to the flip flop
		inputPinPairs.front().second += "_PI";
		const int sequentialCellGateIndex = addGate(name + "_PI", cellType, inputPinPairs.size() - 1, true);
		const int sequentialCellGatePINetIndex = addNet(fanoutNetName, sequentialCellGateIndex, outputPin);
		Logic_Gate & sequentialGate = gates[sequentialCellGateIndex];
		sequentialGate.fanoutNetIndex = sequentialCellGatePINetIndex;
		sequentialGate.sequential = isSequential;
		for(size_t i = 0; i < inputPinPairs.size() - 1 ; i++)
		{
			const int faninNetIndex = addNet(inputPinPairs[i].second);
			Net & faninNet = nets[faninNetIndex];
			faninNet.addSink(Sink(sequentialCellGateIndex, inputPinPairs[i].first));
			sequentialGate.inNets[i] = faninNetIndex;
		}

	}
	else
	{
		const int gateIndex = addGate(name, cellType, inputPinPairs.size() - 1, isInputDriver, primary_output);
		const int netIndex = addNet(fanoutNetName, gateIndex, outputPin);
		Logic_Gate & gate = gates[gateIndex];
		Net & net = nets[netIndex];
		if(net.sourceNode == -1)
			net.sourceNode = gateIndex;
		gate.fanoutNetIndex = netIndex;
		for(size_t i = 0; i < inputPinPairs.size() - 1 ; i++)
		{
			const int faninNetIndex = addNet(inputPinPairs[i].second);
			Net & faninNet = nets[faninNetIndex];
			faninNet.addSink(Sink(gateIndex, inputPinPairs[i].first));
			gate.inNets[i] = faninNetIndex;
		}
		gate.sequential = isSequential;

	}
	if( !primary_output && !isInputDriver )
		_numberOfGates++;



}

const vector<pair<int, string> > Circuit_Netlist::verilog() const 
{
	vector<pair<int, string> > verilogVector(_numberOfGates);
	int j = 0;
    for(size_t i = 0; i < gates.size(); i++)
	{
		if(gates.at(i).inputDriver || gates.at(i).primary_output )
			continue;
		const pair<int, string> indexAndName = make_pair(inverseTopology.at(i), gates.at(i).name);
		verilogVector[j++] = indexAndName;
	}
    return verilogVector;
}

int Circuit_Netlist::timing_arcs() const
{
    return _timing_arcs;
}

int Circuit_Netlist::timing_points() const
{
    return _timing_points;
}
