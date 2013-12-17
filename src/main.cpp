#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include "include/timing_analysis.h"
#include "include/parser.h"
#include "include/circuit_netlist.h"
#include "include/spef_net.h"

#include "include/configuration.h"
#include "include/timer.h"
#include "include/ceff_ratio_experiment.h"
#include "include/slew_degradation_experiment.h"

#include <cstdio>
#include <queue>
using std::priority_queue;

#include <ostream>
using std::ostream;

using std::make_pair;



struct PassingArgs {
    string _contest_root;
    string _contest_benchmark;
    PassingArgs(string contestRoot,	string contestBenchmark) : _contest_root(contestRoot), _contest_benchmark(contestBenchmark){}
};

struct ISPDContestFiles
{
    string verilog;
    string spef;
    string liberty;
    string designConstraints;
    ISPDContestFiles(string contestRoot, string contestBenchmark) {
        verilog = contestRoot + "/" + contestBenchmark + "/" + contestBenchmark + ".v";
        spef = contestRoot + "/" + contestBenchmark + "/" + contestBenchmark + ".spef";
        designConstraints = contestRoot + "/" + contestBenchmark + "/" + contestBenchmark + ".sdc";
        liberty = contestRoot + "/lib/contest.lib";
    }
};

int main(int argc, char const *argv[])
{
    if(argc != 3)
    {
        cerr << "Using: " << argv[0] << " <CONTEST_ROOT> <CONTEST_BENCHMARK>" << endl;
        return -1;
    }

    const PassingArgs args(argv[1], argv[2]);
    Traits::ispd_contest_root = argv[1];
    Traits::ispd_contest_benchmark = argv[2];

    VerilogParser vp;
    LibertyParser lp;
    SpefParser sp;
    SDCParser dcp;

    ISPDContestFiles files(argv[1], argv[2]);

    const Circuit_Netlist netlist = vp.readFile(files.verilog);
    const LibertyLibrary library = lp.readFile(files.liberty);
    const Parasitics parasitics = sp.readFile(files.spef);
    const Design_Constraints constraints = dcp.readFile(files.designConstraints);

    Timing_Analysis::Timing_Analysis ta(netlist, &library, &parasitics, &constraints);
    cout << "Slew Violations: " << ta.slew_violations() << endl;
    cout << "Capacitance Violations: " << ta.capacitance_violations() << endl;
    cout << "TNS: " << ta.total_negative_slack() << endl;

//    ta.set_all_gates_to_max_size();
//    ta.full_timing_analysis();
//    ta.write_timing_file("min_power.timing");

//    ta.incremental_timing_analysis(ta.number_of_gates()/2, 15);
//    ta.write_timing_file("after_change_inc.timing");
//    ta.full_timing_analysis();
//    ta.write_timing_file("after_change.timing");

//    bool ok = ta.check_timing_file("after_change_inc.timing");
//    assert(ok);
//    cout << "OK!" << endl;

//    cout << "number of gates " << ta.number_of_gates() << endl;
//    cout << "timing points   " << ta.timing_points_size() << endl;
//    cout << "timing nets     " << ta.timing_nets_size() << endl;
//    cout << "timing arcs     " << ta.timing_arcs_size() << endl;


//    cout << "gates " << endl;
//    for (int i = 0; i < ta.number_of_gates(); ++i) {
//        cout << " #" << i << endl;
//        cout << "  footprint index " << ta.option(i).footprint_index() << endl;
//        cout << "  option index    " << ta.option(i).option_index() << endl;
//        cout << "  is dont touch?  " << (ta.option(i).is_dont_touch() ? "yes" : "no") << endl;
//    }

//    cout << "timing points " << endl;
//    for (int i = 0; i < ta.timing_points_size(); ++i) {

//        string type;
//        if(ta.timing_point(i).is_input_pin())
//            type = "input_pin";
//        else if(ta.timing_point(i).is_output_pin())
//            type = "output_pin";
//        else if(ta.timing_point(i).is_PI())
//            type = "PI";
//        else if(ta.timing_point(i).is_PI_input())
//            type = "PI_input";
//        else if(ta.timing_point(i).is_PO())
//            type = "PO";
//        else if(ta.timing_point(i).is_reg_input())
//            type = "reg_input";
//        else
//            type = "__UNKNOWN__";
//        cout << " #" << i << endl;
//        cout << "  name        " << ta.timing_point(i).name() << endl;
//        cout << "  gate number " << ta.timing_point(i).gate_number() << endl;
//        cout << "  type        " << type << endl;
//    }

    return 0;
}
