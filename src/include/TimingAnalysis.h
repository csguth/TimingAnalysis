#ifndef TIMING_ANALYSIS_H_
#define TIMING_ANALYSIS_H_

#include <vector>
using std::vector;

#include <string>
using std::string;

#include <ostream>
using std::ostream;

#include <map>
using std::map;

#include <utility>
using std::pair;

#include <fstream>
using std::fstream;

#include <cstdlib>

#include "timer_interface.h"

#include "Transitions.h"
#include "CircuitNetList.h"
#include "WireDelayModel.h"
#include "LibertyLibrary.h"
#include "DesignConstraints.h"

#include "Configuration.h"

#include "Parser.h"

namespace TimingAnalysis
{
	class TimingNet;
	class TimingArc;

	enum TimingPointType
	{
		INPUT, OUTPUT, PI_INPUT, REGISTER_INPUT, PI, PO
	};

	class TimingPoint
	{
		friend class TimingAnalysis;
		friend class TimingNet;
        string _name;
        TimingNet * _net;
		TimingArc * arc;
        Transitions<double> _slack;
        Transitions<double> _slew;
        Transitions<double> _arrival_time;
        int _gate_number;
        TimingPointType _type;


        // TIMING ANALYSIS
        const Transitions<double> update_slack(const Transitions<double> required_time);
        void clear_timing_info();

	public:
        TimingPoint(string name, const int gate_number, TimingPointType type);
        virtual ~TimingPoint(){}


        // GETTERS
        double load() const;
        const string name() const { return _name; }
        int gate_number() const  { return _gate_number; }
        const Transitions<double> arrival_time() const { return _arrival_time; }
        const Transitions<double> required_time() const {return _slack + _arrival_time;}
        bool is_PO() const { return _type == PO; }
        bool is_PI() const { return _type == PI; }
        bool is_input_pin() const { return _type == INPUT; }
        bool is_output_pin() const { return _type == OUTPUT; }
        bool is_PI_input() const { return _type == PI_INPUT; }
        bool is_reg_input() const { return _type == REGISTER_INPUT; }

        friend ostream & operator<<(ostream & out, const TimingPoint & tp);
	};


	class Edge {
        TimingPoint * _from;
        vector<TimingPoint *> _to;
	protected:
        Edge(TimingPoint * from) : _from(from) {}
        TimingPoint * _getTo(const int i) const { return _to.at(i); }
        void _add_fanout(TimingPoint * tp);
        void _set_fanout(const int i, TimingPoint * tp);
	public:
        TimingPoint * from() const { return _from; }
        size_t fanouts_size() const { return _to.size(); }
	};

	class OneFanoutEdge : public Edge
	{
	public:
		OneFanoutEdge(TimingPoint * from, TimingPoint * to) : Edge(from) {
            _set_fanout(0, to);
		}
        TimingPoint * to() const { return _getTo(0); }
		
	};

	class MultiFanoutEdge : public Edge
	{
	public:
        MultiFanoutEdge(TimingPoint * from) : Edge(from) {}
        TimingPoint * to(const int i) const { return _getTo(i); }
	};

	class TimingArc : public OneFanoutEdge
	{
		friend class TimingAnalysis;
        Transitions<double> _delay;
        Transitions<double> _slew;
        int _arc_number;
        int _gate_number;

	public:
        TimingArc(TimingPoint * from, TimingPoint * to, const int arcNumber) : OneFanoutEdge(from, to), _delay(0.0f, 0.0f), _slew(0.0f, 0.0f), _arc_number(arcNumber), _gate_number(from->gate_number()) {}
        virtual ~TimingArc(){}


        // GETTERS
        Transitions<double> delay() const { return _delay; }
        Transitions<double> slew() const { return _slew; }
        int arc_number() const { return _arc_number; }
        int gate_number() const { return _gate_number; }


        friend ostream & operator<<(ostream & out, const TimingArc & ta);

	};

