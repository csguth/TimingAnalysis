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
//    ta.full_timing_analysis();
    ta.call_prime_time();
    ta.print_circuit_info();

    return 0;
}
