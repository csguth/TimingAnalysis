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
	/** @brief 
	*/
    class Option
	{
        friend class Timing_Analysis;
        int _footprint_index;
        int _option_index;
        bool _dont_touch;

    public:
		/** @brief Option default constructor
		*/
        Option():_footprint_index(-1), _option_index(-1), _dont_touch(false){}
		/** @brief Option default constructor
		*
		* @param const int footprintIndex, const int optionIndex
		*/
        Option(const int footprintIndex, const int optionIndex) : _footprint_index(footprintIndex), _option_index(optionIndex), _dont_touch(false){}

	};

    class Timing_Net;
    class Timing_Arc;
	/** @brief 
	*/
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
        Transitions<double> _slew_violations;
        Transitions<double> _capacitance_violations;

        map<string, Transitions<double> > _max_ceff;
        map<string, Transitions<double> > _min_ceff;

        // PRIVATE GETTERS

		/** @brief Private. Returns LibertyCellInfo object reference at node_index
		*
		* @param const int node_index
		*
		* @return const LibertyCellInfo &
		*/
        const LibertyCellInfo & liberty_cell_info(const int node_index) const;
		/** @brief Private. Returns Transitions containing the timing arc delay calculated by interpolating both parameters values using the time values table of the timing_arc
		*
		* @param const Timing_Arc & timing_arc, const Transitions<double> transition, const Transitions<double> ceff
		*
		* @return const Transitions<double>
		*/
        const Transitions<double> calculate_timing_arc_delay(const Timing_Arc & timing_arc, const Transitions<double> transition, const Transitions<double> ceff);

        // STATIC TIMING ANALYSIS
		/** @brief Private. 
		*
		* @param const int timing_point_index
		*
		* @return void
		*/
        void update_timing(const int timing_point_index);
        void update_slacks(const int timing_point_index);

		// TOPOLOGY INIT
        const pair<size_t, size_t> create_timing_points(const int i,const Circuit_Netlist::Logic_Gate & gate,const pair<int, int> cellIndex, const LibertyCellInfo & cellInfo);
        void number_of_timing_points_and_timing_arcs(int & numberOfTimingPoints, int & numberOfTimingArcs, const Circuit_Netlist & netlist, const LibertyLibrary * lib);
        void create_timing_arcs(const pair<size_t, size_t> tpIndexes, const bool is_pi , const bool is_po );

		// PRIMETIME CALLING
        void get_sizes_vector();
		bool check_timing_file(const string timing_file);

		// OUTPUT METHODS
        void write_sizes_file(const string filename);

	public:
        Timing_Analysis(const Circuit_Netlist & netlist, const LibertyLibrary * lib, const Parasitics * _parasitics, const Design_Constraints * sdc);
        virtual ~Timing_Analysis();

        //

        void call_prime_time();
        void full_timing_analysis();

		// GETTERS
        size_t timing_points_size() { return _points.size(); }
        const Timing_Point & timing_point( const int i ) { return _points.at(i); }

        size_t timing_arcs_size() { return _arcs.size(); }
        const Timing_Arc & timing_arc( const int i ) { return _arcs.at(i); }

        size_t timing_nets_size() { return _nets.size(); }
        const Timing_Net & timing_net( const int i ) { return _nets.at(i); }

        double pin_capacitance(const int timing_point_index) const;
        double pin_load(const int timing_point_index) const;
        int option(const int gate_number);

		// SETTERS
        bool option(const int gate_index, const int option);

        // DEBUG
        bool validate_with_prime_time();
        void print_info();
        void print_circuit_info();
        void report_timing();


	};

};

#endif
