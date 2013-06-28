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

			if(gate.inputDriver)
			{
				if(!gate.sequential)
				{
					nodes[i].timingPoints.front().slew = sdc->getInputTransition(gate.name);
					nodes[i].timingPoints.front().arrivalTime = sdc->getInputDelay(gate.name);
				}
				else
				{
					nodes[i].timingPoints.front().slew =  Transitions<double>(lib->getMaxTransition(), lib->getMaxTransition());
					nodes[i].timingPoints.front().arrivalTime = Transitions<double>(0, 0);
				}
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
			updateTiming(i);
		}

	}

	void TimingAnalysis::updateTiming(const int i)
	{

		Node & node = nodes.at(i);
		TimingPoint & outTimingPoint = node.timingPoints.back();
		Edge * outEdge = outTimingPoint.net;

        if(outEdge->wireDelayModel == 0x0)
        {
        	// DUMMY NET
            return;
        }
		
		const LibertyCellInfo & cellInfo = library->getCellInfo(nodesOptions[i].footprintIndex, nodesOptions[i].optionIndex);
		const string nodeNamePinName = outTimingPoint.name;

		// RESET OUTPUT SLEW AND ARRIVAL outTimingPoint
		outTimingPoint.arrivalTime = Transitions<double>(numeric_limits<double>::min(), numeric_limits<double>::min());
		outTimingPoint.slew = Transitions<double>(numeric_limits<double>::min(), numeric_limits<double>::min());


		// UPDATE TIMING ARCS
		for(size_t j = 0; j < node.timingArcs.size(); j++)
		{
			TimingPoint & tp = node.timingPoints.at(j);
			TimingArc & ta = node.timingArcs.at(j);

			if(node.inputDriver)
			{
				const Transitions<double> ceff = outEdge->wireDelayModel->simulate(cellInfo, j, tp.slew);
            	const Transitions<double> delayWith0OutputLoad = this->getNodeDelay(i, j, tp.slew, Transitions<double>(0.0f, 0.0f));
            	ta.delay = outEdge->wireDelayModel->getDelay(nodeNamePinName);
            	if(!node.sequential)
            		ta.delay -= delayWith0OutputLoad; // INPUT DRIVER DELAY = DELAY WITH CEFF - DELAY WITH 0 OUTPUT LOAD
            	ta.slew = outEdge->wireDelayModel->getSlew(nodeNamePinName);
            }
            else
            {
            	const Transitions<double> ceff = outEdge->wireDelayModel->simulate(cellInfo, j, tp.slew);
            	ta.delay = outEdge->wireDelayModel->getDelay(nodeNamePinName);
            	ta.slew = outEdge->wireDelayModel->getSlew(nodeNamePinName);	
            }
			outTimingPoint.arrivalTime = max(outTimingPoint.arrivalTime, tp.arrivalTime.getReversed() + ta.delay); // NEGATIVE UNATE
			outTimingPoint.slew =  max(outTimingPoint.slew, ta.slew);

			
		}
		// UPDATING FANOUT
		for(size_t k = 0; k < outEdge->fanouts.size(); k++)
		{
			TimingPoint * fanoutTimingPoint = outEdge->fanouts.at(k);
            string fanoutNameAndPin = fanoutTimingPoint->name;
			fanoutTimingPoint->arrivalTime = max(fanoutTimingPoint->arrivalTime, outTimingPoint.arrivalTime);
			fanoutTimingPoint->slew = max(fanoutTimingPoint->slew, outTimingPoint.slew);
		}
	}

	void TimingAnalysis::setNodeOption(const int nodeIndex, const int optionNumber)
	{
		if(optionNumber == nodesOptions[nodeIndex].optionIndex)
			return; // NOTHING TO DO

		const LibertyCellInfo & cellInfo = library->getCellInfo(nodesOptions[nodeIndex].footprintIndex, nodesOptions[nodeIndex].optionIndex);
		const LibertyCellInfo & newCellInfo = library->getCellInfo(nodesOptions[nodeIndex].footprintIndex, optionNumber);
		Node & node = nodes.at(nodeIndex);
		
		for(size_t i = 0; i < node.timingArcs.size(); i++)
		{
			const double oldPinCapacitance = cellInfo.pins.at(i + 1).capacitance;
			const double newPinCapacitance = newCellInfo.pins.at(i + 1).capacitance;
			const double deltaPinCapacitance = newPinCapacitance - oldPinCapacitance;
			node.timingPoints.at(i).net->wireDelayModel->setFanoutPinCapacitance(node.timingPoints.at(i).name, deltaPinCapacitance);
		}

		nodesOptions[nodeIndex].optionIndex = optionNumber;		
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
		for(size_t i = 0; i < nodes.size(); i++)
		{	
			Node & node = nodes.at(i);
			const bool primaryOutput = !(nodesOptions[i].footprintIndex);
			const LibertyCellInfo & cellInfo = library->getCellInfo(nodesOptions[i].footprintIndex, nodesOptions[i].optionIndex);
			if(primaryOutput || (node.sequential && !node.inputDriver)) // PRIMARY OUTPUT
			{
				TimingPoint & o = node.timingPoints.front();
				cout << "PO "<< o.name << " net " << o.net->netName << " slew = " << o.slew << " arrival time = " << o.arrivalTime << endl;
			}
			else
			{
				TimingPoint & o = node.timingPoints.back();
				if(!node.sequential || (node.sequential && node.inputDriver))
				{
					cout << node.name <<" load = " << o.net->wireDelayModel->simulate(cellInfo, 0, node.timingPoints.front().slew)  << endl;
					cout << "-- "<< o.name << " net " << o.net->netName << " slew = " << o.slew << " arrival time = " << o.arrivalTime << endl;
				}
				if(!node.inputDriver)
				{
					for(size_t j = 0; j < nodes.at(i).timingPoints.size() -1 ; j++)
					{
						TimingPoint & tp = nodes.at(i).timingPoints.at(j);
						cout <<"-- "<< tp.name << " net " << tp.net->netName << " slew = " << tp.slew << " arrival time = " << tp.arrivalTime << endl;
					}
				}
			}
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
