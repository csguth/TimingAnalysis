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
/** @brief Namespace which contains classes relative to timing analysis graph model
*/
namespace Timing_Analysis
{
	/** @brief Refers to the implementation option of a logic gate
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
	/** @brief Class which performs the timing analysis
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


        int _first_PO_index;

        // PRIVATE GETTERS********************************************************************************************************
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

        // STATIC TIMING ANALYSIS**********************************************************************************************
		/** @brief Private. Updates effective capacitance, critical path transition, capacitive violations, of timing point at index i
		*
		* @param const int timing_point_index
		*
		* @return void
		*/
        void update_timing(const int timing_point_index);
		/** @brief Private. Updates slew vilations and total negative slack of timing point at index i
		*
		* @param const int timing_point_index
		*
		* @return void
		*/
        void update_slacks(const int timing_point_index);

		// TOPOLOGY INIT*********************************************************************************************************
		/** @brief Private. Creates timing points from gate 
		*
		* @param const int i,const Circuit_Netlist::Logic_Gate & gate,const pair<int, int> cellIndex, const LibertyCellInfo & cellInfo
		*
		* @return const pair<size_t, size_t>
		*/
        const pair<size_t, size_t> create_timing_points(const int i,const Circuit_Netlist::Logic_Gate & gate,const pair<int, int> cellIndex, const LibertyCellInfo & cellInfo);
		/** @brief Private.  Updates number of timing ponts and timing arcs from netlist
		*
		* @param int & numberOfTimingPoints, int & numberOfTimingArcs, const Circuit_Netlist & netlist, const LibertyLibrary * lib
		*
		* @return void
		*/
        void number_of_timing_points_and_timing_arcs(int & numberOfTimingPoints, int & numberOfTimingArcs, const Circuit_Netlist & netlist, const LibertyLibrary * lib);
		/** @brief Private. Creates timing arcs from tpIndexes
		*
		* @param const pair<size_t, size_t> tpIndexes, const bool is_pi , const bool is_po
		*
		* @return void
		*/
        void create_timing_arcs(const pair<size_t, size_t> tpIndexes, const bool is_pi , const bool is_po);

		// PRIMETIME CALLING********************************************************************************************************
		/** @brief Private. Creates an iterator to iterate the verilog descriptions vector
		*
		* @return void
		*/
        void get_sizes_vector();
		/** @brief Private. Checks timing file for errors and prints it
		*
		* @param const string timing_file
		*
		* @return bool
		*/
		bool check_timing_file(const string timing_file);

		// OUTPUT METHODS**************************************************************************************************************
		/** @brief Private. Inserts formatted description of the sizes of the verilog descriptions
		*
		* @param const string filename
		*
		* @return void
		*/
        void write_sizes_file(const string filename);

	public:
		/** @brief Timing_Analysis default constructor
		*
		* @param const Circuit_Netlist & netlist, const LibertyLibrary * lib, const Parasitics * _parasitics, const Design_Constraints * sdc
		*
		*/
        Timing_Analysis(const Circuit_Netlist & netlist, const LibertyLibrary * lib, const Parasitics * _parasitics, const Design_Constraints * sdc);
		/** @breaf Timing_Analysis empty destructor
		*/
        virtual ~Timing_Analysis();

        //
		/** @brief Calls prime time to set time values
		*
		* @return void
		*/
        void call_prime_time();
		/** @brief Updates all timing values of all elements
		*
		* @return void
		*/
        void full_timing_analysis();

		// GETTERS******************************************************************************
		/** @brief Returns number of timing points
		*
		* @return size_t
		*/
        size_t timing_points_size() { return _points.size(); }
		/** @brief Returns timing point reference at index i
		*
		* @param const int i
		*
		* @return const Timing_Point &
		*/
        const Timing_Point & timing_point( const int i ) { return _points.at(i); }
		/** @brief Returns number of timing arcs
		*
		* @return size_t
		*/
        size_t timing_arcs_size() { return _arcs.size(); }
		/** @brief Returns timing arc reference at index i
		*
 		* @param const int i
		*
		* @return const Timing_Arc &
		*/
        const Timing_Arc & timing_arc( const int i ) { return _arcs.at(i); }
		/** @brief Returns number of timing nets
		*
		* @return size_t
		*/
        size_t timing_nets_size() { return _nets.size(); }
		/** @brief Returns timing net reference at index i
		*
 		* @param const int i
		*
		* @return const Timing_Net &
		*/
        const Timing_Net & timing_net( const int i ) { return _nets.at(i); }

		/** @brief Returns pin capacitance at timing point index
		*
 		* @param const int timing_point_index 
		*
		* @return double
		*/
        double pin_capacitance(const int timing_point_index) const;
		/** @brief Returns pin load at timing point index
		*
 		* @param const int i
		*
		* @return const Timing_Net &
		*/
        double pin_load(const int timing_point_index) const;
		/** @brief Returns option index at gate_number index
		*
 		* @param const int gate_number
		*
		* @return int
		*/
        int option(const int gate_number);
        size_t number_of_options(const int gate_index);

		/** @brief Returns first primary output index
		*
		* @return int
		*/
        int first_PO_index() const { return _first_PO_index; }

		// SETTERS*******************************************************************************
		/** @brief Returns true if gate_index option is "don't touch"
		*
 		* @param const int gate_index, const int option
		*
		* @return bool
		*/
        bool option(const int gate_index, const int option);

        // DEBUG*********************************************************************************
		/** @brief Validates timing_file with prime time, prints results and returns true
		*
		* @return bool
		*/
        bool validate_with_prime_time();
		/** @brief Prints graph and circuit info
		*
		* @return void
		*/
        void print_info();
		/** @brief Prints timing info
		*
		* @return void
		*/
        void print_circuit_info();
		/** @brief Prints results from analysis
		*
		* @return void
		*/
        void report_timing();


	};

};

#endif
