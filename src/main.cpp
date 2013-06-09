#include <iostream>
using std::cout;
using std::endl;

#include "include/TimingAnalysis.h"

int main(int argc, char const *argv[])
{
	cout << "Olá" << endl;

	TimingAnalysis::TimingAnalysis ta;
	for(unsigned i = 0; i < ta.getNumberOfNodes(); i++)
	{
		cout << ta.getNodeName(i) << endl;
	}

	return 0;
}