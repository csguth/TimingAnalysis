#include "include/TimingAnalysis.h"

namespace TimingAnalysis
{
/*

	TIMING ANALYSIS

*/

	TimingAnalysis::TimingAnalysis(const CircuitNetList netlist) : nodes(netlist.getGatesSize())
	{
		cout << "criando nodo "<< endl;
		for(size_t i = 0; i < netlist.getGatesSize(); i++)
		{
			const CircuitNetList::LogicGate & gate = netlist.getGateT(i);
			nodes[i] = Node(gate.name, gate.inNets.size(), (i<3?reinterpret_cast<WireDelayModel*>(new RCTreeWireDelayModel(10.0f)):reinterpret_cast<WireDelayModel*>(new LumpedCapacitanceWireDelayModel(2.0f))));
		}
	}

	TimingAnalysis::~TimingAnalysis()
	{

	}

	// GETTERS
	const string TimingAnalysis::getNodeName(const int nodeIndex) const
	{
		return nodes[nodeIndex].name;
	}

	const unsigned TimingAnalysis::getNumberOfNodes() const
	{
		return nodes.size();
	}
	const double TimingAnalysis::simulateRCTree(const int &nodeIndex)
	{
		return nodes[nodeIndex].wireDelayModel->simulate();
	}


/*

	NODE

*/
	Node::Node(const string name, const unsigned inputs, WireDelayModel * wireDelayModel) :	name(name),
															timingPoints(inputs + 1),
															timingArcs(inputs),
															wireDelayModel(wireDelayModel)
	{
		//cout << wireDelayModel->simulate() << endl;
	}
	Node::~Node()
	{
		delete wireDelayModel;
	}

/*

	TIMING POINT

*/

	TimingPoint::TimingPoint() :	arrivalTime(0.0f, 0.0f),
									slew(0.0f, 0.0f)
	{
	}
	TimingPoint::~TimingPoint()
	{

	}

/*

	TIMING ARC

*/
	TimingArc::TimingArc() :	delay(0.0f, 0.0f),
								slew(0.0f, 0.0f)
	{

	}
	TimingArc::~TimingArc()
	{

	}

};