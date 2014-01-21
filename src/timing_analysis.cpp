#include "include/timing_analysis.h"

namespace Timing_Analysis
{
/*

    TIMING ANALYSIS

*/


Timing_Analysis::Timing_Analysis(const Circuit_Netlist & netlist, const LibertyLibrary * lib, const Parasitics * parasitics, const Design_Constraints * sdc) : _gate_index_to_timing_point_index(netlist.getGatesSize()), _verilog(netlist.verilog()), _sizes(_verilog.size()), _library(lib), _parasitics(parasitics), _first_PO_index(-1), _total_violating_POs(0)
{

    _target_delay = Transitions<double>(sdc->clock(), sdc->clock());
    _max_transition = Transitions<double>(lib->getMaxTransition(), lib->getMaxTransition());

    int timing_points, timing_arcs;

    // FORNECER PELA NETLIST (Mais eficiente)
    number_of_timing_points_and_timing_arcs(timing_points, timing_arcs, netlist, lib);

    _points.reserve(timing_points);
    _arcs.reserve(timing_arcs);
    _nets.reserve(netlist.getNetsSize());
    _options.reserve(netlist.getGatesSize());

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
            const LibertyCellInfo & opt = liberty_cell_info(tp.gate_number());
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
            if(_first_PO_index == -1)
                _first_PO_index = i;
            const LibertyCellInfo & opt = liberty_cell_info(tp.gate_number());
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
            const LibertyCellInfo & opt = liberty_cell_info(driver_timing_point.gate_number());

            if(parasitics->find(net.name) != parasitics->end())
            {
                delay_model = new Ceff_Elmore_Slew_Degradation_PURI(parasitics->at(net.name), driver_timing_point.name(), opt.timingArcs.size());

//                                delay_model = new Lumped_Elmore_Slew_Degradation(parasitics->at(net.name), driver_timing_point.name(), opt.timingArcs.size());
                //                delay_model = new Lumped_Elmore_No_Slew_Degradation(parasitics->at(net.name), driver_timing_point.name(), opt.timingArcs.size());
                //                delay_model = new Ceff_Elmore_No_Slew_Degradation(parasitics->at(net.name), driver_timing_point.name(), opt.timingArcs.size());

//                                delay_model = new LumpedCapacitanceWireDelayModel(parasitics->at(net.name), driver_timing_point.name());

//                                delay_model = new Ceff_Elmore_Slew_Degradation(parasitics->at(net.name), driver_timing_point.name(), opt.timingArcs.size());
                //                delay_model = new Ceff_Without_Wire_Delay_And_Slew_Degradation(parasitics->at(net.name), driver_timing_point.name(), opt.timingArcs.size());
                //                delay_model = new Reduced_Pi(parasitics->at(net.name), driver_timing_point.name(), opt.timingArcs.size());
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


    for(int i = 0; i < _points.size(); i++)
    {
        Timing_Point & tp = _points.at(i);
        if(tp.is_PI_input() || tp.is_reg_input())
        {
            tp.logic_level(0);
        }
        else if(tp.is_input_pin() || tp.is_PO())
        {
            tp.logic_level(tp.net().from().logic_level() + 1);
        } else if(tp.is_output_pin() || tp.is_PI())
        {

            int current_index = i-1;
            int current_gate = _points.at(current_index).gate_number();
            int max_logic_level = 0;
            while(current_gate == tp.gate_number())
            {
                Timing_Point & tp_in = _points.at(current_index);
                max_logic_level = max(max_logic_level, tp_in.logic_level());
                current_index--;
                if(current_index == -1)
                    break;
                current_gate = _points.at(current_index).gate_number();
            }
            tp.logic_level(max_logic_level + 1);
        }

    }


    // CHECKING FIRST PO INDEX
    for(int i = 0; i < _points.size(); i++)
    {
        if(i >= _first_PO_index)
        {
            //                cout << _points.at(i).name() << endl;
            assert(_points.at(i).is_PO());
        }
        else
            assert(!_points.at(i).is_PO());
    }

    //        cout << "OK " << endl;

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
            if(cellInfo.pins.at(j).name == "d" && is_sequential)
                continue;
            timingPointName = gateName + ":" + cellInfo.pins.at(j).name;
            if(is_sequential)
                type = REGISTER_INPUT;
            else if (is_primary_input)
                type = PI_INPUT;
            else
                type = INPUT;

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
    //    cout << "number of timing points " << numberOfTimingPoints << " netlist number " << netlist.timing_points() << endl;
    //    assert(numberOfTimingPoints == netlist.timing_points());
    //    cout << "number of timing arcs " << numberOfTiming_Arcs << " netlist number " << netlist.timing_arcs() << endl;
    //    assert(numberOfTiming_Arcs == netlist.timing_arcs());

}

void Timing_Analysis::create_timing_arcs(const pair<size_t, size_t> tpIndexes, const bool is_pi, const bool is_po )
{
    if( is_pi )
    {
        // cout << "  PI timing arc " << points.at(tpIndexes.first).name << " -> " << points.at(tpIndexes.second).name << endl;
        _arcs.push_back(Timing_Arc(&_points.at(tpIndexes.first), &_points.at(tpIndexes.second), 0, _points.at(tpIndexes.first).gate_number()));
        _points.at(tpIndexes.first).arc(&_arcs.back());
        assert(&_arcs.back().from() == &_points.at(tpIndexes.first) && &_arcs.back().to() == &_points.at(tpIndexes.second));
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
                assert(&_arcs.back().from() == &_points.at(j) && &_arcs.back().to() == &_points.at(tpIndexes.second));
                // cout << "  timing arc OK" << endl;
            }
        }
    }
}

