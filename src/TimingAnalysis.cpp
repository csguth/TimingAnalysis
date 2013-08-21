#include "include/TimingAnalysis.h"

namespace TimingAnalysis
{
/*

	TIMING ANALYSIS

*/
	const Transitions<double> TimingAnalysis::ZERO_TRANSITIONS(0.0f, 0.0f);
	const Transitions<double> TimingAnalysis::MIN_TRANSITIONS(numeric_limits<double>::min(), numeric_limits<double>::min());
	const Transitions<double> TimingAnalysis::MAX_TRANSITIONS(numeric_limits<double>::max(), numeric_limits<double>::max());
	
	TimingAnalysis::TimingAnalysis(const CircuitNetList netlist, const LibertyLibrary * lib, const Parasitics * parasitics, const DesignConstraints * sdc) : library(lib), parasitics(parasitics), _verilog(netlist.verilog())
	{

		targetDelay = Transitions<double>(sdc->getClock(), sdc->getClock());
		maxTransition = Transitions<double>(lib->getMaxTransition(), lib->getMaxTransition());

		int numberOfTimingPoints, numberOfTimingArcs;
		
		// FORNECER PELA NETLIST (Mais eficiente)
		getNumberOfTimingPointsAndTimingArcs(numberOfTimingPoints, numberOfTimingArcs, netlist, lib); 

		points.reserve(numberOfTimingPoints);
		arcs.reserve(numberOfTimingArcs);
		nets.reserve(netlist.getNetsSize());

		vector<pair<size_t, size_t> > gateIndexToTimingPointIndex(netlist.getGatesSize());

		// Creating Timing Points & Timing Arcs
		for(size_t i = 0; i < netlist.getGatesSize(); i++)
		{
			const CircuitNetList::LogicGate & gate = netlist.getGateT(i);
			// cout << "Gate " << gate.name << " (" << i << ") created!" << endl;
			const pair<int, int> cellIndex = lib->getCellIndex(gate.cellType);
			const LibertyCellInfo & cellInfo = lib->getCellInfo(cellIndex.first, cellIndex.second);
			const bool PI = gate.inputDriver;
			const bool PO = !cellIndex.first || gate.sequential && !PI;
			gateIndexToTimingPointIndex[i] = createTimingPoints(i, gate, cellIndex, cellInfo);
			createTimingArcs(gateIndexToTimingPointIndex[i], PI, PO);

			options.push_back(Option(cellIndex.first, cellIndex.second));
			if(gate.sequential || gate.inputDriver || cellIndex.first == 0 /* PRIMARY OUTPUT*/)
				options.back().dontTouch = true;
			// cout << "   gate option: (" << cellIndex.first << ", " << cellIndex.second << ")" << endl;
			if(options.size() != i+1)
				cout << "error! options.size() ("<< options.size() << ") != " << i+1 << endl;
			assert(options.size() == i + 1);
		}

		// SETTING DESIGN CONSTRAINTS
		// INPUT DELAY && INPUT SLEW
		for(size_t i = 0; i < points.size(); i++)
		{
			const TimingPoint & tp = points.at(i);

			if(tp.isPI())
			{
				const LibertyCellInfo & opt = option(tp.gateNumber);
				if(opt.isSequential)
					continue;
				const string PI_name = tp.name;
				TimingPoint & inPin = points.at(i - 1);
				inPin.arrivalTime = sdc->getInputDelay(PI_name);
				inPin.slew = sdc->getInputTransition(PI_name);

				// cout << "setting input delay " << inPin.arrivalTime << " to pin " << inPin.name << endl;
				// cout << "setting input slew " << inPin.slew << " to pin " << inPin.name << endl;
			}


			if(tp.isPO())
			{
				const LibertyCellInfo & opt = option(tp.gateNumber);
				if(opt.isSequential)
					continue;
				poLoads[i] = sdc->getOutputLoad(tp.name);
				// cout << "setting poLoads["<<i<<"] = " << poLoads.at(i) << endl;
			}
		}

		// Create TimingNets
		for(size_t i = 0; i < netlist.getNetsSize(); i++)
		{
			const CircuitNetList::Net & net = netlist.getNetT(i);
			const int driverTopologicIndex = netlist.getTopologicIndex(net.sourceNode);
			
			if(driverTopologicIndex == -1)
			{
				const string DUMMY_NET_NAME = net.name;
				nets.push_back(TimingNet(DUMMY_NET_NAME, 0, 0));

			}
			else
			{
				const int timingPointIndex = gateIndexToTimingPointIndex.at(driverTopologicIndex).second;
				TimingPoint * driverTp = &points.at(timingPointIndex);

				// Just a test {		
				WireDelayModel * delayModel = 0;

				if(parasitics->find(net.name) != parasitics->end())
				{
					// delayModel = new RCTreeWireDelayModel(parasitics->at(net.name), rcTreeRootNodeName);
					delayModel = new LumpedCapacitanceWireDelayModel(parasitics->at(net.name), driverTp->name);
				}
				// }

				nets.push_back(TimingNet(net.name, driverTp, delayModel));
				driverTp->net = &nets.back();
				// if(delayModel)
				// 	cout << "created net " << nets.back().netName << " with lumped capacitance " << driverTp->load() << endl;
                driverTp->net = &(nets.at(nets.size()-1));
			}
	
		}
		
		// Now, setting the fanin edges of nodes
		for(size_t i = 0; i < netlist.getGatesSize(); i++)
		{
			const CircuitNetList::LogicGate & gate = netlist.getGateT(i);


			// cout << "gate " << gate.name << endl;
			const pair<size_t, size_t> timingPointIndex = gateIndexToTimingPointIndex.at(i);
			for(size_t j = 0; j < gate.inNets.size(); j++)
			{
				const int inNetTopologicIndex = netlist.getNetTopologicIndex(gate.inNets.at(j));
                const int inTimingPointIndex = timingPointIndex.first + j;
                const double pinCap = pinCapacitance(inTimingPointIndex);


                nets.at(inNetTopologicIndex).addFanout(&points.at(inTimingPointIndex));
                if(nets.at(inNetTopologicIndex).wireDelayModel)
                	nets.at(inNetTopologicIndex).wireDelayModel->setFanoutPinCapacitance(points.at(inTimingPointIndex).name, pinCap);
				points.at(inTimingPointIndex).net = &nets.at(inNetTopologicIndex);
			}
		}
		interpolator = new LinearLibertyLookupTableInterpolator();

	}

