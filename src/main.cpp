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
	SpefParser sp;
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

	// cout << "netlist:" << endl;
	// cout << netlist << endl;

	TimingAnalysis::TimingAnalysis ta(netlist, &library, &parasitics, &constraints);

	cout << "TIMING POINTS " << endl;
	for(size_t i = 0; i < ta.timingPointsSize(); i++)
	{
		cout << "  " <<  ta.timingPoint(i) << endl;
	}

	cout << "TIMING ARCS " << endl;
	for(size_t i = 0; i < ta.timingArcsSize(); i++)
	{
		cout << "  " << ta.timingArc(i) << endl;
	}

	cout << "TIMING NETS " << endl;
	for(size_t i = 0; i < ta.timingNetsSize(); i++)
	{
		cout << "  " << ta.timingNet(i).getName() << " net ";
 		cout << (ta.timingNet(i).getFrom() != 0 ? ta.timingNet(i).getFrom()->getName() : "source") << " -> ( ";
 		if(ta.timingNet(i).fanoutsSize() == 0)
 			cout << "sink ";
 		for(int j = 0; j < ta.timingNet(i).fanoutsSize(); j++)
 		{
 			cout << ta.timingNet(i).getTo(j)->getName() << " ";
 		}
 		cout << ")" << endl;
	}

	// for(int i = 0; i < ta.getNumberOfNodes(); i++)
	// 	ta.setNodeOption(i, 0);

	// ta.fullTimingAnalysis();
	// // ta.printInfo();
	// ta.printCircuitInfo();

	cout << "-- DONE!" << endl;

	return 0;
}