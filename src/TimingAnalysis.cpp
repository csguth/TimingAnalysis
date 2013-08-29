#include "include/TimingAnalysis.h"

namespace TimingAnalysis
{
/*

	TIMING ANALYSIS

*/
	const Transitions<double> TimingAnalysis::ZERO_TRANSITIONS(0.0f, 0.0f);
	const Transitions<double> TimingAnalysis::MIN_TRANSITIONS(numeric_limits<double>::min(), numeric_limits<double>::min());
	const Transitions<double> TimingAnalysis::MAX_TRANSITIONS(numeric_limits<double>::max(), numeric_limits<double>::max());
	
    TimingAnalysis::TimingAnalysis(const CircuitNetList & netlist, const LibertyLibrary * lib, const Parasitics * parasitics, const DesignConstraints * sdc) : _gate_index_to_timing_point_index(netlist.getGatesSize()), _verilog(netlist.verilog()), _library(lib), _parasitics(parasitics)
	{

        _target_delay = Transitions<double>(sdc->getClock(), sdc->getClock());
        _max_transition = Transitions<double>(lib->getMaxTransition(), lib->getMaxTransition());

        int timing_points, timing_arcs;
		
		// FORNECER PELA NETLIST (Mais eficiente)
        number_of_timing_points_and_timing_arcs(timing_points, timing_arcs, netlist, lib);

        _points.reserve(timing_points);
        _arcs.reserve(timing_arcs);
        _nets.reserve(netlist.getNetsSize());

		// Creating Timing Points & Timing Arcs
		for(size_t i = 0; i < netlist.getGatesSize(); i++)
		{
			const CircuitNetList::LogicGate & gate = netlist.getGateT(i);
			// cout << "Gate " << gate.name << " (" << i << ") created!" << endl;
            const pair<int, int> cell_index = lib->getCellIndex(gate.cellType);
            const LibertyCellInfo & cell_info = lib->getCellInfo(cell_index.first, cell_index.second);
            const bool is_PI = gate.inputDriver;
            const bool is_PO = !cell_index.first || ( gate.sequential && !is_PI );
            _gate_index_to_timing_point_index[i] = create_timing_points(i, gate, cell_index, cell_info);
            create_timing_arcs(_gate_index_to_timing_point_index[i], is_PI, is_PO);

            _options.push_back(Option(cell_index.first, cell_index.second));
            if(gate.sequential || gate.inputDriver || cell_index.first == 0 /* PRIMARY OUTPUT*/)
                _options.back().dont_touch = true;
			// cout << "   gate option: (" << cellIndex.first << ", " << cellIndex.second << ")" << endl;
            if(_options.size() != i+1)
                cout << "error! options.size() ("<< _options.size() << ") != " << i+1 << endl;
            assert(_options.size() == i + 1);
        }

        _dirty.resize(_options.size());

		// SETTING DESIGN CONSTRAINTS
		// INPUT DELAY && INPUT SLEW
        for(size_t i = 0; i < _points.size(); i++)
		{
            const TimingPoint & tp = _points.at(i);

            if(tp.is_PI())
			{
                const LibertyCellInfo & opt = option(tp._gate_number);
				if(opt.isSequential)
					continue;
                const string PI_name = tp._name;
                TimingPoint & inPin = _points.at(i - 1);
                inPin._arrival_time = sdc->getInputDelay(PI_name);
                inPin._slew = sdc->getInputTransition(PI_name);

				// cout << "setting input delay " << inPin.arrivalTime << " to pin " << inPin.name << endl;
				// cout << "setting input slew " << inPin.slew << " to pin " << inPin.name << endl;
			}


            if(tp.is_PO())
			{

                const LibertyCellInfo & opt = option(tp._gate_number);
				if(opt.isSequential)
					continue;
                _PO_loads[i] = sdc->getOutputLoad(tp._name);
				// cout << "setting poLoads["<<i<<"] = " << poLoads.at(i) << endl;
			}
		}

		// Create TimingNets
		for(size_t i = 0; i < netlist.getNetsSize(); i++)
		{
			const CircuitNetList::Net & net = netlist.getNetT(i);
            const int driverTopologicIndex = netlist.getTopologicIndex(net.sourceNode); // -1 if driver is the 'source'
			
            if(driverTopologicIndex == -1)
			{
                const string dummy_net_name = net.name;
                _nets.push_back(TimingNet(dummy_net_name, 0, 0));

			}
			else
			{
                const int timing_point_index = _gate_index_to_timing_point_index.at(driverTopologicIndex).second;
                TimingPoint & driver_timing_point = _points.at(timing_point_index);

				// Just a test {		
                WireDelayModel * delay_model = 0;

				if(parasitics->find(net.name) != parasitics->end())
				{
					// delayModel = new RCTreeWireDelayModel(parasitics->at(net.name), rcTreeRootNodeName);
                    delay_model = new LumpedCapacitanceWireDelayModel(parasitics->at(net.name), driver_timing_point._name);
				}
				// }

                _nets.push_back(TimingNet(net.name, &driver_timing_point, delay_model));

                if(_nets.size() == 2)
                    cout << "&nets[1] = " << &_nets.at(1) << endl;
				// if(delayModel)
				// 	cout << "created net " << nets.back().netName << " with lumped capacitance " << driverTp->load() << endl;
                driver_timing_point._net = &(_nets.back());
			}
	
        }
		
		// Now, setting the fanin edges of nodes
		for(size_t i = 0; i < netlist.getGatesSize(); i++)
		{
			const CircuitNetList::LogicGate & gate = netlist.getGateT(i);

			// cout << "gate " << gate.name << endl;
            const pair<size_t, size_t> timing_point_index = _gate_index_to_timing_point_index.at(i);
			for(size_t j = 0; j < gate.inNets.size(); j++)
			{
                const int in_net_topologic_index = netlist.get_net_topologic_index(gate.inNets.at(j));
                TimingNet * in_net = &_nets.at(in_net_topologic_index);

                if(in_net->_name == "n_885")
                    cout << endl;

                const int in_timing_point_index = timing_point_index.first + j;
                TimingPoint & fanout_timing_point = _points.at(in_timing_point_index);

                in_net->add_fanout(&fanout_timing_point);
                if(in_net->_wire_delay_model)
                {
                    if(in_net->_name == "n_885")
                        cout << endl;
                    const double pin_cap = pin_capacitance(in_timing_point_index);
                    in_net->_wire_delay_model->setFanoutPinCapacitance(fanout_timing_point._name, pin_cap);
                }
                fanout_timing_point._net = in_net;
			}
		}
        _interpolator = new LinearLibertyLookupTableInterpolator();

        cout << "points address: " << &_points[0] << endl;
        cout << "arcs address: " << &_arcs[0] << endl;
        cout << "nets address: " << &_nets[0] << endl;
        cout << "&nets[1] = " << &_nets.at(1) << endl;

	}