	const pair<size_t, size_t> TimingAnalysis::createTimingPoints(const int i,const CircuitNetList::LogicGate & gate,const pair<int, int> cellIndex, const LibertyCellInfo & cellInfo)
	{
		const bool is_primary_input = gate.inputDriver;
		const bool SEQUENTIAL = gate.sequential;
		const bool is_primary_output = !cellIndex.first || SEQUENTIAL && !is_primary_input ;

		const size_t firstTimingPointIndex = points.size();
		string gateName = gate.name;
		if( SEQUENTIAL && is_primary_input )
			gateName = gateName.substr(0, gateName.size() - string("_PI").size());

		TimingPointType type;
		string timingPointName;

		// INPUT PINS
		if(!SEQUENTIAL || SEQUENTIAL && is_primary_input )
		{
			for(size_t j = 1; j < cellInfo.pins.size(); j++)
			{	
				if(cellInfo.pins.at(j).name == "ck")
					continue;
				timingPointName = gateName + ":" + cellInfo.pins.at(j).name;
				if(SEQUENTIAL)
					type = REGISTER_INPUT;
				else if (is_primary_input)
					type = PI_INPUT;
				else
					type = INPUT;

				pinNameToTimingPointIndex[timingPointName] = points.size();
				points.push_back(TimingPoint(timingPointName, i, type));
			}
		}


		// OUTPUT PIN
		if( is_primary_input )
		{
			type = PI;
			if( SEQUENTIAL )
				timingPointName = gateName + ":" + cellInfo.pins.front().name;
			else
				timingPointName = gateName;
		}
		else if( is_primary_output )
		{
			type = PO;
			if( SEQUENTIAL )
				timingPointName = gateName + ":" + cellInfo.pins.at(2).name;
			else
				timingPointName = gateName;
		}
		else
		{
			type = OUTPUT;
			timingPointName = gateName + ":" + cellInfo.pins.front().name;
		}

		pinNameToTimingPointIndex[timingPointName] = points.size();
		points.push_back(TimingPoint(timingPointName, i, type));
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

	void TimingAnalysis::createTimingArcs(const pair<size_t, size_t> tpIndexes, const bool is_pi, const bool is_po )
	{
		if( is_pi )
		{
			// cout << "  PI timing arc " << points.at(tpIndexes.first).name << " -> " << points.at(tpIndexes.second).name << endl;
			arcs.push_back(TimingArc(&points.at(tpIndexes.first), &points.at(tpIndexes.second), 0));
			points.at(tpIndexes.first).arc = &arcs.back();
			assert(arcs.back().getFrom() == &points.at(tpIndexes.first) && arcs.back().getTo() == &points.at(tpIndexes.second));
			// cout << "  PI timing arc OK" << endl;
		}
		else
		{
			if( !is_po )
			{
				for(size_t j = tpIndexes.first; j < tpIndexes.second; j++)
				{
					// cout << "  timing arc " << points.at(j).name << " -> " << points.at(tpIndexes.second).name << endl;
					arcs.push_back(TimingArc(&points.at(j), &points.at(tpIndexes.second), j-tpIndexes.first));
					points.at(j).arc = &arcs.back();
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

		slewViolations = ZERO_TRANSITIONS;
		capacitanceViolations = ZERO_TRANSITIONS;
		totalNegativeSlack = ZERO_TRANSITIONS;
		criticalPathValues = ZERO_TRANSITIONS;

		for(size_t i = 0; i < points.size(); i++)
			updateTiming(i);

		for(size_t i = 0; i < points.size(); i++)
		{
			size_t n = points.size() - i - 1;
			updateSlacks(n);
		}

	}

	void TimingAnalysis::updateSlacks(const int i)
	{
		TimingPoint & tp = points.at(i);

		Transitions<double> requiredTime = MAX_TRANSITIONS;

		if( tp.isPIInput() || tp.isRegInput() )
			return;

		if( tp.isInputPin() )
			requiredTime = (tp.arc->getTo()->getRequiredTime() - tp.arc->delay).getReversed();
		else if( tp.isPO() )
			requiredTime = targetDelay;
		else if( tp.isOutputPin() || tp.isPI() )
		{
			for(int i = 0; i < tp.net->fanoutsSize(); i++)
				requiredTime = min(requiredTime, tp.net->getTo(i)->getRequiredTime());
		}
		
		tp.updateSlack(requiredTime);
	}


	void TimingAnalysis::updateTiming(const int i)
	{
		// Dirty bit of a gate output pin
		vector<bool> dirty(options.size(), false); 

		const TimingPoint & tp = points.at(i);
		
		if(tp.isInputPin() || tp.isPIInput() || tp.isRegInput())
		{	
			TimingPoint * outputPin = tp.arc->getTo();
			TimingArc * ta = tp.arc;

			// IF IS THE FIRST INPUT PIN OF A GATE
			// CLEAR OUTPUT PIN
			if( !dirty.at(tp.gateNumber) ) 
			{
				dirty.at(tp.gateNumber) = true;
				outputPin->clearTimingInfo(); 
			}
		
			const LibertyCellInfo & cellInfo = option(tp.gateNumber);
			
			// SETTING ARC DELAY AND SLEW
			const Transitions<double> ceff = outputPin->net->wireDelayModel->simulate(cellInfo, ta->arcNumber, tp.slew);
			ta->delay = outputPin->net->wireDelayModel->getDelay(outputPin->name); // INPUT DRIVER DELAY = DELAY WITH CEFF - DELAY WITH 0 OUTPUT LOAD
			if(tp.isPIInput())
			{
				const Transitions<double> delayWith0OutputLoad = this->getGateDelay(tp.gateNumber, 0, tp.slew, ZERO_TRANSITIONS);
				ta->delay -= delayWith0OutputLoad;
			}
       		ta->slew = outputPin->net->wireDelayModel->getSlew(outputPin->name);
       		
			// SETTING OUTPUT SLEW AND ARRIVAL TIME
			outputPin->arrivalTime = max(outputPin->arrivalTime, tp.arrivalTime.getReversed() + ta->delay); // NEGATIVE UNATE
			outputPin->slew = max(outputPin->slew, ta->slew);

		}
		else if(tp.isOutputPin() || tp.isPI())
		{
			TimingNet * net = tp.net;
			for(int i = 0; i < net->fanoutsSize(); i++)
			{
				TimingPoint * fanoutPin = net->getTo(i);
				fanoutPin->arrivalTime = tp.arrivalTime;
				fanoutPin->slew = tp.slew;
			}
		}
		else if(tp.isPO())
		{
			criticalPathValues = max(criticalPathValues, tp.arrivalTime);
		}
		
	}

	const Transitions<double> TimingAnalysis::getGateDelay(const int gateIndex, const int inputNumber, const Transitions<double> transition, const Transitions<double> ceff)
	{
		const LibertyCellInfo & cellInfo = option(gateIndex);
		const LibertyLookupTable fallLUT = cellInfo.timingArcs.at(inputNumber).fallDelay;
		const LibertyLookupTable riseLUT = cellInfo.timingArcs.at(inputNumber).riseDelay;
		return interpolator->interpolate(riseLUT, fallLUT, ceff, transition);
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

		printf("####################################### CIRCUIT GRAPH INFO ############################################\n");
		printf("| %d Timing Points\n", points.size());
		printf("| %d Timing Arcs\n", arcs.size());
		printf("| %d Timing Nets\n", nets.size());
		printf("-------------------------------------------------------------------------------------------------------\n\n\n");
		
		printf("####################################### CIRCUIT TIMING INFO ###########################################\n");
		printf(">>>> Timing Points Infos (pins)\n");
		queue<int> ports;
		queue<int> sequentials;

		queue<int> pins;
		for(int i = 0 ; i < points.size(); i++)
		{
			const TimingPoint & tp = points.at(i);
			const LibertyCellInfo & cellInfo = option(tp.gateNumber);
		

			if( tp.isPI() && cellInfo.isSequential )
			{
				// cout << tp.name << " PI and sequential" << endl;
				sequentials.push(i);
				continue;
			}

			if (tp.isInputPin() )
			{
				pins.push(i);
				continue;
			}

			if( tp.isOutputPin() || tp.isPO() && cellInfo.isSequential ) 
			{
				printf("%s %f %f %f %f %f %f\n", tp.name.c_str(), tp.slack.getRise(), tp.slack.getFall(), tp.slew.getRise(), tp.slew.getFall(), tp.arrivalTime.getRise(), tp.arrivalTime.getFall());
				while( !pins.empty() )
				{
					const TimingPoint & iPin = points.at(pins.front());
					pins.pop();
					printf("%s %f %f %f %f %f %f\n", iPin.name.c_str(), iPin.slack.getRise(), iPin.slack.getFall(), iPin.slew.getRise(), iPin.slew.getFall(), iPin.arrivalTime.getRise(), iPin.arrivalTime.getFall());	
				}
			}


			if( !cellInfo.isSequential && (tp.isPI() || tp.isPO()) )
				ports.push(i);

			if( tp.isPO() && cellInfo.isSequential )
			{
				const int reg = sequentials.front();
				sequentials.pop();
				const TimingPoint & regTp = points.at(reg);
				printf("%s %f %f %f %f %f %f\n", regTp.name.c_str(), regTp.slack.getRise(), regTp.slack.getFall(), regTp.slew.getRise(), regTp.slew.getFall(), regTp.arrivalTime.getRise(), regTp.arrivalTime.getFall());
			}
		}

		printf("\n>>>> Timing Points Infos (ports)\n");
		while(!ports.empty())
		{
			const int tp_index = ports.front();
			ports.pop();
			const TimingPoint & tp = points.at(tp_index);
			printf("%s %f %f %f %f\n", tp.name.c_str(), tp.slack.getRise(), tp.slack.getFall(), tp.slew.getRise(), tp.slew.getFall());
		}

		printf("-------------------------------------------------------------------------------------------------------\n\n\n");
		
	}

	void TimingAnalysis::printCircuitInfo()
	{
		printf("########################################### TIMING INFO ###############################################\n");
		cout << "| Critical Path Values = " << criticalPathValues << " / " << targetDelay << endl;
		cout << "| Slew Violations = " << slewViolations.aggregate() << endl;
		cout << "| Capacitance Violations = " << capacitanceViolations.aggregate() << endl;
		cout << "| Total Negative Slack = " << totalNegativeSlack.aggregate() << endl;
		printf("-------------------------------------------------------------------------------------------------------\n\n\n");
	}

	// PRIMETIME CALLING
	bool TimingAnalysis::validate_with_prime_time()
	{
		const string ispd_contest_root = getenv("ISPD_CONTEST_ROOT");
		const string ispd_contest_benchmark = getenv("ISPD_CONTEST_BENCHMARK");
		const string sizes_int_file = ispd_contest_root + "/" + ispd_contest_benchmark + "/" + ispd_contest_benchmark + ".int.sizes";
		const string timing_file = ispd_contest_root + "/" + ispd_contest_benchmark + "/" + ispd_contest_benchmark + ".timing";
		const vector<pair<string, string> > sizes_vector = get_sizes_vector();
		const unsigned pollingTime = 1;

		cout << "Running timing analysis" << endl;

		TimerInterface::Status s = TimerInterface::runTimingAnalysisBlocking(sizes_vector, ispd_contest_root, ispd_contest_benchmark, pollingTime);
		cout << "Timing analysis finished with status: " << s << endl;

		return check_timing_file(timing_file);
	}

	void TimingAnalysis::writeSizesFile(const string filename)
	{
		const vector<pair<string, string> > sizes_vector = get_sizes_vector();
		fstream out;
		out.open(filename.c_str(), fstream::out);
		for(int i = 0; i < sizes_vector.size(); i++)
		{
			out << sizes_vector.at(i).first << "\t" << sizes_vector.at(i).second;
			if(i < sizes_vector.size() - 1)
				out << endl;
		}
		out.close();
	}

	const double TimingAnalysis::pinCapacitance(const int timingPointIndex)
	{
		const LibertyCellInfo & opt = option(points.at(timingPointIndex).gateNumber);
        if( points.at(timingPointIndex).isInputPin() )
		{
			const int pinNumber = points.at(timingPointIndex).arc->arcNumber;
			return opt.pins.at(pinNumber+1).capacitance;
		}
		else if ( points.at(timingPointIndex).isPO() )
		{
			if(opt.isSequential)
				return opt.pins.at(2).capacitance;
			return poLoads.at(timingPointIndex);
		}
		// assert(false);
		return -1;
	}

	const vector<pair<string, string> > TimingAnalysis::get_sizes_vector()
	{
		vector<pair<string, string> > sizes(_verilog.size());
		for(int i = 0; i < _verilog.size(); i++)
		{
			const pair<int, string> cell = _verilog.at(i);
			const LibertyCellInfo & opt = option(cell.first);
			sizes[i] = make_pair(cell.second, opt.name);
		}
		return sizes;
	}

	bool TimingAnalysis::check_timing_file(const string timing_file)
	{
		return true;
	}	

	const double TimingPoint::load() const
	{
		return net->wireDelayModel->getLumpedCapacitance();
	}


}



