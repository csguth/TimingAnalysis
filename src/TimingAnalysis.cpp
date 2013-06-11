#include "include/TimingAnalysis.h"

namespace TimingAnalysis
{
/*

	TIMING ANALYSIS

*/

	TimingAnalysis::TimingAnalysis(const CircuitNetList netlist, const LibertyLibrary * lib) : nodes(netlist.getGatesSize()), nodesOptions(netlist.getGatesSize()), library(lib)
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
			cout << "node " << i << " cell type " << gate.cellType << endl;
			const pair<int, int> cellIndex = lib->getCellIndex(gate.cellType);
			nodesOptions[i] = Option(cellIndex.first, cellIndex.second);
			cout << nodes[i] << endl;
		}

		interpolator = new LinearLibertyLookupTableInterpolator();
		cout << "Timing Analysis Option Vector: " << endl;
		for(size_t i = 0; i < netlist.getGatesSize(); i++)
		{
			const LibertyCellInfo & cellInfo = lib->getCellInfo(nodesOptions[i].footprintIndex, nodesOptions[i].optionIndex);
			cout << "node " << i << "("<< nodes[i].name <<") footprint " << cellInfo.footprint << " cell " << cellInfo.name << endl;
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
	const Transitions<double> TimingAnalysis::getNodeDelay(const int nodeIndex, const int inputNumber)
	{
		Transitions<double> ceff(nodes[nodeIndex].wireDelayModel->simulate(),nodes[nodeIndex].wireDelayModel->simulate()); // polimorph call
		Transitions<double> transition(20.0f, 20.0f); // todo
		const LibertyCellInfo & cellInfo = library->getCellInfo(nodesOptions[nodeIndex].footprintIndex, nodesOptions[nodeIndex].optionIndex);
		const LibertyLookupTable fallLUT = cellInfo.timingArcs.at(inputNumber).fallDelay;
		const LibertyLookupTable riseLUT = cellInfo.timingArcs.at(inputNumber).riseDelay;
		return interpolator->interpolate(riseLUT, fallLUT, ceff, transition);
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