    const pair<size_t, size_t> TimingAnalysis::create_timing_points(const int i,const CircuitNetList::LogicGate & gate,const pair<int, int> cellIndex, const LibertyCellInfo & cellInfo)
	{
		const bool is_primary_input = gate.inputDriver;
        const bool is_sequential = gate.sequential;
        const bool is_primary_output = !cellIndex.first || ( is_sequential && !is_primary_input );

        const size_t firstTimingPointIndex = _points.size();
		string gateName = gate.name;
        if( is_sequential && is_primary_input )
			gateName = gateName.substr(0, gateName.size() - string("_PI").size());

		TimingPointType type;
		string timingPointName;

		// INPUT PINS
        if(!is_sequential || (is_sequential && is_primary_input ) )
		{
			for(size_t j = 1; j < cellInfo.pins.size(); j++)
			{	
				if(cellInfo.pins.at(j).name == "ck")
					continue;
				timingPointName = gateName + ":" + cellInfo.pins.at(j).name;
                if(is_sequential)
					type = REGISTER_INPUT;
				else if (is_primary_input)
					type = PI_INPUT;
				else
					type = INPUT;

                _pin_name_to_timing_point_index[timingPointName] = _points.size();
                _points.push_back(TimingPoint(timingPointName, i, type));
			}
		}


		// OUTPUT PIN
		if( is_primary_input )
		{
			type = PI;
            if( is_sequential )
				timingPointName = gateName + ":" + cellInfo.pins.front().name;
			else
				timingPointName = gateName;
		}
		else if( is_primary_output )
		{
			type = PO;
            if( is_sequential )
				timingPointName = gateName + ":" + cellInfo.pins.at(2).name;
			else
				timingPointName = gateName;
		}
		else
		{
			type = OUTPUT;
			timingPointName = gateName + ":" + cellInfo.pins.front().name;
		}

        _pin_name_to_timing_point_index[timingPointName] = _points.size();
        _points.push_back(TimingPoint(timingPointName, i, type));
        return make_pair(firstTimingPointIndex, _points.size() - 1);
	}

