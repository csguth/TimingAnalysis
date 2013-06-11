#include "include/TimingAnalysis.h"

namespace TimingAnalysis
{
/*

	TIMING ANALYSIS

*/

	TimingAnalysis::TimingAnalysis(const CircuitNetList netlist) : nodes(netlist.getGatesSize())
	{
		for(size_t i = 0; i < netlist.getGatesSize(); i++)
		{
			const CircuitNetList::LogicGate & gate = netlist.getGateT(i);
			WireDelayModel * delayModel;
			if(i < 3)
				delayModel = new LumpedCapacitanceWireDelayModel(10.0f);
			else
				delayModel = new RCTreeWireDelayModel(20.0f);
			nodes[i] = Node(gate.name, gate.inNets.size(), delayModel);
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