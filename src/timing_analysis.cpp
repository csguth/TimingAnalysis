#include "include/timing_analysis.h"

namespace Timing_Analysis
{
/*

	TIMING ANALYSIS

*/
    const Transitions<double> Timing_Analysis::ZERO_TRANSITIONS(0.0f, 0.0f);
    const Transitions<double> Timing_Analysis::MIN_TRANSITIONS(numeric_limits<double>::min(), numeric_limits<double>::min());
    const Transitions<double> Timing_Analysis::MAX_TRANSITIONS(numeric_limits<double>::max(), numeric_limits<double>::max());
	
    Timing_Analysis::Timing_Analysis(const Circuit_Netlist & netlist, const LibertyLibrary * lib, const Parasitics * parasitics, const Design_Constraints * sdc) : _gate_index_to_timing_point_index(netlist.getGatesSize()), _verilog(netlist.verilog()), _sizes(_verilog.size()), _library(lib), _parasitics(parasitics)
	{

        _target_delay = Transitions<double>(sdc->clock(), sdc->clock());
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
			const Circuit_Netlist::Logic_Gate & gate = netlist.getGateT(i);
			// cout << "Gate " << gate.name << " (" << i << ") created!" << endl;
            const pair<int, int> cell_index = lib->getCellIndex(gate.cellType);
            const LibertyCellInfo & cell_info = lib->getCellInfo(cell_index.first, cell_index.second);
            const bool is_PI = gate.inputDriver;
            const bool is_PO = !cell_index.first || ( gate.sequential && !is_PI );

            assert(i < _gate_index_to_timing_point_index.size());
            _gate_index_to_timing_point_index[i] = create_timing_points(i, gate, cell_index, cell_info);
            create_timing_arcs(_gate_index_to_timing_point_index.at(i), is_PI, is_PO);

            _options.push_back(Option(cell_index.first, cell_index.second));
            if(gate.sequential || gate.inputDriver || cell_index.first == 0 /* PRIMARY OUTPUT*/)
                _options.back()._dont_touch = true;
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
            const Timing_Point & tp = _points.at(i);

            if(tp.is_PI())
			{
                const LibertyCellInfo & opt = option(tp.gate_number());
				if(opt.isSequential)
					continue;
                const string PI_name = tp.name();
                Timing_Point & inPin = _points.at(i - 1);
                inPin.arrival_time(sdc->input_delay(PI_name));
                inPin.slew(sdc->input_transition(PI_name));

				// cout << "setting input delay " << inPin.arrivalTime << " to pin " << inPin.name << endl;
				// cout << "setting input slew " << inPin.slew << " to pin " << inPin.name << endl;
			}


            if(tp.is_PO())
			{

                const LibertyCellInfo & opt = option(tp.gate_number());
				if(opt.isSequential)
					continue;

                _PO_loads.insert(make_pair(i, sdc->output_load(tp.name())));
				// cout << "setting poLoads["<<i<<"] = " << poLoads.at(i) << endl;
			}
		}

        // Create Timing_Nets
		for(size_t i = 0; i < netlist.getNetsSize(); i++)
		{
			const Circuit_Netlist::Net & net = netlist.getNetT(i);
            const int driverTopologicIndex = netlist.getTopologicIndex(net.sourceNode); // -1 if driver is the 'source'
			
            if(driverTopologicIndex == -1)
			{
                const string dummy_net_name = net.name;
                _nets.push_back(Timing_Net(dummy_net_name, 0, 0));

			}
			else
			{
                const int timing_point_index = _gate_index_to_timing_point_index.at(driverTopologicIndex).second;
                Timing_Point & driver_timing_point = _points.at(timing_point_index);

				// Just a test {		
                WireDelayModel * delay_model = 0;

				if(parasitics->find(net.name) != parasitics->end())
				{
					// delayModel = new RCTreeWireDelayModel(parasitics->at(net.name), rcTreeRootNodeName);
                    delay_model = new LumpedCapacitanceWireDelayModel(parasitics->at(net.name), driver_timing_point.name());
				}
				// }

                _nets.push_back(Timing_Net(net.name, &driver_timing_point, delay_model));

				// if(delayModel)
				// 	cout << "created net " << nets.back().netName << " with lumped capacitance " << driverTp->load() << endl;
                driver_timing_point.net(&(_nets.back()));
			}
	
        }
		
