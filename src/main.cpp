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


//		cout << "Verilog file: " << verilog << endl;
//		cout << "Spef File: " << spef << endl;
//		cout << "SDC File: " << designConstraints << endl;
//		cout << "Liberty File: " << liberty << endl;
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

    cout << "#### " << Traits::ispd_contest_benchmark << endl;
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

    Timer timer;
    timer.start();
    ta.full_timing_analysis();
    timer.end();

//    ta.print_info();
	
//    ta.validate_with_prime_time();


//    ta.print_effective_capacitances();

//    pair<pair<int, int>, pair<Transitions<double>, Transitions<double> > > first_error = ta.check_ceffs(0.01f);
//    if(first_error.first.second < numeric_limits<int>::max())
//        cout << "ceffs NOT OK with primetime!\nfirst error = " << ta.timing_point(first_error.first.first).name() << "\n" << "tool ceff " << first_error.second.first << " pt ceff " << first_error.second.second << " error " << abs(first_error.second.first-first_error.second.second)/max(abs(first_error.second.first), abs(first_error.second.second)) << endl;
//    else
//        cout << "ceffs OK with primetime!" << endl;

//    cout << "runtime " << timer.value(Timer::MICRO) << endl;
//    cout << endl << endl;
	return 0;
}
