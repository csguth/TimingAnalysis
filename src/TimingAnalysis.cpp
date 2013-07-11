#include "include/TimingAnalysis.h"

namespace TimingAnalysis
{
/*

	TIMING ANALYSIS

*/
	const Transitions<double> TimingAnalysis::ZERO_TRANSITIONS(0.0f, 0.0f);
	const Transitions<double> TimingAnalysis::MIN_TRANSITIONS(numeric_limits<double>::min(), numeric_limits<double>::min());
	const Transitions<double> TimingAnalysis::MAX_TRANSITIONS(numeric_limits<double>::max(), numeric_limits<double>::max());


	
	TimingAnalysis::TimingAnalysis(const CircuitNetList netlist, const LibertyLibrary * lib, const Parasitics * parasitics, const DesignConstraints * sdc) : library(lib), parasitics(parasitics)
	{
		targetDelay = Transitions<double>(sdc->getClock(), sdc->getClock());
		maxTransition = Transitions<double>(lib->getMaxTransition(), lib->getMaxTransition());

		int numberOfTimingPoints, numberOfTimingArcs;
		
		// FORNECER PELA NETLIST (Mais eficiente)
		getNumberOfTimingPointsAndTimingArcs(numberOfTimingPoints, numberOfTimingArcs, netlist, lib); 

		points.reserve(numberOfTimingPoints);
		arcs.reserve(numberOfTimingArcs);

		vector<pair<size_t, size_t> > gateIndexToTimingPointIndex(netlist.getGatesSize());

		// Creating Timing Points & Timing Arcs
		for(size_t i = 0; i < netlist.getGatesSize(); i++)
		{
			const CircuitNetList::LogicGate & gate = netlist.getGateT(i);
			const pair<int, int> cellIndex = lib->getCellIndex(gate.cellType);
			const LibertyCellInfo & cellInfo = lib->getCellInfo(cellIndex.first, cellIndex.second);
			const bool PI = gate.inputDriver;
			const bool PO = !cellIndex.first || gate.sequential && !PI;
			gateIndexToTimingPointIndex[i] = createTimingPoints(i, gate, cellIndex, cellInfo);
			createTimingArcs(gateIndexToTimingPointIndex[i], PI, PO);
		}

		// Create TimingNets
		for(size_t i = 0; i < netlist.getNetsSize(); i++)
		{
			const CircuitNetList::Net & net = netlist.getNetT(i);
			const int driverTopologicIndex = netlist.getTopologicIndex(net.sourceNode);

			if(driverTopologicIndex == -1)
			{
				const string DUMMY_NET_NAME = net.name;
				nets.push_back(TimingNet(DUMMY_NET_NAME, 0));
			}
			else
			{
				const int timingPointIndex = gateIndexToTimingPointIndex.at(driverTopologicIndex).second;
				TimingPoint * driverTp = &points.at(timingPointIndex);
				nets.push_back(TimingNet(net.name, driverTp));
			}
		}
		
		// Now, setting the fanin edges of nodes
		for(size_t i = 0; i < netlist.getGatesSize(); i++)
		{
			const CircuitNetList::LogicGate & gate = netlist.getGateT(i);
			const pair<size_t, size_t> timingPointIndex = gateIndexToTimingPointIndex.at(i);
			for(size_t j = 0; j < gate.inNets.size(); j++)
			{
				const int inNetTopologicIndex = netlist.getNetTopologicIndex(gate.inNets.at(j));
                const int pinIndex = (gate.sequential? j + 2 :j + 1);
                const int inTimingPointIndex = timingPointIndex.first + j;
                nets.at(inNetTopologicIndex).addFanout(&points.at(inTimingPointIndex));
				points.at(inTimingPointIndex).net = &nets.at(inNetTopologicIndex);
			}
		}
		interpolator = new LinearLibertyLookupTableInterpolator();


	}

	const pair<size_t, size_t> TimingAnalysis::createTimingPoints(const int i,const CircuitNetList::LogicGate & gate,const pair<int, int> cellIndex, const LibertyCellInfo & cellInfo)
	{
		const bool PI = gate.inputDriver;
		const bool SEQUENTIAL = gate.sequential;
		const bool PO = !cellIndex.first || SEQUENTIAL && !PI ;

		const size_t firstTimingPointIndex = points.size();
		string gateName = gate.name;
		if( SEQUENTIAL && PI )
			gateName = gateName.substr(0, gateName.size() - string("_PI").size());
	

		if(!SEQUENTIAL || SEQUENTIAL && PI )
		{
			for(size_t j = 1; j < cellInfo.pins.size(); j++)
			{	
				if(cellInfo.pins.at(j).name == "ck")
					continue;
				const string timingPointName = gateName + ":" + cellInfo.pins.at(j).name;
				points.push_back(TimingPoint(timingPointName));
			}
		}
		

		if( PI )
		{
			if( SEQUENTIAL )
			{
				const string timingPointName = gateName + ":" + cellInfo.pins.front().name;
				points.push_back(TimingPoint(timingPointName));
			}
			else
			{
				const string timingPointName = gateName;
				points.push_back(TimingPoint(timingPointName));
			}
		}
		else if( PO )
		{
			if( SEQUENTIAL )
			{
				const string timingPointName = gateName + ":" + cellInfo.pins.at(2).name;
				points.push_back(TimingPoint(timingPointName));
			}
			else
			{
				const string timingPointName = gateName;
				points.push_back(TimingPoint(timingPointName));
			}
		}
		else
		{
			const string timingPointName = gateName + ":" + cellInfo.pins.front().name;
			points.push_back(TimingPoint(timingPointName));
		}

		return make_pair(firstTimingPointIndex, points.size() - 1);
	}

