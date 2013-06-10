#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include "include/TimingAnalysis.h"
#include "include/Parser.h"
#include "include/CircuitNetList.h"

using std::make_pair;

struct PassingArgs {
	string contestRoot;
	string contestBenchmark;
	PassingArgs(string contestRoot,	string contestBenchmark) : contestRoot(contestRoot), contestBenchmark(contestBenchmark){};
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

	const string verilogFile = args.contestRoot + "/" + args.contestBenchmark + "/" + args.contestBenchmark + ".v";
	const CircuitNetList netlist = vp.readFile(verilogFile);

	TimingAnalysis::TimingAnalysis ta(netlist);
	cout << "-- Timing TimingAnalysis Topology ("<<ta.getNumberOfNodes() << " nodes)" << endl;
	for(size_t i = 0; i < ta.getNumberOfNodes(); i++)
	{
		cout << "---- Node ["<<i<<"] = " <<  ta.getNodeName(i) << endl;
	}

	return 0;
}