    void TimingAnalysis::number_of_timing_points_and_timing_arcs(int & numberOfTimingPoints, int & numberOfTimingArcs, const CircuitNetList & netlist,const  LibertyLibrary * lib)
	{
		numberOfTimingPoints = 0;
		numberOfTimingArcs = 0;
		for(size_t i = 0; i < netlist.getGatesSize(); i++)
		{
			const CircuitNetList::LogicGate & gate = netlist.getGateT(i);
			const pair<int, int> cellIndex = lib->getCellIndex(gate.cellType);
            const bool is_PI = gate.inputDriver;
            const bool is_sequential = gate.sequential;
            const bool is_PO = !cellIndex.first || (is_sequential && !is_PI);
			numberOfTimingPoints += 1;
            if( !is_PO )
			{
				numberOfTimingPoints += gate.inNets.size();
                numberOfTimingArcs += (is_sequential ? 1 : gate.inNets.size());
			}
		}
	}

    void TimingAnalysis::create_timing_arcs(const pair<size_t, size_t> tpIndexes, const bool is_pi, const bool is_po )
	{
		if( is_pi )
		{
			// cout << "  PI timing arc " << points.at(tpIndexes.first).name << " -> " << points.at(tpIndexes.second).name << endl;
            _arcs.push_back(TimingArc(&_points.at(tpIndexes.first), &_points.at(tpIndexes.second), 0));
            _points.at(tpIndexes.first).arc = &_arcs.back();
            assert(_arcs.back().from() == &_points.at(tpIndexes.first) && _arcs.back().to() == &_points.at(tpIndexes.second));
			// cout << "  PI timing arc OK" << endl;
		}
		else
		{
			if( !is_po )
			{
				for(size_t j = tpIndexes.first; j < tpIndexes.second; j++)
				{
					// cout << "  timing arc " << points.at(j).name << " -> " << points.at(tpIndexes.second).name << endl;
                    _arcs.push_back(TimingArc(&_points.at(j), &_points.at(tpIndexes.second), j-tpIndexes.first));
                    _points.at(j).arc = &_arcs.back();
                    assert(_arcs.back().from() == &_points.at(j) && _arcs.back().to() == &_points.at(tpIndexes.second));
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
        for (size_t i = 0; i < _dirty.size(); ++i) {
            _dirty[i] = false;
        }

        _slew_violations = ZERO_TRANSITIONS;
        _capacitance_violations = ZERO_TRANSITIONS;
        _total_negative_slack = ZERO_TRANSITIONS;
        _critical_path = ZERO_TRANSITIONS;

        for(size_t i = 0; i < _points.size(); i++)
            update_timing(i);

        for(size_t i = 0; i < _points.size(); i++)
        {
            size_t n = _points.size() - i - 1;
            update_slacks(n);
        }

    }

    bool TimingAnalysis::gate_option(const int gate_index, const int option_number)
    {
        const LibertyCellInfo & old_cell_info = option(gate_index);
        const Option & gate_option = _options.at(gate_index);
        if( !gate_option.dont_touch )
        {
            _options.at(gate_index).option_index = option_number;
            const LibertyCellInfo & new_cell_info = option(gate_index);

            // UPDATE FANINS OUTPUT LOADS
            const pair<size_t, size_t> timing_points = _gate_index_to_timing_point_index.at(gate_index);
            for(size_t timing_point_index = timing_points.first; timing_point_index <= timing_points.second; timing_point_index++)
            {
                const TimingPoint & timing_point = _points.at(timing_point_index);

                if(timing_point.is_input_pin())
                {
                    // IF SEQUENTIAL, INPUT PIN NUMBER = 2
                    const int pin_number = (old_cell_info.isSequential ? 2 : timing_point.arc->arc_number() + 1);
                    const double old_pin_capacitance = old_cell_info.pins.at(pin_number).capacitance;
                    const double new_pin_capacitance = new_cell_info.pins.at(pin_number).capacitance;
                    TimingNet * in_net = timing_point._net;
                    if(in_net->_wire_delay_model)
                        in_net->_wire_delay_model->setFanoutPinCapacitance(timing_point._name, new_pin_capacitance - old_pin_capacitance);
                }
            }

            return true;
        }
        return false;
    }

    void TimingAnalysis::update_slacks(const int timing_point_index)
	{
        TimingPoint & timing_point = _points.at(timing_point_index);
        TimingNet * net = timing_point._net;

        Transitions<double> required_time = MAX_TRANSITIONS;

        if( timing_point.is_PI_input() || timing_point.is_reg_input() )
			return;
        if( timing_point.is_input_pin() ) //
            required_time = (timing_point.arc->to()->required_time() - timing_point.arc->_delay).getReversed();
        else if( timing_point.is_PO() )
            required_time = _target_delay;
        else if( timing_point.is_output_pin() || timing_point.is_PI() )
		{
            for(size_t i = 0; i < net->fanouts_size(); i++)
                required_time = min(required_time, net->to(i)->required_time());
		}

        timing_point.update_slack(required_time);

        if(timing_point.is_PO())
            _total_negative_slack -= min(ZERO_TRANSITIONS, timing_point._slack);
	}


    void TimingAnalysis::update_timing(const int timing_point_index)
	{
        const TimingPoint & timing_point = _points.at(timing_point_index);
        if(timing_point._name == "i_rx_phy_fs_ce_reg_u0:o" || timing_point._name == "i_rx_phy_fs_ce_reg_u0:d")
            cout << endl;
		
        if(timing_point.is_input_pin() || timing_point.is_PI_input() || timing_point.is_reg_input())
		{	
            TimingPoint * output_pin = timing_point.arc->to();
            TimingArc * timing_arc = timing_point.arc;
            TimingNet * output_net = output_pin->_net;
            const bool first = !_dirty.at(output_pin->_gate_number);
            const LibertyCellInfo & cell_info = option(timing_point._gate_number);

			// IF IS THE FIRST INPUT PIN OF A GATE
			// CLEAR OUTPUT PIN
			if( first ) 
			{
                _dirty[output_pin->_gate_number] = true;
                output_pin->clear_timing_info();
			}
		
			// SETTING ARC DELAY AND SLEW
            const Transitions<double> ceff = output_net->_wire_delay_model->simulate(cell_info, timing_arc->_arc_number, timing_point._slew);

            timing_arc->_delay = output_net->_wire_delay_model->getDelay(output_pin->_name); // INPUT DRIVER DELAY = DELAY WITH CEFF - DELAY WITH 0 OUTPUT LOAD

            if(timing_point.is_PI_input())
			{
                const Transitions<double> delay_with_0_output_load = this->calculate_gate_delay(timing_point._gate_number, 0, timing_point._slew, ZERO_TRANSITIONS);
                timing_arc->_delay -= delay_with_0_output_load;
			}

            timing_arc->_slew = output_net->_wire_delay_model->getSlew(output_pin->_name);

			// SETTING OUTPUT SLEW AND ARRIVAL TIME
            output_pin->_arrival_time = max(output_pin->_arrival_time, timing_point._arrival_time.getReversed() + timing_arc->_delay); // NEGATIVE UNATE
            output_pin->_slew = max(output_pin->_slew, timing_arc->_slew);

			// SETTING OUTPUT PIN VIOLATIONS
			if( first )
			{
                const LibertyCellInfo & cell_info = option(output_pin->_gate_number);
                _capacitance_violations += max(ZERO_TRANSITIONS, (ceff - cell_info.pins.front().maxCapacitance));
			}

		}
        else if(timing_point.is_output_pin() || timing_point.is_PI())
		{
            const TimingNet * net = timing_point._net;
            for(size_t i = 0; i < net->fanouts_size(); i++)
			{
                TimingPoint * fanoutPin = net->to(i);
                fanoutPin->_arrival_time = timing_point._arrival_time;
                fanoutPin->_slew = timing_point._slew;
			}
		}
        else if(timing_point.is_PO())
        {
            _critical_path = max(_critical_path, timing_point._arrival_time);
        }
		
	}

    const Transitions<double> TimingAnalysis::calculate_gate_delay(const int gate_index, const int input_number, const Transitions<double> transition, const Transitions<double> ceff)
	{
        const LibertyCellInfo & cellInfo = option(gate_index);
        const LibertyLookupTable fallLUT = cellInfo.timingArcs.at(input_number).fallDelay;
        const LibertyLookupTable riseLUT = cellInfo.timingArcs.at(input_number).riseDelay;
        return _interpolator->interpolate(riseLUT, fallLUT, ceff, transition);
    }

    const LibertyCellInfo &TimingAnalysis::option(const int node_index) const
    {
        return _library->getCellInfo(_options.at(node_index).footprint_index, _options.at(node_index).option_index);
    }

    void TimingAnalysis::print_info()
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
        printf("| %u Timing Points\n", unsigned(_points.size()));
        printf("| %u Timing Arcs\n", unsigned(_arcs.size()));
        printf("| %u Timing Nets\n", unsigned(_nets.size()));
		printf("-------------------------------------------------------------------------------------------------------\n\n\n");
		
		printf("####################################### CIRCUIT TIMING INFO ###########################################\n");
		printf(">>>> Timing Points Infos (pins)\n");
		queue<int> ports;
		queue<int> sequentials;

		queue<int> pins;
        for(size_t i = 0 ; i < _points.size(); i++)
		{
            const TimingPoint & tp = _points.at(i);
            const LibertyCellInfo & cellInfo = option(tp._gate_number);

            if( tp.is_PI() && cellInfo.isSequential )
			{
				// cout << tp.name << " PI and sequential" << endl;
				sequentials.push(i);
				continue;
			}

            if (tp.is_input_pin() )
			{

				pins.push(i);
				continue;
			}

            if( tp.is_output_pin() || (tp.is_PO() && cellInfo.isSequential))
			{
                printf("%s %f %f %f %f %f %f\n", tp._name.c_str(), tp._slack.getRise(), tp._slack.getFall(), tp._slew.getRise(), tp._slew.getFall(), tp._arrival_time.getRise(), tp._arrival_time.getFall());
				while( !pins.empty() )
				{
                    const TimingPoint & iPin = _points.at(pins.front());
					pins.pop();
                    printf("%s %f %f %f %f %f %f\n", iPin._name.c_str(), iPin._slack.getRise(), iPin._slack.getFall(), iPin._slew.getRise(), iPin._slew.getFall(), iPin._arrival_time.getRise(), iPin._arrival_time.getFall());
				}
			}


            if( !cellInfo.isSequential && (tp.is_PI() || tp.is_PO()) )
				ports.push(i);

            if( tp.is_PO() && cellInfo.isSequential )
			{
				const int reg = sequentials.front();
				sequentials.pop();
                const TimingPoint & regTp = _points.at(reg);
                printf("%s %f %f %f %f %f %f\n", regTp._name.c_str(), regTp._slack.getRise(), regTp._slack.getFall(), regTp._slew.getRise(), regTp._slew.getFall(), regTp._arrival_time.getRise(), regTp._arrival_time.getFall());
			}
		}

		printf("\n>>>> Timing Points Infos (ports)\n");
		while(!ports.empty())
		{
			const int tp_index = ports.front();
			ports.pop();
            const TimingPoint & tp = _points.at(tp_index);
            printf("%s %f %f %f %f\n", tp._name.c_str(), tp._slack.getRise(), tp._slack.getFall(), tp._slew.getRise(), tp._slew.getFall());
		}

		printf("-------------------------------------------------------------------------------------------------------\n\n\n");
		
	}

    void TimingAnalysis::print_circuit_info()
	{
		printf("########################################### TIMING INFO ###############################################\n");
        cout << "| Critical Path Values = " << _critical_path << " / " << _target_delay << endl;
        cout << "| Slew Violations = " << _slew_violations.aggregate() << endl;
        cout << "| Capacitance Violations = " << _capacitance_violations.aggregate() << endl;
        cout << "| Total Negative Slack = " << _total_negative_slack.aggregate() << endl;
		printf("-------------------------------------------------------------------------------------------------------\n\n\n");
	}

	// PRIMETIME CALLING
	bool TimingAnalysis::validate_with_prime_time()
	{
        const string timing_file = Traits::ispd_contest_root + "/" + Traits::ispd_contest_benchmark + "/" + Traits::ispd_contest_benchmark + ".timing";
		const vector<pair<string, string> > sizes_vector = get_sizes_vector();
		const unsigned pollingTime = 1;

		cout << "Running timing analysis" << endl;

        TimerInterface::Status s = TimerInterface::runTimingAnalysisBlocking(sizes_vector,  Traits::ispd_contest_root, Traits::ispd_contest_benchmark, pollingTime);
		cout << "Timing analysis finished with status: " << s << endl;

		return check_timing_file(timing_file);
	}

    void TimingAnalysis::write_sizes_file(const string filename)
	{
		const vector<pair<string, string> > sizes_vector = get_sizes_vector();
		fstream out;
		out.open(filename.c_str(), fstream::out);
        for(size_t i = 0; i < sizes_vector.size(); i++)
		{
			out << sizes_vector.at(i).first << "\t" << sizes_vector.at(i).second;
			if(i < sizes_vector.size() - 1)
				out << endl;
		}
		out.close();
	}

    double TimingAnalysis::pin_capacitance(const int timing_point_index) const
	{
        const LibertyCellInfo & opt = option(_points.at(timing_point_index)._gate_number);
        const TimingPoint & timing_point = _points.at(timing_point_index);
        if( timing_point.is_input_pin() || timing_point.is_PI_input() || timing_point.is_reg_input() )
		{
            const int pin_number = _points.at(timing_point_index).arc->_arc_number;
            return opt.pins.at(pin_number+1).capacitance;
		}
        else if ( _points.at(timing_point_index).is_PO() )
		{
			if(opt.isSequential)
				return opt.pins.at(2).capacitance;
            return _PO_loads.at(timing_point_index);
		}
        assert(false);
        return -1;
    }

    const Option &TimingAnalysis::gate_option(const int gate_index)
    {
        return _options.at(gate_index);
    }

	const vector<pair<string, string> > TimingAnalysis::get_sizes_vector()
	{
		vector<pair<string, string> > sizes(_verilog.size());
        for(size_t i = 0; i < _verilog.size(); i++)
		{
			const pair<int, string> cell = _verilog.at(i);
			const LibertyCellInfo & opt = option(cell.first);
			sizes[i] = make_pair(cell.second, opt.name);
		}
		return sizes;
	}

	bool TimingAnalysis::check_timing_file(const string timing_file)
	{
		Prime_Time_Output_Parser prime_time_parser;
		const Prime_Time_Output_Parser::Prime_Time_Output prime_time_output = prime_time_parser.parse_prime_time_output_file(timing_file);


        unsigned first_wrong_node_slack = numeric_limits<unsigned>::max();
        unsigned first_wrong_node_slew = numeric_limits<unsigned>::max();
        unsigned first_wrong_node_arrival = numeric_limits<unsigned>::max();
        bool slack_fail = false;
        bool slew_fail = false;
        bool arrival_time_fail = false;

		cout << "pin timing (total = " << prime_time_output.pins_size() << ")" << endl;
		for(size_t i = 0; i < prime_time_output.pins_size(); i++)
		{
			const Prime_Time_Output_Parser::Pin_Timing pin_timing = prime_time_output.pin(i);
            const TimingPoint & tp = _points.at(_pin_name_to_timing_point_index.at(pin_timing.pin_name));

            Transitions<double> slack_error(tp._slack-pin_timing.slack);
            Transitions<double> slew_error(tp._slew-pin_timing.slew);
            Transitions<double> arrival_time_error(tp._arrival_time-pin_timing.arrival_time);

            slack_error = Transitions<double>(fabs(slack_error.getRise()), fabs(slack_error.getFall()));
            slew_error = Transitions<double>(fabs(slew_error.getRise()), fabs(slew_error.getFall()));
            arrival_time_error = Transitions<double>(fabs(arrival_time_error.getRise()), fabs(arrival_time_error.getFall()));

            bool slack_diff = slack_error.getFall() >= Traits::STD_THRESHOLD || slack_error.getRise() >= Traits::STD_THRESHOLD;
            bool slew_diff = slew_error.getFall() >= Traits::STD_THRESHOLD || slew_error.getRise() >= Traits::STD_THRESHOLD;
            bool arrival_time_diff = arrival_time_error.getFall() >= Traits::STD_THRESHOLD || arrival_time_error.getRise() >= Traits::STD_THRESHOLD;

            if(slack_diff)
            {
                if(_pin_name_to_timing_point_index.at(pin_timing.pin_name) < first_wrong_node_slack)
                {
                    first_wrong_node_slack = _pin_name_to_timing_point_index.at(pin_timing.pin_name);
                    slack_fail = slack_diff;
                }
            }

            if(slew_diff)
            {
                if(_pin_name_to_timing_point_index.at(pin_timing.pin_name) < first_wrong_node_slew)
                {
                    first_wrong_node_slew = _pin_name_to_timing_point_index.at(pin_timing.pin_name);
                    slew_fail = slew_diff;
                }
            }


            if(arrival_time_diff)
            {
                if(_pin_name_to_timing_point_index.at(pin_timing.pin_name) < first_wrong_node_arrival)
                {
                    first_wrong_node_arrival = _pin_name_to_timing_point_index.at(pin_timing.pin_name);
                    arrival_time_fail = arrival_time_diff;
                }
            }

            cout << "pin " << pin_timing.pin_name << " index " << _pin_name_to_timing_point_index.at(pin_timing.pin_name) << endl;
            cout << "  slack = " << tp._slack << " prime_time_slack = " << pin_timing.slack;
            if(slack_diff)
                cout << " error " << slack_error;
            cout << endl;
            cout << "  slew = " << tp._slew << " prime_time_slew = " << pin_timing.slew;
            if(slew_diff)
                cout << " error " << slew_error;
            cout << endl;
            cout << "  arrival_time = " << tp._arrival_time << " prime_time_arrival_time = " << pin_timing.arrival_time;
            if(arrival_time_diff)
                cout << " error " << arrival_time_error;
            cout << endl;

//            assert(fabs(tp.slew.getRise() - pin_timing.slew.getRise()) < Traits::STD_THRESHOLD);
//            assert(fabs(tp.slew.getFall() - pin_timing.slew.getFall()) < Traits::STD_THRESHOLD);
//            assert(fabs(tp.arrivalTime.getRise() - pin_timing.arrival_time.getRise()) < Traits::STD_THRESHOLD);
//            assert(fabs(tp.arrivalTime.getFall() - pin_timing.arrival_time.getFall()) < Traits::STD_THRESHOLD);
		}
		cout << "port timing (total = " << prime_time_output.ports_size() << ")" << endl;
		for(size_t i = 0; i < prime_time_output.ports_size(); i++)
		{
			const Prime_Time_Output_Parser::Port_Timing port_timing = prime_time_output.port(i);
            const TimingPoint & tp = _points.at(_pin_name_to_timing_point_index.at(port_timing.port_name));

            cout << "port " << port_timing.port_name << " index " << _pin_name_to_timing_point_index.at(port_timing.port_name) << endl;
            cout << "  slack = " << tp._slack << " prime_time_slack = " << port_timing.slack << endl;
            cout << "  slew = " << tp._slew << " prime_time_slew = " << port_timing.slew << endl;

//            assert(fabs(tp.slew.getRise() - port_timing.slew.getRise()) < Traits::STD_THRESHOLD);
//            assert(fabs(tp.slew.getFall() - port_timing.slew.getFall()) < Traits::STD_THRESHOLD);
		}


        cout << "#############" << endl;
        if(first_wrong_node_slack < _points.size())
        {
            cout << "slack = " << _points.at(first_wrong_node_slack)._name << endl;
            cout << "  slack fail = " << slack_fail << endl;
        }
        if(first_wrong_node_slew < _points.size())
        {
            cout << "slew = " << _points.at(first_wrong_node_slew)._name << endl;
            cout << "  slew fail = " << slew_fail << endl;
        }
        if(first_wrong_node_arrival < _points.size())
        {
            cout << "arrival = " << _points.at(first_wrong_node_arrival)._name << endl;
            cout << "  arrival fail = " << arrival_time_fail << endl;
        }

        if(first_wrong_node_slack >= _points.size() && first_wrong_node_slew >= _points.size() && first_wrong_node_arrival >= _points.size())
            cout << "ALL OK" << endl;
        cout << "###########" << endl;


        return true;
    }

    TimingPoint::TimingPoint(std::string name, const int gate_number, TimingPointType type): _name(name), _net(0), arc(0), _slack(0.0f, 0.0f), _slew(0.0f, 0.0f), _arrival_time(0.0f, 0.0f), _gate_number(gate_number), _type(type)
    {
        if(type == REGISTER_INPUT)
            _slew = Transitions<double>(80.0f, 80.0f);
    }

    const Transitions<double> TimingPoint::update_slack(const Transitions<double> required_time)
    {
        _slack = required_time - _arrival_time;
        return _slack;
    }

    void TimingPoint::clear_timing_info()
    {
        _slack = Transitions<double>(0.0f, 0.0f);
        _slew = Transitions<double>(numeric_limits<double>::min(), numeric_limits<double>::min());
        _arrival_time = Transitions<double>(numeric_limits<double>::min(), numeric_limits<double>::min());
    }

    std::ostream & operator<<(std::ostream &out, const TimingPoint &tp)
    {
        return out << tp._name << " slack " << tp._slack << " slew " << tp._slew << " arrival " << tp._arrival_time;
    }

    double TimingPoint::load() const
    {
        return _net->_wire_delay_model->lumped_capacitance();
    }

    void TimingNet::add_fanout(TimingPoint *tp)
    {
        _add_fanout(tp);
    }

    const std::string TimingNet::name() const
    {
        return _name;
    }

    std::ostream & operator<<(std::ostream &out, const TimingNet &tn)
    {
        out << tn._name << " " << (tn.from() != 0 ? tn.from()->name() : "source") << " -> (";
        for(size_t i = 0; i < tn.fanouts_size(); i++)
            out << (tn.to(i) != 0 ? tn.to(i)->name() : "sink") << " ";
        out << ")";
        return out;
    }

    void Edge::_add_fanout(TimingPoint *tp)
    {
        _to.push_back(tp);
    }

    void Edge::_set_fanout(const int i, TimingPoint *tp)
    {
        if(_to.empty() && !i)
            _to.push_back(tp);
        else
            _to[i] = tp;
        assert(_to.size() == 1);
    }

    std::ostream &operator<<(std::ostream &out, const TimingArc &ta)
    {
        return out << ta.from()->name() << " -> " << ta.to()->name();

    }

}
