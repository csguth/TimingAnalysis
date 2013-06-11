#include "include/CircuitNetList.h"

// CIRCUIT NETLIST
void CircuitNetList::updateTopology()
{
	vector<int> inputsVisiteds(gates.size(), 0);
	vector<bool> inserted(gates.size(), false);
	vector<bool> insertedN(nets.size(), false);

	queue<int> q;
	for(size_t i = 0; i < gates.size(); i++)
	{
		if(gates[i].inputDriver)
		{
			q.push(i);
			inserted[i] = true;
		}
	}

	topology.clear();
	netTopology.clear();
	
	while(!q.empty())
	{
		const int currentIndex = q.front();
		q.pop();

		LogicGate & gate = gates[currentIndex];
		const Net & fanoutNet = nets[gate.fanoutNetIndex];
		for(size_t i = 0; i < gate.inNets.size(); i++)
		{
			if(!insertedN[gate.inNets[i]])
			{
				netTopology.push_back(gate.inNets[i]);
				insertedN[gate.inNets[i]] = true;
			}
		}
		if(!insertedN[gate.fanoutNetIndex])
		{
			netTopology.push_back(gate.fanoutNetIndex);
			insertedN[gate.fanoutNetIndex] = true;
		}
		
		topology.push_back(currentIndex);
		for(size_t i = 0; i < fanoutNet.sinks.size(); i++)
		{
			const int fanoutIndex = fanoutNet.sinks[i].gate;
			LogicGate & fanout = gates[fanoutIndex];
			if(!inserted[fanoutIndex])
			{
				if(++inputsVisiteds[fanoutIndex] == fanout.inNets.size())
				{
					q.push(fanoutIndex);
					inserted[fanoutIndex] = true;
				}
			}
		}
	}
	
	// cout << "printing topology: " << endl;
	// for(size_t i = 0; i < topology.size(); i++)
	// {
	// 	LogicGate & gate = gates[topology[i]];
	// 	cout << "topology[" << i << "] = " << gate << endl;
	// }
}

const int CircuitNetList::addGate(const string name, const string cellType, const int inputs, const bool isInputDriver)
{
	if(gateNameToGateIndex.find(name) != gateNameToGateIndex.end())
		return gateNameToGateIndex[name];

	//const string name, const string cellType, const unsigned inputs, int fanoutNetIndex
	gates.push_back(LogicGate(name, cellType, inputs, -1, isInputDriver));
	gateNameToGateIndex[name] = gates.size() - 1;
	return gateNameToGateIndex[name];
}

const int CircuitNetList::addNet(const string name)
{
	if(netNameToNetIndex.find(name) != netNameToNetIndex.end())
		return netNameToNetIndex[name];

	nets.push_back(Net(name, -1, "o"));
	netNameToNetIndex[name] = nets.size() - 1;
	return netNameToNetIndex[name];
}

const int CircuitNetList::addNet(const string name, const int sourceNode, const string sourcePin)
{
	if(netNameToNetIndex.find(name) != netNameToNetIndex.end())
		return netNameToNetIndex[name];

	nets.push_back(Net(name, sourceNode, sourcePin));
	netNameToNetIndex[name] = nets.size() - 1;
	return netNameToNetIndex[name];
}

void CircuitNetList::addCellInst(const string name, const string cellType, vector<pair<string, string> > inputPinPairs, const bool isSequential, const bool isInputDriver)
{
	const string outputPin = inputPinPairs.back().first;
	const string fanoutNetName = inputPinPairs.back().second;

	if(isSequential)
	{
		const int gateIndex = addGate(name, cellType, inputPinPairs.size() - 1);
		const int netIndex = addNet(fanoutNetName + "_PO", gateIndex, outputPin);
		LogicGate & gate = gates[gateIndex];
		gate.fanoutNetIndex = netIndex;

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
		LogicGate & sequentialGate = gates[sequentialCellGateIndex];
		sequentialGate.fanoutNetIndex = sequentialCellGatePINetIndex;

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
		const int gateIndex = addGate(name, cellType, inputPinPairs.size() - 1, isInputDriver);
		const int netIndex = addNet(fanoutNetName, gateIndex, outputPin);
		LogicGate & gate = gates[gateIndex];
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
	}
}