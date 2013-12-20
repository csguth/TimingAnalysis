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

#include <stack>
using std::stack;

#include <set>
using std::set;

#include <queue>
using std::priority_queue;

#include <cstdlib>

#include "timer_interface.h"

#include "transitions.h"
#include "circuit_netlist.h"
#include "wire_delay_model.h"
#include "liberty_library.h"
#include "design_constraints.h"

#include "configuration.h"

#include "parser.h"


#include "timing_net.h"
#include "timing_point.h"
#include "timing_net.h"

namespace Timing_Analysis
{

    class ita_comparator {

    public:
        bool operator()(Timing_Point * a, Timing_Point * b);
    };

    class Option
	{
        friend class Timing_Analysis;
        int _footprint_index;
        int _option_index;
        bool _dont_touch;

    public:
        Option():_footprint_index(-1), _option_index(-1), _dont_touch(false){}
        Option(const int footprintIndex, const int optionIndex) : _footprint_index(footprintIndex), _option_index(optionIndex), _dont_touch(false){}
        int footprint_index() const;
        int option_index() const;
        bool is_dont_touch() const;
	};


    class Timing_Net;
    class Timing_Arc;
    class Timing_Analysis
	{


        vector<Timing_Point> _points;
        vector<Timing_Arc> _arcs;
        vector<Timing_Net> _nets;
        vector<Option> _options;
        map<int, double> _PO_loads;
        map<string, int> _pin_name_to_timing_point_index;
        vector<pair<size_t, size_t> > _gate_index_to_timing_point_index;

        vector<pair<int, string> > _verilog;
        vector<pair<string, string> > _sizes;
        vector<bool> _dirty;

        const LibertyLibrary * _library;
        const Parasitics * _parasitics;
        LibertyLookupTableInterpolator * _interpolator;

        Transitions<double> _target_delay;
        Transitions<double> _max_transition;
        Transitions<double> _critical_path;
        Transitions<double> _total_negative_slack;
        Transitions<double> _worst_slack;
        Transitions<double> _slew_violations;
        Transitions<double> _capacitance_violations;

        unsigned _total_violating_POs;
        int _first_PO_index;


        void initialize_timing_data();

        // STATIC TIMING ANALYSIS
        void update_timing(const int timing_point_index);
        void update_slacks(const int timing_point_index);
        void clear_violations(const int timing_point_index);
        void update_violations(const int timing_point_index);

		// TOPOLOGY INIT
        const pair<size_t, size_t> create_timing_points(const int i,const Circuit_Netlist::Logic_Gate & gate,const pair<int, int> cellIndex, const LibertyCellInfo & cellInfo);
        void number_of_timing_points_and_timing_arcs(int & numberOfTimingPoints, int & numberOfTimingArcs, const Circuit_Netlist & netlist, const LibertyLibrary * lib);
        void create_timing_arcs(const pair<size_t, size_t> tpIndexes, const bool is_pi , const bool is_po );

		// PRIMETIME CALLING
        void get_sizes_vector();

		// OUTPUT METHODS
        void write_sizes_file(const string filename);

	public:
        Timing_Analysis(const Circuit_Netlist & netlist, const LibertyLibrary * lib, const Parasitics * _parasitics, const Design_Constraints * sdc);
        virtual ~Timing_Analysis();


        //

        void call_prime_time();
        void full_timing_analysis();
        void incremental_timing_analysis(int gate_number, int new_option);
        void update_timing_points(const Timing_Point * output_timing_point);

		// GETTERS
        size_t number_of_gates() const { return _options.size(); }

        size_t timing_points_size() { return _points.size(); }
        const Timing_Point & timing_point( const int i ) { return _points.at(i); }

        size_t timing_arcs_size() { return _arcs.size(); }
        const Timing_Arc & timing_arc( const int i ) { return _arcs.at(i); }

        size_t timing_nets_size() { return _nets.size(); }
        const Timing_Net & timing_net( const int i ) { return _nets.at(i); }

        double pin_capacitance(const int timing_point_index) const;
        double pin_load(const int timing_point_index) const;
        const Option & option(const int gate_number);
        const LibertyCellInfo & liberty_cell_info(int gate_index, int option_index = -1) const;

        size_t number_of_options(const int gate_index);

        Transitions<double> total_negative_slack() const { return _total_negative_slack; }
        Transitions<double> worst_slack() const { return _worst_slack; }
        Transitions<double> target_delay() const { return _target_delay; }
        unsigned total_violating_POs() const { return _total_violating_POs; }
        Transitions<double> critical_path() const { return _critical_path; }
        Transitions<double> capacitance_violations() const { return _capacitance_violations; }
        Transitions<double> slew_violations() const { return _slew_violations; }
        int output_timing_point_index(int gate_number);

        bool has_capacitance_violations(const Timing_Point & tp);
        bool has_slew_violations(const Timing_Point & tp);

        set<int> timing_points_in_longest_path();
        set<int> timing_points_in_critical_path();

        bool has_timing_violations();

        const Transitions<double> calculate_timing_arc_delay(const Timing_Arc & timing_arc, const Transitions<double> transition, const Transitions<double> ceff);


        int first_PO_index() const { return _first_PO_index; }

		// SETTERS
        bool option(const int gate_index, const int option);
        void target_delay(Transitions<double> target_delay) { _target_delay = target_delay; }
        void set_all_gates_to_max_size();
        void set_all_gates_to_min_size();

        // DEBUG
        bool validate_with_prime_time();
        void print_info();
        void print_circuit_info();
        void report_timing();
        void print_PO_arrivals();
        void print_effective_capacitances();
        void write_timing_file(const string filename);
        bool check_timing_file(const string timing_file);
        pair<pair<int, int>, pair<Transitions<double>, Transitions<double> > > check_ceffs(double precision);
        void print_gates();


    };

};

#endif
