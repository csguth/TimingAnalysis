#include "include/TimingAnalysis.h"

namespace TimingAnalysis
{
/*

	TIMING ANALYSIS

*/

	TimingAnalysis::TimingAnalysis(const CircuitNetList netlist, const LibertyLibrary * lib, const Parasitics * parasitics, const DesignConstraints * sdc) : nodes(netlist.getGatesSize()), nodesOptions(netlist.getGatesSize()), edges(netlist.getNetsSize()), library(lib), parasitics(parasitics)
	{

		targetDelay = sdc->getClock();

		for(size_t i = 0; i < netlist.getGatesSize(); i++)
		{
			const CircuitNetList::LogicGate & gate = netlist.getGateT(i);
			nodes[i] = Node(gate.name, gate.inNets.size());

			nodes[i].inputDriver = gate.inputDriver;
			nodes[i].sequential = gate.sequential;

			if(gate.inputDriver && !gate.sequential)
			{
				nodes[i].timingArcs.front().slew = sdc->getInputTransition(gate.name);
				nodes[i].timingArcs.front().delay = sdc->getInputDelay(gate.name);
			}

			// cout << "node " << i << " cell type " << gate.cellType << endl;
			const pair<int, int> cellIndex = lib->getCellIndex(gate.cellType);
			nodesOptions[i] = Option(cellIndex.first, cellIndex.second);
			const LibertyCellInfo & cellInfo = lib->getCellInfo(nodesOptions[i].footprintIndex, nodesOptions[i].optionIndex);
			for(size_t j = 1; j < cellInfo.pins.size(); j++) {
				if(cellInfo.pins.at(j).name == "ck")
					nodes[i].timingPoints[j-1].name = gate.name + ":" + cellInfo.pins.at(j+1).name;
				else
					nodes[i].timingPoints[j-1].name = gate.name + ":" + cellInfo.pins.at(j).name;
			}

			if(nodesOptions[i].footprintIndex == 0) // IS PO?
				nodes[i].timingPoints.front().name = nodes[i].name;

			if(!nodes[i].inputDriver)
				nodes[i].timingPoints.back().name = gate.name + ":" + cellInfo.pins.front().name;
			else
				nodes[i].timingPoints.back().name = gate.name;
		}

		interpolator = new LinearLibertyLookupTableInterpolator();
		// cout << "Timing Analysis Option Vector: " << endl;
		for(size_t i = 0; i < netlist.getGatesSize(); i++)
		{
			const LibertyCellInfo & cellInfo = lib->getCellInfo(nodesOptions[i].footprintIndex, nodesOptions[i].optionIndex);
			// cout << "node " << i << "("<< nodes[i].name <<") footprint " << cellInfo.footprint << " cell " << cellInfo.name << endl;
		}


		// Creating the edges and setting their drivers
		for(size_t i = 0; i < netlist.getNetsSize(); i++)
		{
			const CircuitNetList::Net & net = netlist.getNetT(i);
			const int driverTopologicIndex = netlist.getTopologicIndex(net.sourceNode);
			Node * driverNode = 0;
			TimingPoint * driver = 0;
			string rcTreeRootNodeName;

			if(driverTopologicIndex != -1) // TODO Dá pra tirar tudo isso só usando o atributo name de Timing Point
			{
				driverNode = &nodes.at(driverTopologicIndex);
				driver = &driverNode->timingPoints.back();
				const LibertyCellInfo & driverCellInfo = lib->getCellInfo(nodesOptions[driverTopologicIndex].footprintIndex, nodesOptions[driverTopologicIndex].optionIndex);
				rcTreeRootNodeName = driverNode->name;
				if(!driverNode->inputDriver || driverNode->sequential)
				{
					size_t pos = rcTreeRootNodeName.find("_PI");
					if(pos != string::npos)
						rcTreeRootNodeName = rcTreeRootNodeName.substr(0, pos);
					rcTreeRootNodeName += ":";
					rcTreeRootNodeName += driverCellInfo.pins.front().name;
				}
			}
			// Just a test {
			WireDelayModel * delayModel = 0;
			
			if(parasitics->find(net.name) != parasitics->end())
				delayModel = new LumpedCapacitanceWireDelayModel(parasitics->at(net.name).netLumpedCap);
			// 	delayModel = new RCTreeWireDelayModel(parasitics->at(net.name), false, rcTreeRootNodeName);
			// // }

			// cout << "creating edge " << net.name << " with "<< net.sinks.size() << " sinks" << endl;
			edges[i] = Edge(net.name, delayModel, driver, net.sinks.size());
			assert(edges[i].fanouts.empty());
			if(driver)
				driver->net = &edges[i];
		}

		// Now, setting the fanin edges of nodes
		for(size_t i = 0; i < netlist.getGatesSize(); i++)
		{
			const CircuitNetList::LogicGate & gate = netlist.getGateT(i);
			Node & node = nodes[i];
			const LibertyCellInfo & nodeCellInfo = lib->getCellInfo(nodesOptions[i].footprintIndex, nodesOptions[i].optionIndex);
            double pinCapacitance;
            bool PO = false;
            if(nodesOptions[i].footprintIndex == 0)
            {// if PO
                pinCapacitance = sdc->getOutputLoad(node.name);
                PO = true;
            }
			for(size_t j = 0; j < gate.inNets.size(); j++)
			{
				const int inNetTopologicIndex = netlist.getNetTopologicIndex(gate.inNets.at(j));
                const int pinIndex = (node.sequential? j + 2 :j + 1);

                edges[inNetTopologicIndex].addFanout(&node.timingPoints.at(j), (PO?pinCapacitance:nodeCellInfo.pins.at(pinIndex).capacitance) );
				node.timingPoints.at(j).net = &edges[inNetTopologicIndex];
			}
		}

	}