	class TimingNet : public MultiFanoutEdge
	{
		friend class TimingAnalysis;
		friend class TimingPoint;
        string _name;
        WireDelayModel * _wire_delay_model;

        void add_fanout(TimingPoint * tp);

	public:
        TimingNet(const string & name, TimingPoint * from, WireDelayModel * wire_delay_model)
        :MultiFanoutEdge(from), _name(name),_wire_delay_model(wire_delay_model)
		{
		}
        virtual ~TimingNet(){}

        const string name() const;
        friend ostream & operator<<(ostream & out, const TimingNet & tn);
	};

	struct Option
	{
        int footprint_index;
        int option_index;
        bool dont_touch;

        Option():footprint_index(-1), option_index(-1), dont_touch(false){}
        Option(const int footprintIndex, const int optionIndex) : footprint_index(footprintIndex), option_index(optionIndex), dont_touch(false){}
	};

	

	

	

	class TimingAnalysis
	{


        vector<TimingPoint> _points;
        vector<TimingArc> _arcs;
        vector<TimingNet> _nets;
        vector<Option> _options;
        map<int, double> _PO_loads;
        map<string, int> _pin_name_to_timing_point_index;
        vector<pair<size_t, size_t> > _gate_index_to_timing_point_index;

        vector<pair<int, string> > _verilog;
        vector<bool> _dirty;

        const LibertyLibrary * _library;
        const Parasitics * _parasitics;
        LibertyLookupTableInterpolator * _interpolator;

        Transitions<double> _target_delay;
        Transitions<double> _max_transition;
        Transitions<double> _critical_path;
        Transitions<double> _total_negative_slack;
        Transitions<double> _slew_violations;
        Transitions<double> _capacitance_violations;

        // PRIVATE GETTERS
        const LibertyCellInfo & option(const int node_index) const;
        const Transitions<double> calculate_gate_delay(const int gate_index, const int input_number, const Transitions<double> transition, const Transitions<double> ceff);

        // STATIC TIMING ANALYSIS
        void update_timing(const int timing_point_index);
        void update_slacks(const int timing_point_index);

		// TOPOLOGY INIT
        const pair<size_t, size_t> create_timing_points(const int i,const CircuitNetList::LogicGate & gate,const pair<int, int> cellIndex, const LibertyCellInfo & cellInfo);
        void number_of_timing_points_and_timing_arcs(int & numberOfTimingPoints, int & numberOfTimingArcs, const CircuitNetList & netlist, const LibertyLibrary * lib);
        void create_timing_arcs(const pair<size_t, size_t> tpIndexes, const bool is_pi , const bool is_po );

		// PRIMETIME CALLING
		const vector<pair<string, string> > get_sizes_vector();
		bool check_timing_file(const string timing_file);

		// OUTPUT METHODS
        void write_sizes_file(const string filename);

	public:
        TimingAnalysis(const CircuitNetList & netlist, const LibertyLibrary * lib, const Parasitics * _parasitics, const DesignConstraints * sdc);
		virtual ~TimingAnalysis();
		void fullTimingAnalysis();


		// GETTERS
        size_t timing_points_size() { return _points.size(); }
        const TimingPoint & timing_point( const int i ) { return _points.at(i); }

        size_t timing_arcs_size() { return _arcs.size(); }
        const TimingArc & timing_arc( const int i ) { return _arcs.at(i); }

        size_t timing_nets_size() { return _nets.size(); }
        const TimingNet & timing_net( const int i ) { return _nets.at(i); }

        double pin_capacitance(const int timing_point_index) const;
        double pin_load(const int timing_point_index) const;
        const Option & gate_option(const int gate_index);

		// SETTERS
        bool gate_option(const int gate_index, const int option_number);

        // GETTERS

        void print_info();
        void print_circuit_info();

        // DEBUG
        bool validate_with_prime_time();


        // CONSTANTS
        static const Transitions<double> ZERO_TRANSITIONS;
        static const Transitions<double> MIN_TRANSITIONS;
        static const Transitions<double> MAX_TRANSITIONS;
	};

};

#endif