	void TimingAnalysis::getNumberOfTimingPointsAndTimingArcs(int & numberOfTimingPoints, int & numberOfTimingArcs, const CircuitNetList & netlist,const  LibertyLibrary * lib)
	{
		numberOfTimingPoints = 0;
		numberOfTimingArcs = 0;
		for(size_t i = 0; i < netlist.getGatesSize(); i++)
		{
			const CircuitNetList::LogicGate & gate = netlist.getGateT(i);
			const pair<int, int> cellIndex = lib->getCellIndex(gate.cellType);
			const bool PI = gate.inputDriver;
			const bool SEQUENTIAL = gate.sequential;
			const bool PO = !cellIndex.first || SEQUENTIAL && !PI;
			numberOfTimingPoints += 1;
			if( !PO )
			{
				numberOfTimingPoints += gate.inNets.size();
				numberOfTimingArcs += (SEQUENTIAL ? 1 : gate.inNets.size());
			}
		}
	}

	void TimingAnalysis::createTimingArcs(const pair<size_t, size_t> tpIndexes, const bool PI, const bool PO)
	{
		if( PI )
		{
			// cout << "  PI timing arc " << points.at(tpIndexes.first).name << " -> " << points.at(tpIndexes.second).name << endl;
			arcs.push_back(TimingArc(&points.at(tpIndexes.first), &points.at(tpIndexes.second)));
			assert(arcs.back().getFrom() == &points.at(tpIndexes.first) && arcs.back().getTo() == &points.at(tpIndexes.second));
			// cout << "  PI timing arc OK" << endl;
		}
		else
		{
			if( !PO )
			{
				for(size_t j = tpIndexes.first; j < tpIndexes.second; j++)
				{
					// cout << "  timing arc " << points.at(j).name << " -> " << points.at(tpIndexes.second).name << endl;
					arcs.push_back(TimingArc(&points.at(j), &points.at(tpIndexes.second)));
					assert(arcs.back().getFrom() == &points.at(j) && arcs.back().getTo() == &points.at(tpIndexes.second));
					// cout << "  timing arc OK" << endl;
				}
			}
		}
	}

	TimingAnalysis::~TimingAnalysis()
	{

	}


	void TimingAnalysis::fullTimingAnalysis()
	{

		// slewViolations = ZERO_TRANSITIONS;
		// capacitanceViolations = ZERO_TRANSITIONS;
		// totalNegativeSlack = ZERO_TRANSITIONS;
		// criticalPathValues = ZERO_TRANSITIONS;

		// for(size_t i = 0; i < nodes.size(); i++)
		// 	updateTiming(i);

		// for(size_t i = 0; i < nodes.size(); i++)
		// {
		// 	size_t n = nodes.size() - i - 1;
		// 	updateSlacks(n);
		// }		

	}

	void TimingAnalysis::updateSlacks(const int i)
	{
		// Node & node = nodes.at(i);
		// const LibertyCellInfo & cellInfo = library->getCellInfo(nodesOptions[i].footprintIndex, nodesOptions[i].optionIndex);
		// TimingPoint & o = node.timingPoints.back();
		// Edge * outEdge = o.net;

		// if(nodesOptions[i].footprintIndex == 0 || node.sequential && !node.inputDriver) // IS PO OR REGISTER
		// {
		// 	TimingPoint & tp = node.timingPoints.front();
		// 	const Transitions<double> slack = tp.updateSlack(targetDelay);
		// 	criticalPathValues = max(criticalPathValues, tp.arrivalTime);
		// 	totalNegativeSlack -= min(tp.slack, ZERO_TRANSITIONS);
		// 	return;
		// }

		// Transitions<double> requiredTime(numeric_limits<double>::max(), numeric_limits<double>::max());
		
		// for(size_t j = 0; j < outEdge->fanouts.size(); j++)
		// 	requiredTime = min(requiredTime, outEdge->fanouts.at(j)->getRequiredTime());

		// node.timingPoints.back().updateSlack(requiredTime);

		// for(size_t j = 0; j < node.timingArcs.size(); j++)
		// 	node.timingPoints.at(j).updateSlack((requiredTime - node.timingArcs.at(j).delay).getReversed());
			
	}

