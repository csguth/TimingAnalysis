#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include "include/TimingAnalysis.h"
#include "include/Parser.h"
#include "include/CircuitNetList.h"
#include "include/SpefNet.h"

#include "include/Configuration.h"

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


		cout << "Verilog file: " << verilog << endl;
		cout << "Spef File: " << spef << endl;
		cout << "SDC File: " << designConstraints << endl;
		cout << "Liberty File: " << liberty << endl;
	};
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
	

	const CircuitNetList netlist = vp.readFile(files.verilog);
	const LibertyLibrary library = lp.readFile(files.liberty);
	const Parasitics parasitics = sp.readFile(files.spef);
	const DesignConstraints constraints = dcp.readFile(files.designConstraints);

	TimingAnalysis::TimingAnalysis ta(netlist, &library, &parasitics, &constraints);



    for(size_t i = 0; i < ta.timing_points_size(); i++)
	{	
        const TimingAnalysis::TimingPoint & tp = ta.timing_point(i);
		// cout << "tp " << tp.getName() << " gate number = " << tp.getGateNumber() << endl;
		// cout << "setting gate " << tp.getGateNumber() << " option to 0"<< endl;
        cout << "tp " << tp.name() << ":: changing gate " << tp.gate_number() << " option to 0";
        if( !ta.gate_option(tp.gate_number(), 0) )
            cout << ", but it's a don't touch gate!!";
        cout << endl;

		
	}


    for(size_t i = 0; i < ta.timing_points_size(); i++)
    {
        const TimingAnalysis::TimingPoint & tp = ta.timing_point(i);
        cout << "option vector["<< tp.name() << "] = " << ta.gate_option(tp.gate_number()).option_index << endl;
    }


	ta.fullTimingAnalysis();

	
	ta.validate_with_prime_time();

    ta.print_info();
    ta.print_circuit_info();

	cout << "-- DONE!" << endl;

	return 0;
}