		// Now, setting the fanin edges of nodes
		for(size_t i = 0; i < netlist.getGatesSize(); i++)
		{
			const Circuit_Netlist::Logic_Gate & gate = netlist.getGateT(i);

			// cout << "gate " << gate.name << endl;
            const pair<size_t, size_t> timing_point_index = _gate_index_to_timing_point_index.at(i);
			for(size_t j = 0; j < gate.inNets.size(); j++)
			{
                const int in_net_topologic_index = netlist.get_net_topologic_index(gate.inNets.at(j));
                Timing_Net * in_net = &_nets.at(in_net_topologic_index);



                const int in_timing_point_index = timing_point_index.first + j;
                Timing_Point & fanout_timing_point = _points.at(in_timing_point_index);

                in_net->add_fanout(&fanout_timing_point);
                if(in_net->_wire_delay_model)
                {

                    const double pin_cap = pin_capacitance(in_timing_point_index);
                    in_net->_wire_delay_model->setFanoutPinCapacitance(fanout_timing_point.name(), pin_cap);
                }
                fanout_timing_point.net(in_net);
			}
		}
        _interpolator = new LinearLibertyLookupTableInterpolator();


	}

    const pair<size_t, size_t> Timing_Analysis::create_timing_points(const int i,const Circuit_Netlist::Logic_Gate & gate,const pair<int, int> cellIndex, const LibertyCellInfo & cellInfo)
	{
		const bool is_primary_input = gate.inputDriver;
        const bool is_sequential = gate.sequential;
        const bool is_primary_output = !cellIndex.first || ( is_sequential && !is_primary_input );

        const size_t firstTimingPointIndex = _points.size();
		string gateName = gate.name;
        if( is_sequential && is_primary_input )
			gateName = gateName.substr(0, gateName.size() - string("_PI").size());

        Timing_Point_Type type;
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

                if(type != REGISTER_INPUT)
                    _pin_name_to_timing_point_index.insert(make_pair(timingPointName, _points.size()));
                _points.push_back(Timing_Point(timingPointName, i, type));
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

        _pin_name_to_timing_point_index.insert(make_pair(timingPointName, _points.size()));
        _points.push_back(Timing_Point(timingPointName, i, type));
        return make_pair(firstTimingPointIndex, _points.size() - 1);
	}

    void Timing_Analysis::number_of_timing_points_and_timing_arcs(int & numberOfTimingPoints, int & numberOfTiming_Arcs, const Circuit_Netlist & netlist,const  LibertyLibrary * lib)
	{
		numberOfTimingPoints = 0;
        numberOfTiming_Arcs = 0;
		for(size_t i = 0; i < netlist.getGatesSize(); i++)
		{
			const Circuit_Netlist::Logic_Gate & gate = netlist.getGateT(i);
			const pair<int, int> cellIndex = lib->getCellIndex(gate.cellType);
            const bool is_PI = gate.inputDriver;
            const bool is_sequential = gate.sequential;
            const bool is_PO = !cellIndex.first || (is_sequential && !is_PI);
			numberOfTimingPoints += 1;
            if( !is_PO )
			{
				numberOfTimingPoints += gate.inNets.size();
                numberOfTiming_Arcs += (is_sequential ? 1 : gate.inNets.size());
			}
		}
	}

    void Timing_Analysis::create_timing_arcs(const pair<size_t, size_t> tpIndexes, const bool is_pi, const bool is_po )
	{
		if( is_pi )
		{
			// cout << "  PI timing arc " << points.at(tpIndexes.first).name << " -> " << points.at(tpIndexes.second).name << endl;
            _arcs.push_back(Timing_Arc(&_points.at(tpIndexes.first), &_points.at(tpIndexes.second), 0, _points.at(tpIndexes.first).gate_number()));
            _points.at(tpIndexes.first).arc(&_arcs.back());
            assert(_arcs.back().from() == &_points.at(tpIndexes.first) && &_arcs.back().to() == &_points.at(tpIndexes.second));
			// cout << "  PI timing arc OK" << endl;
		}
		else
		{
			if( !is_po )
			{
				for(size_t j = tpIndexes.first; j < tpIndexes.second; j++)
				{
					// cout << "  timing arc " << points.at(j).name << " -> " << points.at(tpIndexes.second).name << endl;
                    _arcs.push_back(Timing_Arc(&_points.at(j), &_points.at(tpIndexes.second), j-tpIndexes.first, _points.at(j).gate_number()));
                    _points.at(j).arc(&_arcs.back());
                    assert(_arcs.back().from() == &_points.at(j) && &_arcs.back().to() == &_points.at(tpIndexes.second));
					// cout << "  timing arc OK" << endl;
				}
			}
		}
	}

    Timing_Analysis::~Timing_Analysis()
	{

	}

    void Timing_Analysis::full_timing_analysis()
	{
        std::fill(_dirty.begin(), _dirty.end(), false);

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

    bool Timing_Analysis::gate_option(const int gate_index, const int option_number)
    {
        const LibertyCellInfo & old_cell_info = option(gate_index);
        const Option & gate_option = _options.at(gate_index);
        if( !gate_option._dont_touch )
        {
            _options.at(gate_index)._option_index = option_number;
            const LibertyCellInfo & new_cell_info = option(gate_index);

            // UPDATE FANINS OUTPUT LOADS
            const pair<size_t, size_t> timing_points = _gate_index_to_timing_point_index.at(gate_index);
            for(size_t timing_point_index = timing_points.first; timing_point_index <= timing_points.second; timing_point_index++)
            {
                Timing_Point & timing_point = _points.at(timing_point_index);

                if(timing_point.is_input_pin())
                {
                    // IF SEQUENTIAL, INPUT PIN NUMBER = 2
                    const int pin_number = (old_cell_info.isSequential ? 2 : timing_point.arc().arc_number() + 1);
                    const double old_pin_capacitance = old_cell_info.pins.at(pin_number).capacitance;
                    const double new_pin_capacitance = new_cell_info.pins.at(pin_number).capacitance;
                    Timing_Net & in_net = timing_point.net();
                    if(in_net.wire_delay_model())
                        in_net.wire_delay_model()->setFanoutPinCapacitance(timing_point.name(), new_pin_capacitance - old_pin_capacitance);
                }
            }

            return true;
        }
        return false;
    }

    void Timing_Analysis::update_slacks(const int timing_point_index)
	{
        Timing_Point & timing_point = _points.at(timing_point_index);
        Timing_Net & net = timing_point.net();

        Transitions<double> required_time = MAX_TRANSITIONS;

        if( timing_point.is_PI_input() || timing_point.is_reg_input() )
			return;
        if( timing_point.is_input_pin() ) //
            required_time = (timing_point.arc().to().required_time() - timing_point.arc().delay()).getReversed();
        else if( timing_point.is_PO() )
            required_time = _target_delay;
        else if( timing_point.is_output_pin() || timing_point.is_PI() )
		{
            for(size_t i = 0; i < net.fanouts_size(); i++)
                required_time = min(required_time, net.to(i).required_time());
		}

        timing_point.update_slack(required_time);

        if(timing_point.is_PO())
            _total_negative_slack -= min(ZERO_TRANSITIONS, timing_point.slack());
	}


    void Timing_Analysis::update_timing(const int timing_point_index)
	{
        Timing_Point & timing_point = _points.at(timing_point_index);
        if(timing_point.name() == "u25:a" || timing_point.name() == "u25:b")
            cout << endl;
		
        if(timing_point.is_input_pin() || timing_point.is_PI_input() || timing_point.is_reg_input())
		{	
            Timing_Point & output_pin = timing_point.arc().to();
            assert(output_pin.gate_number() == timing_point.gate_number());
            Timing_Arc & timing_arc = timing_point.arc();
            assert(timing_arc.gate_number() == timing_point.gate_number());
            Timing_Net & output_net = output_pin.net();
            const bool first = !_dirty.at(output_pin.gate_number());
            const LibertyCellInfo & cell_info = option(timing_point.gate_number());

			// IF IS THE FIRST INPUT PIN OF A GATE
			// CLEAR OUTPUT PIN
			if( first ) 
			{
                assert(output_pin.gate_number() < _dirty.size());
                _dirty[output_pin.gate_number()] = true;
                output_pin.clear_timing_info();
			}
		
			// SETTING ARC DELAY AND SLEW
            const Transitions<double> ceff = output_net.wire_delay_model()->simulate(cell_info, timing_arc.arc_number(), timing_point.slew());

            timing_arc.delay(output_net.wire_delay_model()->getDelay(output_pin.name())); // INPUT DRIVER DELAY = DELAY WITH CEFF - DELAY WITH 0 OUTPUT LOAD

            if(timing_point.is_PI_input())
			{
                const Transitions<double> delay_with_0_output_load = this->calculate_gate_delay(timing_point.gate_number(), 0, timing_point.slew(), ZERO_TRANSITIONS);
                timing_arc.delay(timing_arc.delay()-delay_with_0_output_load);
			}

            timing_arc.slew(output_net.wire_delay_model()->getSlew(output_pin.name()));

			// SETTING OUTPUT SLEW AND ARRIVAL TIME

            output_pin.arrival_time(max(output_pin.arrival_time(), timing_point.arrival_time().getReversed() + timing_arc.delay())); // NEGATIVE UNATE
            output_pin.slew(max(output_pin.slew(), timing_arc.slew()));

            assert(output_pin.arrival_time().getRise() >= timing_point.arrival_time().getFall() + timing_arc.delay().getRise());
            assert(output_pin.arrival_time().getFall() >= timing_point.arrival_time().getRise() + timing_arc.delay().getFall());

            assert(output_pin.slew().getRise() >= timing_arc.slew().getRise());
            assert(output_pin.slew().getFall() >= timing_arc.slew().getFall());


			// SETTING OUTPUT PIN VIOLATIONS
			if( first )
			{
                const LibertyCellInfo & cell_info = option(output_pin.gate_number());
                _capacitance_violations += max(ZERO_TRANSITIONS, (ceff - cell_info.pins.front().maxCapacitance));
			}

		}
        else if(timing_point.is_output_pin() || timing_point.is_PI())
		{
            Timing_Net & net = timing_point.net();
            for(size_t i = 0; i < net.fanouts_size(); i++)
			{
                Timing_Point & fanoutPin = net.to(i);
                fanoutPin.arrival_time(timing_point.arrival_time());
                fanoutPin.slew(timing_point.slew());
			}
		}
        else if(timing_point.is_PO())
        {
            _critical_path = max(_critical_path, timing_point.arrival_time());
        }
		
	}

    const Transitions<double> Timing_Analysis::calculate_gate_delay(const int gate_index, const int input_number, const Transitions<double> transition, const Transitions<double> ceff)
	{
        const LibertyCellInfo & cellInfo = option(gate_index);
        const LibertyLookupTable fallLUT = cellInfo.timingArcs.at(input_number).fallDelay;
        const LibertyLookupTable riseLUT = cellInfo.timingArcs.at(input_number).riseDelay;
        return _interpolator->interpolate(riseLUT, fallLUT, ceff, transition);
    }

    const LibertyCellInfo &Timing_Analysis::option(const int node_index) const
    {
        return _library->getCellInfo(_options.at(node_index)._footprint_index, _options.at(node_index)._option_index);
    }

    void Timing_Analysis::print_info()
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
            const Timing_Point & tp = _points.at(i);
            const LibertyCellInfo & cellInfo = option(tp.gate_number());

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
                printf("%s %f %f %f %f %f %f\n", tp.name().c_str(), tp.slack().getRise(), tp.slack().getFall(), tp.slew().getRise(), tp.slew().getFall(), tp.arrival_time().getRise(), tp.arrival_time().getFall());
				while( !pins.empty() )
				{
                    const Timing_Point & iPin = _points.at(pins.front());
					pins.pop();
                    printf("%s %f %f %f %f %f %f\n", iPin.name().c_str(), iPin.slack().getRise(), iPin.slack().getFall(), iPin.slew().getRise(), iPin.slew().getFall(), iPin.arrival_time().getRise(), iPin.arrival_time().getFall());
				}
			}


            if( !cellInfo.isSequential && (tp.is_PI() || tp.is_PO()) )
				ports.push(i);

            if( tp.is_PO() && cellInfo.isSequential )
			{
				const int reg = sequentials.front();
				sequentials.pop();
                const Timing_Point & regTp = _points.at(reg);
                printf("%s %f %f %f %f %f %f\n", regTp.name().c_str(), regTp.slack().getRise(), regTp.slack().getFall(), regTp.slew().getRise(), regTp.slew().getFall(), regTp.arrival_time().getRise(), regTp.arrival_time().getFall());
			}
		}

		printf("\n>>>> Timing Points Infos (ports)\n");
		while(!ports.empty())
		{
			const int tp_index = ports.front();
			ports.pop();
            const Timing_Point & tp = _points.at(tp_index);
            printf("%s %f %f %f %f\n", tp.name().c_str(), tp.slack().getRise(), tp.slack().getFall(), tp.slew().getRise(), tp.slew().getFall());
		}

		printf("-------------------------------------------------------------------------------------------------------\n\n\n");
		
	}

    void Timing_Analysis::print_circuit_info()
	{
		printf("########################################### TIMING INFO ###############################################\n");
        cout << "| Critical Path Values = " << _critical_path << " / " << _target_delay << endl;
        cout << "| Slew Violations = " << _slew_violations.aggregate() << endl;
        cout << "| Capacitance Violations = " << _capacitance_violations.aggregate() << endl;
        cout << "| Total Negative Slack = " << _total_negative_slack.aggregate() << endl;
		printf("-------------------------------------------------------------------------------------------------------\n\n\n");
	}

	// PRIMETIME CALLING
    bool Timing_Analysis::validate_with_prime_time()
	{
        const string timing_file = Traits::ispd_contest_root + "/" + Traits::ispd_contest_benchmark + "/" + Traits::ispd_contest_benchmark + ".timing";

        get_sizes_vector();
        const unsigned pollingTime = 1;

		cout << "Running timing analysis" << endl;

        TimerInterface::Status s = TimerInterface::runTimingAnalysisBlocking(_sizes,  Traits::ispd_contest_root, Traits::ispd_contest_benchmark, pollingTime);
		cout << "Timing analysis finished with status: " << s << endl;

		return check_timing_file(timing_file);
	}

    void Timing_Analysis::write_sizes_file(const string filename)
	{
        get_sizes_vector();
		fstream out;
		out.open(filename.c_str(), fstream::out);
        for(size_t i = 0; i < _sizes.size(); i++)
		{
            out << _sizes.at(i).first << "\t" << _sizes.at(i).second;
            if(i < _sizes.size() - 1)
				out << endl;
		}
		out.close();
	}

    double Timing_Analysis::pin_capacitance(const int timing_point_index) const
	{
        const LibertyCellInfo & opt = option(_points.at(timing_point_index).gate_number());
        const Timing_Point & timing_point = _points.at(timing_point_index);
        if( timing_point.is_input_pin() || timing_point.is_PI_input() || timing_point.is_reg_input() )
		{
            const int pin_number = _points.at(timing_point_index).arc().arc_number();
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

    int Timing_Analysis::option_number(const int gate_number)
    {
        return _options.at(gate_number)._option_index;
    }

    void Timing_Analysis::get_sizes_vector()
	{
        cout << "verilog size " << _verilog.size() << endl;
        vector<pair<int, string> >::iterator verilog_iterator = _verilog.begin();
        vector<pair<string, string> >::iterator sizes_iterator = _sizes.begin();
        for(; verilog_iterator != _verilog.end() && sizes_iterator != _sizes.end(); verilog_iterator++, sizes_iterator++)
		{
            const pair<int, string> & cell = (*verilog_iterator);
			const LibertyCellInfo & opt = option(cell.first);
            const pair<string, string> new_item(cell.second, opt.name);
            (*sizes_iterator) = new_item;
		}
	}

    bool Timing_Analysis::check_timing_file(const string timing_file)
	{
		Prime_Time_Output_Parser prime_time_parser;
		const Prime_Time_Output_Parser::Prime_Time_Output prime_time_output = prime_time_parser.parse_prime_time_output_file(timing_file);

		for(size_t i = 0; i < prime_time_output.pins_size(); i++)
		{
			const Prime_Time_Output_Parser::Pin_Timing pin_timing = prime_time_output.pin(i);
            const Timing_Point & timing_point = _points.at(_pin_name_to_timing_point_index.at(pin_timing.pin_name));

            assert(fabs(timing_point.slack().getRise() - pin_timing.slack.getRise()) < Traits::STD_THRESHOLD);
            assert(fabs(timing_point.slack().getFall() - pin_timing.slack.getFall()) < Traits::STD_THRESHOLD);
            assert(fabs(timing_point.slew().getRise() - pin_timing.slew.getRise()) < Traits::STD_THRESHOLD);
            assert(fabs(timing_point.slew().getFall() - pin_timing.slew.getFall()) < Traits::STD_THRESHOLD);
            assert(fabs(timing_point.arrival_time().getRise() - pin_timing.arrival_time.getRise()) < Traits::STD_THRESHOLD);
            assert(fabs(timing_point.arrival_time().getFall() - pin_timing.arrival_time.getFall()) < Traits::STD_THRESHOLD);
		}

        for(size_t i = 0; i < prime_time_output.ports_size(); i++)
		{
			const Prime_Time_Output_Parser::Port_Timing port_timing = prime_time_output.port(i);
            const Timing_Point & timing_point = _points.at(_pin_name_to_timing_point_index.at(port_timing.port_name));

            assert(fabs(timing_point.slack().getRise() - port_timing.slack.getRise()) < Traits::STD_THRESHOLD);
            assert(fabs(timing_point.slack().getFall() - port_timing.slack.getFall()) < Traits::STD_THRESHOLD);
            assert(fabs(timing_point.slew().getRise() - port_timing.slew.getRise()) < Traits::STD_THRESHOLD);
            assert(fabs(timing_point.slew().getFall() - port_timing.slew.getFall()) < Traits::STD_THRESHOLD);
		}

        return true;
    }

}