	void TimingAnalysis::updateTiming(const int i)
	{

		// Node & node = nodes.at(i);
		// TimingPoint & outTimingPoint = node.timingPoints.back();
		// Edge * outEdge = outTimingPoint.net;

  //       if(outEdge->wireDelayModel == 0x0)
  //       {
  //       	// DUMMY NET
  //           return;
  //       }
		
		// const LibertyCellInfo & cellInfo = library->getCellInfo(nodesOptions[i].footprintIndex, nodesOptions[i].optionIndex);
		// const string nodeNamePinName = outTimingPoint.name;

		// // RESET OUTPUT SLEW AND ARRIVAL outTimingPoint
		// outTimingPoint.arrivalTime = MIN_TRANSITIONS;
		// outTimingPoint.slew = MIN_TRANSITIONS;


		// Transitions<double> maxOutputEffectiveCapacitance = ZERO_TRANSITIONS;
		// // UPDATE TIMING ARCS
		// for(size_t j = 0; j < node.timingArcs.size(); j++)
		// {
		// 	TimingPoint & tp = node.timingPoints.at(j);
		// 	TimingArc & ta = node.timingArcs.at(j);

		// 	const Transitions<double> ceff = outEdge->wireDelayModel->simulate(cellInfo, j, tp.slew);

  //           ta.delay = outEdge->wireDelayModel->getDelay(nodeNamePinName);
  //       	if(node.inputDriver && !node.sequential)
  //       	{
		// 		const Transitions<double> delayWith0OutputLoad = this->getNodeDelay(i, j, tp.slew, ZERO_TRANSITIONS);
  //       		ta.delay -= delayWith0OutputLoad; // INPUT DRIVER DELAY = DELAY WITH CEFF - DELAY WITH 0 OUTPUT LOAD
  //       	}
  //       	ta.slew = outEdge->wireDelayModel->getSlew(nodeNamePinName);	
            

               
	 //        slewViolations += max(ta.slew - maxTransition, ZERO_TRANSITIONS);
	 //        maxOutputEffectiveCapacitance = max(maxOutputEffectiveCapacitance, ceff);
		// 	outTimingPoint.arrivalTime = max(outTimingPoint.arrivalTime, tp.arrivalTime.getReversed() + ta.delay); // NEGATIVE UNATE
		// 	outTimingPoint.slew =  max(outTimingPoint.slew, ta.slew);
		// }

		// // cout << "node " << node.name << " max output effective capacitance = " << maxOutputEffectiveCapacitance << ", max capacitance = " << cellInfo.pins.front().maxCapacitance << endl;
		// capacitanceViolations += outEdge->fanouts.size() * max(maxOutputEffectiveCapacitance - cellInfo.pins.front().maxCapacitance, ZERO_TRANSITIONS);
		// // UPDATING FANOUT
		// for(size_t k = 0; k < outEdge->fanouts.size(); k++)
		// {
		// 	TimingPoint * fanoutTimingPoint = outEdge->fanouts.at(k);
  //           string fanoutNameAndPin = fanoutTimingPoint->name;
		// 	fanoutTimingPoint->arrivalTime = max(fanoutTimingPoint->arrivalTime, outTimingPoint.arrivalTime);
		// 	fanoutTimingPoint->slew = max(fanoutTimingPoint->slew, outTimingPoint.slew);


		// }

	}




	void TimingAnalysis::printInfo()
	{
		// for(size_t i = 0; i < nodes.size(); i++)
		// {	
		// 	Node & node = nodes.at(i);
		// 	const bool primaryOutput = !(nodesOptions[i].footprintIndex);
		// 	const LibertyCellInfo & cellInfo = library->getCellInfo(nodesOptions[i].footprintIndex, nodesOptions[i].optionIndex);
		// 	if(primaryOutput || (node.sequential && !node.inputDriver)) // PRIMARY OUTPUT
		// 	{
		// 		TimingPoint & o = node.timingPoints.front();
		// 		cout << "PO "<< o.name << " " << o.slack << " " << o.slew << " " << o.arrivalTime << endl;
		// 	}
		// 	else
		// 	{
		// 		TimingPoint & o = node.timingPoints.back();
		// 		if(!node.sequential || (node.sequential && node.inputDriver))
		// 		{
		// 			cout << node.name << endl;
		// 			cout << "-- "<< o.name << " net " << o.net->netName << " " << o.slack << " " << o.slew << " " << o.arrivalTime << endl;
		// 		}
		// 		if(!node.inputDriver)
		// 		{
		// 			for(size_t j = 0; j < nodes.at(i).timingPoints.size() -1 ; j++)
		// 			{
		// 				TimingPoint & tp = nodes.at(i).timingPoints.at(j);
		// 				cout <<"-- "<< tp.name << " net " << tp.net->netName << tp.slack << " " << tp.slew << " " << tp.arrivalTime << endl;
		// 			}
		// 		}
		// 	}
		// }

		// printCircuitInfo();
	}

	void TimingAnalysis::printCircuitInfo()
	{
		cout << "--" << endl;
		cout << "Critical Path Values = " << criticalPathValues << " / " << targetDelay << endl;
		cout << "Slew Violations = " << slewViolations.aggregate() << endl;
		cout << "Capacitance Violations = " << capacitanceViolations.aggregate() << endl;
		cout << "Total Negative Slack = " << totalNegativeSlack.aggregate() << endl;
	}



	void TimingAnalysis::validateWithPrimeTime()
	{
		
	}
}