Timing_Analysis::~Timing_Analysis()
{

}

void Timing_Analysis::initialize_timing_data()
{
    std::fill(_dirty.begin(), _dirty.end(), false);
    _slew_violations = numeric_limits<Transitions<double> >::zero();
    _capacitance_violations = numeric_limits<Transitions<double> >::zero();
    _total_negative_slack = numeric_limits<Transitions<double> >::zero();
    _critical_path = numeric_limits<Transitions<double> >::zero();
    _total_violating_POs = 0;

}

void Timing_Analysis::full_timing_analysis()
{
    initialize_timing_data();

    for(size_t i = 0; i < _points.size(); i++)
    {
        update_timing(i);
        update_violations(i);
    }

    for(size_t i = 0; i < _points.size(); i++)
    {
        size_t n = _points.size() - i - 1;
        update_slacks(n);
    }

}

typedef priority_queue<Timing_Point*, vector<Timing_Point*>, ita_comparator> ita_priority_queue;

/*
Timing_Analysis::incremental_timing_analysis

Enfilera o timing point de saída da porta que está trocando de opção;
Enfilera os timing points de saída das portas que são fanins da porta que está trocando de opção;

Processamento da fila (até esvaziar):
    Atualiza informação de timing;
    Se alterou arrival time ou slew:
        Enfilera os timing points de saída das portas fanouts da porta que está sendo processada.

Atualiza Slack nas saídas primárias

*/

void Timing_Analysis::incremental_timing_analysis(int gate_number, int new_option)
{
    int tp_index = _gate_index_to_timing_point_index.at(gate_number).second;

    Timing_Point * timing_point = &_points.at(tp_index);

    assert(timing_point->is_output_pin());
    assert(timing_point->gate_number() == gate_number);

    ita_priority_queue pq;

    _total_negative_slack = numeric_limits<Transitions<double> >::zero();
    _critical_path = numeric_limits<Transitions<double> >::zero();
    _total_violating_POs = 0;

    set<Timing_Point*> inserted;
    if(inserted.insert(timing_point).second)
    {
        pq.push(timing_point);
        assert(option(timing_point->gate_number(), new_option));
    }

    // FANINS MUST BE PUSHED IF THE GATE SIZE (INPUT PIN CAPACITANCE) CHANGES
    int input_pin_index = tp_index - 1;
    while(input_pin_index >= 0 && _points.at(input_pin_index).gate_number() == timing_point->gate_number())
    {
        Timing_Point & fanin = _points.at(input_pin_index).net().from();
        if(inserted.insert(&fanin).second)
            pq.push(&fanin);
        input_pin_index--;
    }

    int updated = 0;
//    cout << " -- timing points being updated: " << endl;
    while(!pq.empty())
    {
        Timing_Point * tp = pq.top();

        assert(tp->is_output_pin() || tp->is_PI());

        pq.pop();
        updated++;

        // SAVE PREVIOUS TIMING INFORMATION
        vector<Transitions<double> > slews0(tp->net().fanouts_size());
        vector<Transitions<double> > arrival_times0(slews0);
        vector<Transitions<double> > slew_violations0(slews0);

        for(int i = 0; i < slews0.size(); i++)
        {
            slews0.at(i) = tp->net().to(i).slew();
            arrival_times0.at(i) = tp->net().to(i).arrival_time();
            slew_violations0.at(i) = max(numeric_limits<Transitions<double> >::zero(), tp->net().to(i).slew() - _max_transition);
        }

        // UPDATE GATE TIMING POINTS AND VIOLATIONS
        update_timing_points(tp);

        // IF CHANGES, PUSH FANOUTS
        vector<Transitions<double> > slewsF(tp->net().fanouts_size());
        vector<Transitions<double> > arrival_timesF(slewsF);

        for(int i = 0; i < slewsF.size(); i++)
        {
            slewsF.at(i) = tp->net().to(i).slew();
            arrival_timesF.at(i) = tp->net().to(i).arrival_time();

            const bool changed =
                    abs(slewsF.at(i) - slews0.at(i)).getMax() > Traits::STD_THRESHOLD ||
                    abs(arrival_timesF.at(i) - arrival_times0.at(i)).getMax() > Traits::STD_THRESHOLD;

            if(changed)
            {
                Timing_Point * input_tp_of_fanout = &tp->net().to(i);
                if( input_tp_of_fanout->is_PO() )
                {
                    continue;
                }

                Timing_Point * output_tp_of_fanout = &tp->net().to(i).arc().to();
                if(input_tp_of_fanout->is_input_pin() && inserted.insert(output_tp_of_fanout).second)
                {
                    pq.push(output_tp_of_fanout);
                }
            }
        }
    }
//    cout << "  -- total = " << updated << endl;

    // UPDATE PO SLACKS, TNS AND CRITICAL PATH VALUES
    for (int i = _first_PO_index; i < _points.size(); i++)
    {

        update_timing(i);
        update_slacks(i);
    }

}

