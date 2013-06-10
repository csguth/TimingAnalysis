#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include "include/TimingAnalysis.h"
#include "include/Parser.h"

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

	cout << "Olá" << endl;
	
	CircuitNetList netlist;
	VerilogParser vp;

	const string verilogFile = args.contestRoot + "/" + args.contestBenchmark + "/" + args.contestBenchmark + ".v";
	vp.readFile(verilogFile, netlist);


	cout << netlist << endl;

	cout << "gates size: " << netlist.getGatesSize() << endl;
	cout << "nets size: " << netlist.getNetsSize() << endl;

	cout << "fim" << endl;

	return 0;
}