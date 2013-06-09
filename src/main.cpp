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
	vector<pair<string, string> > input;
	input.push_back(make_pair("a", "inp1"));
	input.push_back(make_pair("b", "inp2"));
	input.push_back(make_pair("o", "n1"));
	netlist.addCellInst("u1", "na02f01", input);

	input.clear();
	input.push_back(make_pair("d", "n1"));
	input.push_back(make_pair("clk", "ispd_clk"));
	input.push_back(make_pair("q", "n2"));
	netlist.addCellInst("f1", "ms00", input);

	input.clear();
	input.push_back(make_pair("a", "n2"));	
	input.push_back(make_pair("o", "out"));
	netlist.addCellInst("u2", "in01f01", input);

	cout << netlist << endl;

	cout << "fim" << endl;

	return 0;
}