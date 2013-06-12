#include "include/WireDelayModel.h"

const double LumpedCapacitanceWireDelayModel::simulate()
{
	return lumpedCapacitance;
}

RCTreeWireDelayModel::RCTreeWireDelayModel(const SpefNetISPD2013 & descriptor, const bool dummyEdge, const string rootNode) : WireDelayModel(lumpedCapacitance), nodes(descriptor.nodesSize()), nodesNames(descriptor.nodesSize())
{
	if (dummyEdge)
		return;


	cout << "root node: " << rootNode << endl;


	const int rootIndex = descriptor.getNodeIndex(rootNode);

	cout << "root index " << rootIndex << endl;

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

	nodes[0].nodeCapacitance = root.capacitance;
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
		nodes[counter].nodeCapacitance = nDescriptor.capacitance;
		nodes[counter].resistance = rDescriptor.value;

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

const double RCTreeWireDelayModel::simulate()
{
	/*
initializeEffectiveCapacitances();
	updateDownstreamCapacitances();
	//forwardIterate();
	Transitions<double> error;

	Transitions<double> ceff0, ceffF;
	int i = 0;
	do
	{
		ceff0 = nodes[0].effectiveCapacitance;
		updateSlews(cellType, option, input, slew);
		updateEffectiveCapacitances();
		ceffF = nodes[0].effectiveCapacitance;
		i++;
		error = abs(ceffF - ceff0) / max(abs(ceff0), abs(ceffF));
	}
	while (error.getRise() > Defines::STD_THRESHOLD || error.getFall() > Defines::STD_THRESHOLD);
	*/
	const double ceff = 1.2f;
	return ceff;
}


void RCTreeWireDelayModel::updateSlews(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew)
{
	/*

nodes[0].slew = cellType->computeOutputSlew(option, input, slew, Transitions<double>(nodes[0].totalCapacitance, nodes[0].totalCapacitance));
	_worstSlew = max(_worstSlew, nodes[0].slew);
	nodes[0].delay = cellType->computeDelay(option, input, slew, Transitions<double>(nodes[0].totalCapacitance, nodes[0].totalCapacitance));
	for (int i = 1; i < nodes.size(); i++)
	{
		Transitions<double> & T01 = nodes[i].delay;
		const Transitions<double> & S0 = nodes[nodes[i].parent].slew;
		const double & R1 = nodes[i].resistance;
		const Transitions<double> & Ceff = nodes[i].effectiveCapacitance;
		const Transitions<double> R1TimesCeffOverS0 = R1 * Ceff / S0;
		const Transitions<double> S0OverR1TimesCeff = 1 / R1TimesCeffOverS0;
		Transitions<double> & S1 = nodes[i].slew;
		T01 = nodes[nodes[i].parent].delay + R1 * Ceff;
		S1 = S0 / (1 - (R1TimesCeffOverS0) * (1 - exp(-(S0OverR1TimesCeff))));
		if (nodes[i].sink)
		{
			_slews[fanoutNameToNodeNumber[nodesNames[i]]] = S1;
			_delays[fanoutNameToNodeNumber[nodesNames[i]]] = T01;
		}

	}
	*/
}



void RCTreeWireDelayModel::updateEffectiveCapacitances()
{
	vector<bool> initialized(nodes.size(), false);
	for (int i = nodes.size() - 1; i > 0; i--)
	{
		RCTreeWireDelayModel::Node & node = nodes[i];
		if (node.sink)
			node.effectiveCapacitance.set(node.nodeCapacitance, node.nodeCapacitance);

		const double & CTot = node.totalCapacitance;
		const Transitions<double> & Ceff = node.effectiveCapacitance;
		const double & R = node.resistance;
		const Transitions<double> & S = nodes[node.parent].slew;
		const Transitions<double> TwoTimesRTimesCeffOverS = 2 * R * Ceff / S;
		const Transitions<double> SOverTwoTimesRTimesCeff = 1 / TwoTimesRTimesCeffOverS;
		const Transitions<double> K = 1 - (TwoTimesRTimesCeffOverS) * (1 - (exp(-SOverTwoTimesRTimesCeff)));

		if (!initialized[node.parent])
		{
			nodes[node.parent].effectiveCapacitance.set(node.nodeCapacitance, node.nodeCapacitance);
			initialized[node.parent] = true;
		}

		nodes[node.parent].effectiveCapacitance += K * CTot;

	}
}

void RCTreeWireDelayModel::updateDownstreamCapacitances()
{
	for (int i = 0; i < nodes.size(); i++)
	{
		Node & node = nodes[i];
		node.totalCapacitance = node.nodeCapacitance;
	}
	for (int i = nodes.size() - 1; i > 0; i--)
	{
		Node & node = nodes[i];
		nodes[node.parent].totalCapacitance += node.totalCapacitance;
	}
}

void RCTreeWireDelayModel::initializeEffectiveCapacitances()
{
	for (int i = 0; i < nodes.size(); i++)
	{
		Node & node = nodes[i];
		node.effectiveCapacitance.set(node.totalCapacitance, node.totalCapacitance);
	}
}