void Timing_Analysis::update_timing_points(const Timing_Point *output_timing_point)
{
    unsigned output_timing_point_index = output_timing_point - &_points.front();
    unsigned first_input_timing_point_index = _gate_index_to_timing_point_index.at(output_timing_point->gate_number()).first;

    _dirty.at(_points.at(output_timing_point_index).gate_number()) = false;

    for(int i = first_input_timing_point_index; i < output_timing_point_index; i++)
    {
        assert(output_timing_point->gate_number() == _points.at(i).gate_number());
        this->clear_violations(i);
        this->update_timing(i);
        this->update_violations(i);
    }

    this->clear_violations(output_timing_point_index);
    this->update_timing(output_timing_point_index);
    this->update_violations(output_timing_point_index);
}

bool Timing_Analysis::option(const int gate_index, const int option_number)
{
    const LibertyCellInfo & old_cell_info = liberty_cell_info(gate_index);
    const Option & gate_option = _options.at(gate_index);
    if( !gate_option._dont_touch )
    {
        _options.at(gate_index)._option_index = option_number;
        const LibertyCellInfo & new_cell_info = liberty_cell_info(gate_index);

        // UPDATE FANINS OUTPUT LOADS
        const pair<size_t, size_t> timing_points = _gate_index_to_timing_point_index.at(gate_index);
        for(size_t timing_point_index = timing_points.first; timing_point_index <= timing_points.second; timing_point_index++)
        {
            Timing_Point & timing_point = _points.at(timing_point_index);

            if(timing_point.is_input_pin())
            {
                // IF SEQUENTIAL, INPUT PIN NUMBER = 2
                const int pin_number = timing_point.arc().arc_number() + 1; /* +1 porque o primeiro é o pino de saída */
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

void Timing_Analysis::set_all_gates_to_max_size()
{
    for(int i = 0; i < _points.size(); i++)
        option(_points.at(i).gate_number(), 0);
}

void Timing_Analysis::set_all_gates_to_min_size()
{
    for(int i = 0; i < _points.size(); i++)
    {
        if(_points.at(i).is_PO() || _points.at(i).is_PI())
            continue;
        const int gate_number = _points.at(i).gate_number();
        const int number_of_options = _library->number_of_options(_options.at(gate_number)._footprint_index);
        option(gate_number, number_of_options - 1);
    }
}

void Timing_Analysis::update_slacks(const int timing_point_index)
{
    Timing_Point & timing_point = _points.at(timing_point_index);
    Timing_Net & net = timing_point.net();

    Transitions<double> required_time = numeric_limits<Transitions<double> >::max();

    if( timing_point.is_PI_input() || timing_point.is_reg_input() ) // doesn't matter
        return;
    if( timing_point.is_input_pin() )
        required_time = (timing_point.arc().to().required_time() - timing_point.arc().delay()).getReversed();
    else if( timing_point.is_PO() )
        required_time = _target_delay;
    else if( timing_point.is_output_pin() || timing_point.is_PI() )
    {
        for(size_t i = 0; i < net.fanouts_size(); i++)
        {
            const Transitions<double> interconnect_delay = net.to(i).arrival_time() - timing_point.arrival_time();
            required_time = min(required_time, net.to(i).required_time() - interconnect_delay);
        }
    }

    if(timing_point.update_slack(required_time).getMin() < 0 && timing_point.is_PO())
    {
        _total_violating_POs++;
        _total_negative_slack -= min(numeric_limits<Transitions<double> >::zero(), timing_point.slack());
    }
}

void Timing_Analysis::clear_violations(const int timing_point_index)
{
    Timing_Point & timing_point = _points.at(timing_point_index);
    if(timing_point.is_input_pin() || timing_point.is_PI_input() || timing_point.is_reg_input())
    {
        _slew_violations -= max(numeric_limits<Transitions<double> >::zero(), timing_point.slew() - _max_transition);
    }
    else if(timing_point.is_output_pin() || timing_point.is_PI())
    {
        const LibertyCellInfo & cell_info = liberty_cell_info(timing_point.gate_number());
        _capacitance_violations -= max(numeric_limits<Transitions<double> >::zero(), (timing_point.ceff() - cell_info.pins.front().maxCapacitance));
    }
    else if(timing_point.is_PO())
    {
    }
}

void Timing_Analysis::update_violations(const int timing_point_index)
{
    Timing_Point & timing_point = _points.at(timing_point_index);

    if(timing_point.is_input_pin() || timing_point.is_PI_input() || timing_point.is_reg_input())
    {
        _slew_violations += max(numeric_limits<Transitions<double> >::zero(), timing_point.slew() - _max_transition);
    }
    else if(timing_point.is_output_pin() || timing_point.is_PI())
    {
        const LibertyCellInfo & cell_info = liberty_cell_info(timing_point.gate_number());
//        _capacitance_violations += max(numeric_limits<Transitions<double> >::zero(), (timing_point.ceff() - cell_info.pins.front().maxCapacitance));
        _capacitance_violations += max(double(0.0f), (timing_point.net().wire_delay_model()->lumped_capacitance() - cell_info.pins.front().maxCapacitance));
    }
    else if(timing_point.is_PO())
    {
    }
}


void Timing_Analysis::update_timing(const int timing_point_index)
{
    Timing_Point & timing_point = _points.at(timing_point_index);

    if(timing_point.is_input_pin() || timing_point.is_PI_input() || timing_point.is_reg_input())
    {
        Timing_Point & output_pin = timing_point.arc().to();
        assert(output_pin.gate_number() == timing_point.gate_number());
        Timing_Arc & timing_arc = timing_point.arc();
        assert(timing_arc.gate_number() == timing_point.gate_number());
        Timing_Net & output_net = output_pin.net();
        const bool is_the_first_input_pin_of_a_gate = !_dirty.at(output_pin.gate_number());
        const LibertyCellInfo & cell_info = liberty_cell_info(timing_point.gate_number());

        if( is_the_first_input_pin_of_a_gate )
        {
            assert(output_pin.gate_number() < _dirty.size());
            _dirty.at(output_pin.gate_number()) = true;
            output_pin.clear_timing_info();
            output_net.wire_delay_model()->clear();
        }

        const Transitions<double> ceff_by_this_timing_arc = output_net.wire_delay_model()->simulate(cell_info, timing_arc.arc_number(), timing_point.slew(), timing_point.is_PI_input());
        output_pin.ceff(max(output_pin.ceff(), ceff_by_this_timing_arc));

        const Transitions<double> current_arc_delay_at_output_pin = calculate_timing_arc_delay(timing_arc, timing_point.slew(), ceff_by_this_timing_arc);
        const Transitions<double> current_arc_slew_at_output_pin = output_net.wire_delay_model()->root_slew(timing_arc.arc_number());

        timing_arc.delay(current_arc_delay_at_output_pin);
        timing_arc.slew(current_arc_slew_at_output_pin);

        // SETTING OUTPUT SLEW AND ARRIVAL TIME
        const Transitions<double> max_arrival_time = max(output_pin.arrival_time(), timing_point.arrival_time().getReversed() + timing_arc.delay());
        const Transitions<double> max_slew = max(output_pin.slew(), timing_arc.slew());

        output_pin.arrival_time(max_arrival_time); // NEGATIVE UNATE
        output_pin.slew(max_slew);

    }
    else if(timing_point.is_output_pin() || timing_point.is_PI())
    {
        Timing_Net & output_net = timing_point.net();

        for(size_t i = 0; i < output_net.fanouts_size(); i++)
        {
            Timing_Point & fanout_timing_point = output_net.to(i);
            fanout_timing_point.arrival_time(timing_point.arrival_time() + output_net.wire_delay_model()->delay_at_fanout_node(fanout_timing_point.name()));
            fanout_timing_point.slew(output_net.wire_delay_model()->slew_at_fanout_node(fanout_timing_point.name()));
        }
    }
    else if(timing_point.is_PO())
    {
        _critical_path = max(_critical_path, timing_point.arrival_time());
    }

}

const Transitions<double> Timing_Analysis::calculate_timing_arc_delay(const Timing_Arc & timing_arc, const Transitions<double> transition, const Transitions<double> ceff)
{
    const LibertyCellInfo & cellInfo = liberty_cell_info(timing_arc.gate_number());
    const LibertyLookupTable & fallLUT = cellInfo.timingArcs.at(timing_arc.arc_number()).fallDelay;
    const LibertyLookupTable & riseLUT = cellInfo.timingArcs.at(timing_arc.arc_number()).riseDelay;
    return _interpolator->interpolate(riseLUT, fallLUT, ceff, transition, cellInfo.isSequential ? NON_UNATE : NEGATIVE_UNATE, timing_arc.from().is_PI_input());
}

const LibertyCellInfo &Timing_Analysis::liberty_cell_info(int gate_index, int option_index) const
{
    if(option_index == -1)
        return _library->getCellInfo(_options.at(gate_index)._footprint_index, _options.at(gate_index)._option_index);
    return _library->getCellInfo(_options.at(gate_index)._footprint_index, option_index);
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


    //        cout << "Ceff" << endl;
    //        for(size_t i = 0; i < _points.size(); i++)
    //        {
    //            if(_points.at(i).is_PI() || _points.at(i).is_output_pin())
    //                cout << "ceff " << _points.at(i).name() << " = " << _points.at(i).ceff() << endl;
    //        }
    //        cout << "##" << endl;

    queue<int> pins;
    for(size_t i = 0 ; i < _points.size(); i++)
    {
        const Timing_Point & tp = _points.at(i);
        const LibertyCellInfo & cellInfo = liberty_cell_info(tp.gate_number());

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
    cout << "| Slew Violations = " << _slew_violations << endl;
    cout << "| Capacitance Violations = " << _capacitance_violations << endl;
    cout << "| Total Negative Slack = " << _total_negative_slack << endl;
    printf("-------------------------------------------------------------------------------------------------------\n\n\n");
}

void Timing_Analysis::report_timing()
{
    stack<int> current_gate;


    bool done = false;
    int previous_gate = -1;

    int i = 0;
    int k = 0;
    do {
        if(!_points.at(i).is_input_pin())
        {
            i++;
            continue;
        }

        current_gate.push(_points.at(i).gate_number());
        if(previous_gate != current_gate.top())
        {
            cout << endl;
            cout << "timing info for cell '"<< current_gate.top() <<"'" << endl;
            cout << "--------------------------------------------------------------" << endl;
        }
        cout << endl;
        cout << "from_pin\tto_pin\tarc_rise\tarc_fall\tsense" << endl;
        previous_gate = current_gate.top();

        k = 1;
        while(_points.at(i+k).gate_number() == previous_gate)
        {
            Timing_Point & timing_point = _points.at(i+k-1);
            Timing_Arc & arc = timing_point.arc();
            Timing_Point & to_pin = arc.to();
            cout << timing_point.name() << "\t" << to_pin.name() << "\t"<<arc.delay().getRise()<<"\t" << arc.delay().getFall() << endl;
            k++;
        }
        cout << endl;

        k = 1;
        while(_points.at(i+k).gate_number() == previous_gate)
        {
            cout << "from_pin\tslack_r\tslack_f\tarrival_r\tarrival_f\ttrans_r\ttrans_f" << endl;
            Timing_Point & timing_point = _points.at(i+k-1);
            cout << timing_point.name() << "\t" << timing_point.slack().getRise() << "\t"<< timing_point.slack().getFall()<<"\t" << timing_point.arrival_time().getRise() << "\t" << timing_point.arrival_time().getFall() << "\t" << timing_point.slew().getRise() << "\t" << timing_point.slew().getFall() << endl;
            cout << endl;
            k++;
        }


        cout << "to_pin\tslack_r\tslack_f" << endl;
        Timing_Point & timing_point = _points.at(i+k);
        cout << timing_point.name() << "\t" << timing_point.slack().getRise() << "\t"<< timing_point.slack().getFall()<<"\t"<< endl;

        i += k;


    } while(i < _points.size());

}

void Timing_Analysis::print_PO_arrivals()
{
    for(int i = _first_PO_index; i < _points.size(); i++)
    {
        Timing_Point & tp = _points.at(i);
        assert(tp.is_PO());

        cout << tp.name() << " arr " << tp.arrival_time() << " req " << tp.required_time() << " slack " << tp.slack() << " slew " << tp.slew() << endl;
    }
}

void Timing_Analysis::print_effective_capacitances()
{
    //        cout << "-- Effective Capacitances" << endl;
    //        for(vector<Timing_Point>::iterator it = _points.begin(); it != _points.end(); it++)
    //        {
    //            const Timing_Point & tp = (*it);
    //            if(_max_ceff.find(tp.name()) != _max_ceff.end())
    //                cout << tp.name() << " " << tp.ceff().getRise() << " " << tp.ceff().getFall() << endl;
    //        }
    //        cout << "--" << endl;
}

set<int> Timing_Analysis::timing_points_in_longest_path()
{
    int max_path_PO = _first_PO_index;

    for(int i = _first_PO_index + 1; i < _points.size(); i++)
    {
        const Timing_Point & tp = _points.at(i);
        if(tp.logic_level() > _points.at(max_path_PO).logic_level())
            max_path_PO = i;
    }
    //    cout << "longest path size = " << _points.at(max_path_PO).logic_level() << endl;

    int current = max_path_PO;
    stack<int> path;
    set<int> longest_path;
    while(!_points.at(current).is_PI())
    {
        path.push(current);
        longest_path.insert(current);
        const Timing_Point & tp = _points.at(current);
        if(tp.is_PO() || tp.is_input_pin())
        {
            const int input_index = &tp.net().from() - &_points[0];
            current = input_index;
        } else if(tp.is_output_pin())
        {
            int input_index = current-1;
            int max_size_input = input_index;
            input_index--;
            while(_points.at(input_index).gate_number() == tp.gate_number())
            {
                if(_points.at(input_index).logic_level() > _points.at(max_size_input).logic_level())
                    max_size_input = input_index;
                input_index--;
            }
            current = max_size_input;
        }
        else
            assert(false);
    }
    path.push(current);
    longest_path.insert(current);


    //    while(!path.empty())
    //    {
    //        const int index = path.top();
    //        path.pop();

    //        const Timing_Point & tp = _points.at(index);
    //        cout << tp.name();
    //        if(tp.is_PI())
    //            cout << " PI";
    //        else if (tp.is_PO())
    //            cout << " PO";

    //        if(!path.empty())
    //            cout << " -> ";
    //        else
    //            cout << ";";
    //    }
    //    cout << endl;
    return longest_path;
}

set<int> Timing_Analysis::timing_points_in_critical_path()
{
    int max_path_PO = _first_PO_index;

    for(int i = _first_PO_index + 1; i < _points.size(); i++)
    {
        const Timing_Point & tp = _points.at(i);
        if(tp.arrival_time().getMax() > _points.at(max_path_PO).arrival_time().getMax())
            max_path_PO = i;
    }
    int current = max_path_PO;
    set<int> critical_path;
    stack<int> path;
    while(!_points.at(current).is_PI())
    {
        critical_path.insert(current);
        path.push(current);
        const Timing_Point & tp = _points.at(current);
        if(tp.is_PO() || tp.is_input_pin())
        {
            const int input_index = &tp.net().from() - &_points[0];
            current = input_index;
        } else if(tp.is_output_pin())
        {
            int input_index = current-1;
            int max_size_input = input_index;
            input_index--;
            while(_points.at(input_index).gate_number() == tp.gate_number())
            {
                if(_points.at(input_index).arrival_time().getMax() > _points.at(max_size_input).arrival_time().getMax())
                    max_size_input = input_index;
                input_index--;
            }
            current = max_size_input;
        }
        else
            assert(false);
    }
    critical_path.insert(current);
    path.push(current);


    //    cout << "critical path (" << _critical_path << ") size = " << critical_path.size() << endl;
    //    while(!path.empty())
    //    {
    //        const int index = path.top();
    //        path.pop();

    //        const Timing_Point & tp = _points.at(index);
    //        cout << tp.name();
    //        if(tp.is_PI())
    //            cout << " PI";
    //        else if (tp.is_PO())
    //            cout << " PO";

    //        if(!path.empty())
    //            cout << " -> ";
    //        else
    //            cout << ";";
    //    }
    //    cout << endl;
    return critical_path;
}

bool Timing_Analysis::has_timing_violations()
{
    return _total_negative_slack.getFall() != 0.0f || _total_negative_slack.getRise() != 0.0f;
}

int Timing_Analysis::Timing_Analysis::output_timing_point_index(int gate_number)
{
    return _gate_index_to_timing_point_index.at(gate_number).second;
}

pair<pair<int, int>, pair<Transitions<double>, Transitions<double> > > Timing_Analysis::check_ceffs(double precision)
{
    const string ceff_file = Traits::ispd_contest_root + "/" + Traits::ispd_contest_benchmark + "/" + Traits::ispd_contest_benchmark + ".ceff";
    fstream in;
    in.open(ceff_file.c_str(), fstream::in);

    string pin_name;
    double rise, fall;
    Transitions<double> ceff;

    int first_point_index = numeric_limits<int>::max();
    int first_logic_level = numeric_limits<int>::max();
    Transitions<double> tool_ceff, pt_ceff;

    while(!in.eof())
    {
        in >> pin_name;
        in >> rise;
        in >> fall;
        ceff.set(rise, fall);

        size_t slash_position = pin_name.find_first_of('/');
        if(slash_position != string::npos)
        {
            pin_name.replace(slash_position, 1, ":");
            assert(_pin_name_to_timing_point_index.find(pin_name) != _pin_name_to_timing_point_index.end());
            const Timing_Point & tp = _points.at(_pin_name_to_timing_point_index.at(pin_name));

            Transitions<double> ceff_error;
            ceff_error = abs(tp.ceff() - ceff) / max(abs(tp.ceff()), abs(ceff));
            //                if(tp.name() == "g2412_u1:o")
            //                    cout << "g2412_u1:o ceff " << tp.ceff() << " pt ceff " << ceff << endl;

            if(ceff_error.getMax() >= precision)
            {
                cout << "pin " << pin_name << " ceff " << tp.ceff() << " pt ceff " << ceff << " CEFF ERROR > " << precision*100 << "% = " << ceff_error << endl;
                if(tp.logic_level() < first_logic_level)
                {
                    first_point_index = &tp - &_points.at(0);
                    cout << "new first logic level " <<  tp.logic_level()  << "(old = " << first_logic_level << ")" << endl;
                    first_logic_level = tp.logic_level();

                    tool_ceff = tp.ceff();
                    pt_ceff = ceff;
                }
            }

        }
    }
    return make_pair(make_pair(first_point_index, first_logic_level), make_pair(tool_ceff, pt_ceff));
}

void Timing_Analysis::print_gates()
{
    for(int i = 0; i < _verilog.size(); i++)
    {
        if(!_options.at(_verilog.at(i).first).is_dont_touch())
            cout << _verilog.at(i).first << "\t" << _verilog.at(i).second << endl;
    }
}


// PRIMETIME CALLING
bool Timing_Analysis::validate_with_prime_time()
{
    const string timing_file = Traits::ispd_contest_root + "/" + Traits::ispd_contest_benchmark + "/" + Traits::ispd_contest_benchmark + ".timing";

    get_sizes_vector();
    const unsigned pollingTime = 1;

    //        cout << "Running timing analysis" << endl;

    TimerInterface::Status s = TimerInterface::runTimingAnalysisBlocking(_sizes,  Traits::ispd_contest_root, Traits::ispd_contest_benchmark, pollingTime);
    //    if(s != 2)
    //        cout << "Timing analysis finished with status: " << s << endl;

    return check_timing_file(timing_file);
}

void Timing_Analysis::call_prime_time(double target_delay_factor)
{
    const string timing_file = Traits::ispd_contest_root + "/" + Traits::ispd_contest_benchmark + "/" + Traits::ispd_contest_benchmark + ".timing";
    const string ceff_file = Traits::ispd_contest_root + "/" + Traits::ispd_contest_benchmark + "/" + Traits::ispd_contest_benchmark + ".ceff";

    get_sizes_vector();

    initialize_timing_data();
    const unsigned pollingTime = 1;

    cout << "Running timing analysis" << endl;

    TimerInterface::Status s = TimerInterface::runTimingAnalysisBlocking(_sizes,  Traits::ispd_contest_root, Traits::ispd_contest_benchmark, pollingTime);
    cout << "Timing analysis finished with status: " << s << endl;

    cout << "Reading Timing Information" << endl;
    Prime_Time_Output_Parser prime_time_parser;
    const Prime_Time_Output_Parser::Prime_Time_Output prime_time_output = prime_time_parser.parse_prime_time_output_file(timing_file);
    const Prime_Time_Output_Parser::Ceffs ceffs = prime_time_parser.parse_ceffs_file(ceff_file);

    cout << "Setting Timing Information" << endl;

    // setting ceffs
    for(size_t i = 0; i < ceffs.pins_size(); i++)
    {
        const Prime_Time_Output_Parser::Pin_Ceff pin_ceff = ceffs.pin(i);
        Timing_Point & timing_point = _points.at(_pin_name_to_timing_point_index.at(pin_ceff.pin_name));
        assert(timing_point.is_output_pin() || timing_point.is_PI());
        timing_point.ceff(pin_ceff.ceff);
    }

    for(size_t i = 0; i < prime_time_output.pins_size(); i++)
    {
        const Prime_Time_Output_Parser::Pin_Timing pin_timing = prime_time_output.pin(i);
        Timing_Point & timing_point = _points.at(_pin_name_to_timing_point_index.at(pin_timing.pin_name));
        timing_point.slack(pin_timing.slack);
        timing_point.slew(pin_timing.slew);
        timing_point.arrival_time(pin_timing.arrival_time);

        update_violations(_pin_name_to_timing_point_index.at(pin_timing.pin_name));

        if(timing_point.is_PO() /* pode ser o pino d de um registrador */)
        {
            _critical_path = max(_critical_path, timing_point.arrival_time());
            timing_point.slack(timing_point.slack() * target_delay_factor);
            if(timing_point.slack().getMin() < 0)
            {
                _total_negative_slack -= timing_point.slack();
                _total_violating_POs++;
            }
        }

    }

    for(size_t i = 0; i < prime_time_output.ports_size(); i++)
    {
        const Prime_Time_Output_Parser::Port_Timing port_timing = prime_time_output.port(i);
        Timing_Point & timing_point = _points.at(_pin_name_to_timing_point_index.at(port_timing.port_name));

        timing_point.slack(port_timing.slack);
        timing_point.slew(port_timing.slew);
        timing_point.arrival_time(port_timing.arrival_window);

        update_violations(_pin_name_to_timing_point_index.at(port_timing.port_name));

        //            cout << "port timing " << timing_point << endl;

        if(timing_point.is_PO())
        {
            timing_point.arrival_time(_target_delay - timing_point.slack());
            _critical_path = max(_critical_path, _target_delay - timing_point.slack());
            timing_point.slack(timing_point.slack() * target_delay_factor);
            if(timing_point.slack().getMin() < 0)
            {
                _total_negative_slack -= timing_point.slack();
                _total_violating_POs++;
            }
        }
    }
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

void Timing_Analysis::write_timing_file(const std::string filename)
{

    queue<int> ports;
    queue<int> sequentials;
    queue<int> pins;

    fstream out;
    out.open(filename.c_str(), fstream::out);
    out << "# pin timing" << endl;


    for(size_t i = 0 ; i < _points.size(); i++)
    {
        const Timing_Point & tp = _points.at(i);
        const LibertyCellInfo & cellInfo = liberty_cell_info(tp.gate_number());

        if( tp.is_PI() && cellInfo.isSequential )
        {
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
            out << tp.name() << " " <<  tp.slack().getRise() << " " << tp.slack().getFall() << " " << tp.slew().getRise() << " " << tp.slew().getFall() << " " << tp.arrival_time().getRise() << " " << tp.arrival_time().getFall() << endl;
            while( !pins.empty() )
            {
                const Timing_Point & iPin = _points.at(pins.front());
                pins.pop();
                out << iPin.name() << " " <<  iPin.slack().getRise() << " " << iPin.slack().getFall() << " " << iPin.slew().getRise() << " " << iPin.slew().getFall() << " " << iPin.arrival_time().getRise() << " " << iPin.arrival_time().getFall() << endl;
            }
        }


        if( !cellInfo.isSequential && (tp.is_PI() || tp.is_PO()) )
            ports.push(i);

        if( tp.is_PO() && cellInfo.isSequential )
        {
            const int reg = sequentials.front();
            sequentials.pop();
            const Timing_Point & regTp = _points.at(reg);
            out << regTp.name() << " " <<  regTp.slack().getRise() << " " << regTp.slack().getFall() << " " << regTp.slew().getRise() << " " << regTp.slew().getFall() << " " << regTp.arrival_time().getRise() << " " << regTp.arrival_time().getFall() << endl;
        }
    }

    out << "# port timing" << endl;

    while(!ports.empty())
    {
        const int tp_index = ports.front();
        ports.pop();
        const Timing_Point & tp = _points.at(tp_index);
        out << tp.name() << " " <<  tp.slack().getRise() << " " << tp.slack().getFall() << " " << tp.slew().getRise() << " " << tp.slew().getFall() << endl;
    }

    out.close();
}

double Timing_Analysis::pin_capacitance(const int timing_point_index) const
{
    const LibertyCellInfo & opt = liberty_cell_info(_points.at(timing_point_index).gate_number());
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

const Option & Timing_Analysis::option(const int gate_number)
{
    return _options.at(gate_number);
}

size_t Timing_Analysis::number_of_options(const int gate_index)
{
    return _library->number_of_options(_options.at(gate_index)._footprint_index);
}

bool Timing_Analysis::has_capacitance_violations(const Timing_Point &tp)
{
//    if(tp.ceff().getMax() > liberty_cell_info(tp.gate_number()).pins.front().maxCapacitance)
//        return true;
    if(tp.net().wire_delay_model()->lumped_capacitance() > liberty_cell_info(tp.gate_number()).pins.front().maxCapacitance)
        return true;
    return false;
}

bool Timing_Analysis::has_slew_violations(const Timing_Point &tp)
{
    if(tp.slew().getMax() > _max_transition.getMax())
        return true;
    return false;
}
void Timing_Analysis::get_sizes_vector()
{
    vector<pair<int, string> >::iterator verilog_iterator = _verilog.begin();
    vector<pair<string, string> >::iterator sizes_iterator = _sizes.begin();
    for(; verilog_iterator != _verilog.end() && sizes_iterator != _sizes.end(); verilog_iterator++, sizes_iterator++)
    {
        const pair<int, string> & cell = (*verilog_iterator);
        const LibertyCellInfo & opt = liberty_cell_info(cell.first);
        const pair<string, string> new_item(cell.second, opt.name);
        (*sizes_iterator) = new_item;
    }
}

bool Timing_Analysis::check_timing_file(const string timing_file)
{
    Prime_Time_Output_Parser prime_time_parser;
    const Prime_Time_Output_Parser::Prime_Time_Output prime_time_output = prime_time_parser.parse_prime_time_output_file(timing_file);

    fstream out;
    string file = timing_file + ".err";
    out.open(file.c_str(), fstream::out);

    Transitions<double> average_pin_slack_error = numeric_limits<Transitions<double> >::zero();
    Transitions<double> average_pin_slew_error = numeric_limits<Transitions<double> >::zero();
    Transitions<double> average_pin_arrival_time_error = numeric_limits<Transitions<double> >::zero();

    Transitions<double> average_port_slack_error = numeric_limits<Transitions<double> >::zero();
    Transitions<double> average_port_slew_error = numeric_limits<Transitions<double> >::zero();

    Transitions<double> max_slack_error = numeric_limits<Transitions<double> >::min();
    Transitions<double> max_slew_error = numeric_limits<Transitions<double> >::min();
    Transitions<double> max_arrival_time_error = numeric_limits<Transitions<double> >::min();

    Transitions<double> min_slack_error = numeric_limits<Transitions<double> >::max();
    Transitions<double> min_slew_error = numeric_limits<Transitions<double> >::max();
    Transitions<double> min_arrival_time_error = numeric_limits<Transitions<double> >::max();

    Transitions<int> worst_pin_index(-1, -1);

    Transitions<double> critical_path = numeric_limits<Transitions<double> >::min();

    for(size_t i = 0; i < prime_time_output.pins_size(); i++)
    {
        const Prime_Time_Output_Parser::Pin_Timing pin_timing = prime_time_output.pin(i);
        const Timing_Point & timing_point = _points.at(_pin_name_to_timing_point_index.at(pin_timing.pin_name));

        Transitions<double> pin_slack_error = (timing_point.slack()/pin_timing.slack) -1;
        Transitions<double> pin_slew_error = (timing_point.slew()/pin_timing.slew) -1;
        Transitions<double> pin_arrival_time_error = (timing_point.arrival_time()/pin_timing.arrival_time) -1;

        if(timing_point.is_output_pin() || timing_point.is_PO())
            out << timing_point.logic_level() << "\t" << pin_arrival_time_error.getMax() << endl;

        if(timing_point.is_PO())
            critical_path = max(critical_path, pin_timing.arrival_time);

        if(pin_arrival_time_error.getRise() > max_arrival_time_error.getRise())
            worst_pin_index.set(i, worst_pin_index.getFall());

        if(pin_arrival_time_error.getFall() > max_arrival_time_error.getFall())
            worst_pin_index.set(worst_pin_index.getRise(), i);

        max_slack_error = max(max_slack_error, pin_slack_error);
        max_slew_error = max(max_slew_error, pin_slew_error);
        max_arrival_time_error = max(max_arrival_time_error, pin_arrival_time_error);

        min_slack_error = min(min_slack_error, pin_slack_error);
        min_slew_error = min(min_slew_error, pin_slew_error);
        min_arrival_time_error = min(min_arrival_time_error, pin_arrival_time_error);

        average_pin_slack_error += pin_slack_error;
        average_pin_slew_error += pin_slew_error;
        average_pin_arrival_time_error += pin_arrival_time_error;
    }

    average_pin_slack_error /= prime_time_output.pins_size();
    average_pin_slew_error /= prime_time_output.pins_size();
    average_pin_arrival_time_error /= prime_time_output.pins_size();

    for(size_t i = 0; i < prime_time_output.ports_size(); i++)
    {
        const Prime_Time_Output_Parser::Port_Timing port_timing = prime_time_output.port(i);
        const Timing_Point & timing_point = _points.at(_pin_name_to_timing_point_index.at(port_timing.port_name));

        Transitions<double> port_slack_error = 1 - timing_point.slack() / port_timing.slack;
        Transitions<double> port_slew_error = 1 - timing_point.slew() / port_timing.slew;

        if(timing_point.is_PO())
        {
            Transitions<double> arrival_time = _target_delay - port_timing.slack;
            Transitions<double> port_arrival_time_error = (timing_point.arrival_time() / arrival_time) - 1;

            critical_path = max(critical_path, arrival_time);
            out << timing_point.logic_level() << "\t" << port_arrival_time_error.getMax() << endl;
        }

        average_port_slack_error += port_slack_error;
        average_port_slew_error += port_slew_error;

        max_slack_error = max(max_slack_error, port_slack_error);
        max_slew_error = max(max_slew_error, port_slew_error);

        min_slack_error = min(min_slack_error, port_slack_error);
        min_slew_error = min(min_slew_error, port_slew_error);

    }

    out.close();

    average_port_slack_error /= prime_time_output.ports_size();
    average_port_slew_error /= prime_time_output.ports_size();

//    cout << "max slack error " << max_slack_error << endl;
//    cout << "max slew error " << max_slew_error << endl;
//    cout << "max arrival error " << max_arrival_time_error << endl;

//    cout << "min slack error " << min_slack_error << endl;
//    cout << "min slew error " << min_slew_error << endl;
//    cout << "min arrival error " << min_arrival_time_error << endl;


    if(max_arrival_time_error.getMax() > Traits::STD_THRESHOLD
            || max_slew_error.getMax() > Traits::STD_THRESHOLD)
        return false;

    return true;
}

int Option::footprint_index() const
{
    return _footprint_index;
}

int Option::option_index() const
{
    return _option_index;
}

bool Option::is_dont_touch() const
{
    return _dont_touch;
}

bool ita_comparator::operator()(Timing_Point *a, Timing_Point *b)
{
    return a->logic_level() >= b->logic_level();
}

}
