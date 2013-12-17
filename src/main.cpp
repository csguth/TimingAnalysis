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
    ta.full_timing_analysis();
    cout << "Slew Violations: " << ta.slew_violations() << endl;
    cout << "Capacitance Violations: " << ta.capacitance_violations() << endl;
    cout << "TNS: " << ta.total_negative_slack() << endl;

    //    ta.print_gates();

    /*
618	g1661_u0
614	g1680_u0
571	g1681_u0
613	g1695_u0
466	g1696_u0
527	g1697_u0
615	g1702_u0
605	g1703_u0
460	g1707_u0
603	g1709_u0
518	g1711_u0
365	g1717_u0
454	g1718_u0
389	g1719_u0
     */



    ta.print_circuit_info();

//    cout << "PO arrival times\n\n\n";

//    ta.print_PO_arrivals();


    cout << "changing gate 518 to option 15 ... \n\n\n";
//    assert(ta.option(518, 15));
    ta.incremental_timing_analysis(518, 15);
    ta.print_circuit_info();

//    ta.print_PO_arrivals();


    cout << "sta!\n\n\n";
    ta.full_timing_analysis();
    ta.print_circuit_info();


    return 0;
}
