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

//    cout << endl << "## " << Traits::ispd_contest_benchmark << endl;
    Timer timer;
    timer.start();
    ta.full_timing_analysis();
//    ta.call_prime_time();
    timer.end();
//    cout << "runtime " << timer.value(Timer::MICRO) << endl;

//    if(Traits::ispd_contest_benchmark == "usb_phy_slow")
//        printf("    BENCHMARK NAME |         TNS |     Viol. POs |     Crit. Rise |     Crit. Fall |     Target |  Runtime (s)\n");
//    printf("%18s |%12.1lf |%14d |%15.2lf |%15.2lf |%11.2lf |%13.2lf\n", Traits::ispd_contest_benchmark.c_str(), ta.total_negative_slack().aggregate(), ta.total_violating_POs(), ta.critical_path().getRise(), ta.critical_path().getFall(), ta.target_delay().getRise(), timer.value(Timer::SECOND).time());


//    Ceff_Ratio_Experiment::run_sorted_by_wire_size(ta);
//    Ceff_Ratio_Experiment::run_sorted_by_slew(ta);

//    Slew_Degradation_Experiment::run(ta);


	return 0;
}
