#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include "include/TimingAnalysis.h"
#include "include/Parser.h"
#include "include/CircuitNetList.h"
#include "include/SpefNet.h"

using std::make_pair;

struct PassingArgs {
	string contestRoot;
	string contestBenchmark;
	PassingArgs(string contestRoot,	string contestBenchmark) : contestRoot(contestRoot), contestBenchmark(contestBenchmark){};
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

	VerilogParser vp;
	LibertyParser lp;
	SpefParserISPD2013 sp;
	SDCParser dcp;

	
	ISPDContestFiles files(argv[1], argv[2]);
	

	const CircuitNetList netlist = vp.readFile(files.verilog);
	const LibertyLibrary library = lp.readFile(files.liberty);
	const Parasitics parasitics = sp.readFile(files.spef);
	const DesignConstraints constraints = dcp.readFile(files.designConstraints);


	// cout << "printing cell in01 option 13" << endl;
	// const LibertyCellInfo & cell = library.getCellInfo("in01", 13);
	// cout << (LibertyCellInfo &) cell << endl;

	// cout << "printing cell in01m01" << endl;
	// const LibertyCellInfo & cell2 = library.getCellInfo("in01m01");
	// cout << (LibertyCellInfo &) cell2 << endl;

	cout << "netlist:" << endl;
	cout << netlist << endl;

	cout << "OK" << endl;

	TimingAnalysis::TimingAnalysis ta(netlist, &library, &parasitics, &constraints);
	ta.fullTimingAnalysis();
	ta.printInfo();

	return 0;
}