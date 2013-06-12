#include "include/TimingAnalysis.h"

namespace TimingAnalysis
{
/*

	TIMING ANALYSIS

*/

	TimingAnalysis::TimingAnalysis(const CircuitNetList netlist, const LibertyLibrary * lib, const Parasitics * parasitics) : nodes(netlist.getGatesSize()), nodesOptions(netlist.getGatesSize()), edges(netlist.getNetsSize()), library(lib), parasitics(parasitics)
	{
		for(size_t i = 0; i < netlist.getGatesSize(); i++)
		{
			const CircuitNetList::LogicGate & gate = netlist.getGateT(i);
			nodes[i] = Node(gate.name, gate.inNets.size());
			nodes[i].inputDriver = gate.inputDriver;
			nodes[i].sequential = gate.sequential;
			cout << "node " << i << " cell type " << gate.cellType << endl;
			const pair<int, int> cellIndex = lib->getCellIndex(gate.cellType);
			nodesOptions[i] = Option(cellIndex.first, cellIndex.second);
		}

		interpolator = new LinearLibertyLookupTableInterpolator();
		cout << "Timing Analysis Option Vector: " << endl;
		for(size_t i = 0; i < netlist.getGatesSize(); i++)
		{
			const LibertyCellInfo & cellInfo = lib->getCellInfo(nodesOptions[i].footprintIndex, nodesOptions[i].optionIndex);
			cout << "node " << i << "("<< nodes[i].name <<") footprint " << cellInfo.footprint << " cell " << cellInfo.name << endl;
		}


		// Creating the edges and setting their drivers
		for(size_t i = 0; i < netlist.getNetsSize(); i++)
		{
			const CircuitNetList::Net & net = netlist.getNetT(i);
			const int driverTopologicIndex = netlist.getTopologicIndex(net.sourceNode);
			Node * driverNode = 0;
			TimingPoint * driver = 0;
			string rcTreeRootNodeName;

			if(driverTopologicIndex != -1)
			{
				driverNode = &nodes.at(driverTopologicIndex);
				driver = &driverNode->timingPoints.back();
				const LibertyCellInfo & driverCellInfo = lib->getCellInfo(nodesOptions[driverTopologicIndex].footprintIndex, nodesOptions[driverTopologicIndex].optionIndex);
				rcTreeRootNodeName = driverNode->name;
				size_t pos = rcTreeRootNodeName.find("_PI");
				if(pos != string::npos)
					rcTreeRootNodeName = rcTreeRootNodeName.substr(0, pos);
				if(!driverNode->inputDriver || driverNode->sequential)
				{
					rcTreeRootNodeName += ":";
					rcTreeRootNodeName += driverCellInfo.pins.front().name;
				}
			}
			// Just a test {
			WireDelayModel * delayModel = 0;
			//delayModel = new LumpedCapacitanceWireDelayModel(10.0f);
			if(parasitics->find(net.name) != parasitics->end())
				delayModel = new RCTreeWireDelayModel(parasitics->at(net.name), false, rcTreeRootNodeName);
			// }
			edges[i] = Edge(net.name, delayModel, driver, net.sinks.size());
			if(driver)
				driver->net = &edges[i];
		}

		// Now, setting the fanin edges of nodes
		for(size_t i = 0; i < netlist.getGatesSize(); i++)
		{
			const CircuitNetList::LogicGate & gate = netlist.getGateT(i);
			Node & node = nodes[i];
			for(size_t j = 0; j < gate.inNets.size(); j++)
			{
				const int inNetTopologicIndex = netlist.getNetTopologicIndex(gate.inNets.at(j));
				
				edges[inNetTopologicIndex].addFanout(&node.timingPoints.at(j));
				
				node.timingPoints.at(j).net = &edges[inNetTopologicIndex];
			}
		}

		// printing topology
		cout << "printing topology: " << endl;
		for(size_t i = 0; i < nodes.size(); i++)
		{
			Node & node = nodes[i];
			const LibertyCellInfo & cellInfo = lib->getCellInfo(nodesOptions[i].footprintIndex, nodesOptions[i].optionIndex);
			cout << "node " << node.name << " op " << cellInfo.name << " ("<<nodesOptions[i].optionIndex <<")" << endl;
			for(size_t j = 0; j < node.timingPoints.size(); j++)
			{
				cout << "-- timing point[" << j << "] driver = " << node.timingPoints.at(j).getNetName() << endl;
			}
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
	
	const Transitions<double> TimingAnalysis::getNodeDelay(const int nodeIndex, const int inputNumber)
	{
		//Transitions<double> ceff(nodes[nodeIndex].wireDelayModel->simulate(),nodes[nodeIndex].wireDelayModel->simulate()); // polimorph call
		Transitions<double> ceff(1.0f, 1.0f);
		Transitions<double> transition(20.0f, 20.0f); // todo
		const LibertyCellInfo & cellInfo = library->getCellInfo(nodesOptions[nodeIndex].footprintIndex, nodesOptions[nodeIndex].optionIndex);
		const LibertyLookupTable fallLUT = cellInfo.timingArcs.at(inputNumber).fallDelay;
		const LibertyLookupTable riseLUT = cellInfo.timingArcs.at(inputNumber).riseDelay;
		return interpolator->interpolate(riseLUT, fallLUT, ceff, transition);
	}



/*

	NODE

*/
	Node::Node(const string name, const unsigned inputs) :	name(name),
															timingPoints(inputs + 1),
															timingArcs(inputs)
	{
		//cout << wireDelayModel->simulate() << endl;
	}
	Node::~Node()
	{
	}

/*

	EDGE

*/

	Edge::Edge(const string & netName, WireDelayModel * wireDelayModel, TimingPoint * driver, const int numFanouts) : netName(netName), wireDelayModel(wireDelayModel), driver(driver), fanouts(numFanouts)
	{

	}
	Edge::~Edge()
	{

	}
	void Edge::addFanout(TimingPoint * fanout)
	{
		fanouts.push_back(fanout);
	}
	const string Edge::getNetName()
	{
		return netName;
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
	const string TimingPoint::getNetName()
	{
		if(!net)
			return "FROM_SOURCE_NET";
		return net->getNetName();
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