#include <iostream>
using std::cout;
using std::endl;

#include "include/TimingAnalysis.h"
#include "include/Parser.h"

using std::make_pair;

int main(int argc, char const *argv[])
{
	cout << "Olá" << endl;

	TimingAnalysis::TimingAnalysis ta;
	
	CircuitNetList netlist;
	
	VerilogParser vp;
	vp.readFile("/home/csguth/Documents/UFSC/tcc/implementacao/ispd2013/simple/simple.v", netlist);

	cout << netlist << endl;

	cout << "fim" << endl;

	return 0;
}