	TimingAnalysis::~TimingAnalysis()
	{

	}


	void TimingAnalysis::fullTimingAnalysis()
	{

		for(size_t i = 0; i < nodes.size(); i++)
		{
			Node & node = nodes.at(i);
			TimingPoint & outTimingPoint = node.timingPoints.back();
			Edge * outEdge = outTimingPoint.net;

            if(outEdge->wireDelayModel == 0x0)
                continue;
			
			const LibertyCellInfo & cellInfo = library->getCellInfo(nodesOptions[i].footprintIndex, nodesOptions[i].optionIndex);
			const string nodeNamePinName = outTimingPoint.name;
			for(size_t k = 0; k < outEdge->fanouts.size(); k++)
			{
				outTimingPoint.arrivalTime = Transitions<double>(numeric_limits<double>::min(), numeric_limits<double>::min());
				outTimingPoint.slew = Transitions<double>(numeric_limits<double>::min(), numeric_limits<double>::min());
			}

			for(size_t j = 0; j < node.timingPoints.size() - 1; j++)
			{
				TimingPoint & tp = node.timingPoints.at(j);
				TimingArc & ta = node.timingArcs.at(j);

				// UPDATING NODE TIMING ARC
				const Transitions<double> ceff = outEdge->wireDelayModel->simulate(cellInfo, j, tp.slew);
                ta.delay = outEdge->wireDelayModel->getDelay(nodeNamePinName);
                ta.slew = outEdge->wireDelayModel->getSlew(nodeNamePinName);


				// UPDATING FANOUT
				for(size_t k = 0; k < outEdge->fanouts.size(); k++)
				{
					TimingPoint * fanoutTimingPoint = outEdge->fanouts.at(k);
                    string fanoutNameAndPin = fanoutTimingPoint->name;
					outTimingPoint.arrivalTime = max(outTimingPoint.arrivalTime, tp.arrivalTime.getReversed() + outEdge->wireDelayModel->getDelay(fanoutNameAndPin)); // NEGATIVE UNATE
					outTimingPoint.slew =  max(outTimingPoint.slew, outEdge->wireDelayModel->getSlew(fanoutNameAndPin));
				}
			}
		}

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
	
	const Transitions<double> TimingAnalysis::getNodeDelay(const int nodeIndex, const int inputNumber, const Transitions<double> transition, const Transitions<double> ceff)
	{
		const LibertyCellInfo & cellInfo = library->getCellInfo(nodesOptions[nodeIndex].footprintIndex, nodesOptions[nodeIndex].optionIndex);
		const LibertyLookupTable fallLUT = cellInfo.timingArcs.at(inputNumber).fallDelay;
		const LibertyLookupTable riseLUT = cellInfo.timingArcs.at(inputNumber).riseDelay;
		return interpolator->interpolate(riseLUT, fallLUT, ceff, transition);
	}


	void TimingAnalysis::printInfo()
	{
		cout << "timing info: " << endl;
		for(size_t i = 0; i < nodes.size(); i++)
		{
			cout << "node "<<i<<" (" << nodes[i].name << ") " << (nodes[i].inputDriver ? " :: input driver :: " : "") << (nodesOptions[i].footprintIndex == 0 || nodes[i].sequential && !nodes[i].inputDriver ? " :: primary output :: " : "") << endl;

			cout << "-- timing arcs" << endl;
			for(size_t j = 0; j < nodes[i].timingArcs.size(); j++)
			{
				cout << "---- " << j << " slew " << nodes[i].timingArcs.at(j).slew << " delay " << nodes[i].timingArcs.at(j).delay << endl;
			}
			cout << "-- timing points" << endl;
			for(size_t j = 0; j < nodes[i].timingPoints.size(); j++)
			{
				cout << "---- " << j << " slew " << nodes[i].timingPoints.at(j).slew << " arrival time " << nodes[i].timingPoints.at(j).arrivalTime << endl;
			}
			cout << endl;
		}
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

	Edge::Edge(const string & netName, WireDelayModel * wireDelayModel, TimingPoint * driver, const int numFanouts) : netName(netName), wireDelayModel(wireDelayModel), driver(driver)
	{
	}
	Edge::~Edge()
	{

	}
	void Edge::addFanout(TimingPoint * fanout, const double pinCapacitance)
	{
		const string fanoutNameAndPin = fanout->name;
		fanouts.push_back(fanout);
        if(wireDelayModel != 0x00)
            wireDelayModel->setFanoutPinCapacitance(fanoutNameAndPin, pinCapacitance);
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
