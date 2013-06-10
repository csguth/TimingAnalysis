#include "include/Parser.h"

Parser::Parser()
{

}

Parser::~Parser()
{

}

bool Parser::isSpecialChar(const char & c)
{
	static const char specialChars[] =
	{ '(', ')', ',', ':', ';', '/', '#', '[', ']', '{', '}', '*', '\"', '\\' };

	for (unsigned i = 0; i < sizeof(specialChars); ++i)
	{
		if (c == specialChars[i])
			return true;
	}

	return false;	
}


bool Parser::readLineAsTokens(istream& is, vector<string>& tokens, bool includeSpecialChars)
{
	tokens.clear() ;

	string line ;
	std::getline (is, line) ;

	while (is && tokens.empty()) {

		string token = "" ;

		for (int i=0; i < line.size(); ++i) {
			char currChar = line[i] ;
			bool _isSpecialChar = isSpecialChar(currChar) ;

			if (std::isspace (currChar) || _isSpecialChar) {

			if (!token.empty()) {
				// Add the current token to the list of tokens
				tokens.push_back(token) ;
				token.clear() ;
			}

			if (includeSpecialChars && _isSpecialChar) {
				tokens.push_back(string(1, currChar)) ;
			}

			} else {
				// Add the char to the current token
				token.push_back(currChar) ;
			}

		}

		if (!token.empty())
			tokens.push_back(token) ;


		if (tokens.empty())
			// Previous line read was empty. Read the next one.
			std::getline (is, line) ;
	}

	//for (int i=0; i < tokens.size(); ++i)
	//  cout << tokens[i] << " " ;
	//cout << endl ;

	return !tokens.empty() ;
}


// VERILOG PARSER
const string VerilogParser::SEQUENTIAL_CELL = "ms00f80";
const string VerilogParser::INPUT_DRIVER_CELL = "in01f80";
const string VerilogParser::PRIMARY_OUTPUT_CELL = "__PO__";
const string VerilogParser::CLOCK_NET = "ispd_clk";

const CircuitNetList VerilogParser::readFile(const string filename)
{
	is.open(filename.c_str(), fstream::in);
	string moduleName;
	bool valid = read_module(moduleName);
	assert(valid);

	CircuitNetList netlist;

	// cout << "Module " << moduleName << endl << endl;

	do
	{
		string primaryInput;
		valid = read_primary_input(primaryInput);

		if (valid) 
		{
			// cout << "Primary input: " << primaryInput << endl;
			if(primaryInput != CLOCK_NET)
			{
				vector<std::pair<string, string> > piPins;
				piPins.push_back(make_pair("a", primaryInput + "_PI"));
				piPins.push_back(make_pair("o", primaryInput));
				netlist.addCellInst(primaryInput, INPUT_DRIVER_CELL, piPins, false, true);
			}
		}

	}
	while (valid);

	// cout << endl;

	do
	{
		string primaryOutput;
		valid = read_primary_output(primaryOutput);

		if (valid)
		{
			// cout << "Primary output: " << primaryOutput << endl;
			// netlist.addNet(primaryOutput);
			vector<std::pair<string, string> > poPins;
			poPins.push_back(make_pair("i", primaryOutput));
			poPins.push_back(make_pair("o", primaryOutput + "_PO"));
			netlist.addCellInst(primaryOutput, PRIMARY_OUTPUT_CELL, poPins);
		}

	}
	while (valid);

	// cout << endl;

	do
	{
		string net;
		valid = read_wire(net);

		if (valid)
		{
			// cout << "Net: " << net << endl;
			// netlist.addNet(net);
		}

	}
	while (valid);

	// cout << endl;
	// cout << "Cell insts: " << std::endl;

	do
	{
		string cellType, cellInst;
		vector<std::pair<string, string> > pinNetPairs;

		valid = read_cell_inst(cellType, cellInst, pinNetPairs);

		if (valid)
		{
			// cout << cellType << " " << cellInst << " ";
			// for (int i = 0; i < pinNetPairs.size(); ++i)
			// {
			// 	cout << "(" << pinNetPairs[i].first << " " << pinNetPairs[i].second << ") ";
			// }

			// cout << endl;
			const bool isSequential = ( cellType == SEQUENTIAL_CELL );
			if(isSequential)
			{
				for(vector<pair<string, string> >::iterator it = pinNetPairs.begin(); it != pinNetPairs.end(); it++)
				{
					if((*it).second == CLOCK_NET )
					{
						pinNetPairs.erase(it);
						break;
					}
				}
			}


			netlist.addCellInst(cellInst, cellType, pinNetPairs, isSequential);
		}


	}
	while (valid);
	is.close();

	for(size_t i = 0; i < netlist.getNetsSize(); i++)
	{
		const int sourceNodeIndex = netlist.getNet(i).sourceNode;
		const int sinkNodeIndex = (netlist.getNet(i).sinks.empty() ? -1 : netlist.getNet(i).sinks.front().gate);
		if( sourceNodeIndex == -1 || sinkNodeIndex == -1 )
			netlist.getNet(i).dummyNet = true;
	}

	netlist.updateTopology();

	return netlist;
}


bool VerilogParser::read_module(string& moduleName)
{

	vector<string> tokens;
	bool valid = readLineAsTokens(is, tokens);

	while (valid)
	{

		if (tokens.size() == 2 && tokens[0] == "module")
		{
			moduleName = tokens[1];

			break;
		}

		valid = readLineAsTokens(is, tokens);
	}

	// Read and skip the port names in the module definition
	// until we encounter the tokens {"Start", "PIs"}
	while (valid && !(tokens.size() == 2 && tokens[0] == "Start" && tokens[1] == "PIs"))
	{

		valid = readLineAsTokens(is, tokens);
		assert(valid);
	}

	return valid;
}

bool VerilogParser::read_primary_input(string& primaryInput)
{

	primaryInput = "";

	vector<string> tokens;
	bool valid = readLineAsTokens(is, tokens);

	assert(valid);
	assert(tokens.size() == 2);

	if (valid && tokens[0] == "input")
	{
		primaryInput = tokens[1];

	}
	else
	{
		assert(tokens[0] == "Start" && tokens[1] == "POs");
		return false;
	}

	return valid;
}

bool VerilogParser::read_primary_output(string& primaryOutput)
{

	primaryOutput = "";

	vector<string> tokens;
	bool valid = readLineAsTokens(is, tokens);

	assert(valid);
	assert(tokens.size() == 2);

	if (valid && tokens[0] == "output")
	{
		primaryOutput = tokens[1];

	}
	else
	{
		assert(tokens[0] == "Start" && tokens[1] == "wires");
		return false;
	}

	return valid;
}

bool VerilogParser::read_wire(string& wire)
{

	wire = "";

	vector<string> tokens;
	bool valid = readLineAsTokens(is, tokens);

	assert(valid);
	assert(tokens.size() == 2);

	if (valid && tokens[0] == "wire")
	{
		wire = tokens[1];

	}
	else
	{
		assert(tokens[0] == "Start" && (tokens[1] == "cells" || tokens[1] == "assigns"));
		return false;
	}

	return valid;
}

bool VerilogParser::read_assign(pair<string, string> & assignment)
{
	vector<string> tokens;
	bool valid = readLineAsTokens(is, tokens);

	assert(valid);
	assert(tokens.size() == 4 || tokens.size() == 2);

	if (valid && tokens[0] == "assign")
	{
		assignment.first = tokens[1];
		assignment.second = tokens[3];

	}
	else
	{
		assert(tokens[0] == "Start" && tokens[1] == "cells");
		return false;
	}

	return valid;
}
bool VerilogParser::read_cell_inst(string& cellType, string& cellInstName, vector<std::pair<string, string> >& pinNetPairs)
{

	cellType = "";
	cellInstName = "";
	pinNetPairs.clear();

	vector<string> tokens;
	bool valid = readLineAsTokens(is, tokens);

	assert(valid);

	if (tokens.size() == 1)
	{
		assert(tokens[0] == "endmodule");
		return false;
	}

	assert(tokens.size() >= 4);
	// We should have cellType, instName, and at least one pin-net pair

	cellType = tokens[0];
	cellInstName = tokens[1];

	for (int i = 2; i < tokens.size() - 1; i += 2)
	{

		assert(tokens[i][0] == '.');
		// pin names start with '.'
		string pinName = tokens[i].substr(1); // skip the first character of tokens[i]

		pinNetPairs.push_back(std::make_pair(pinName, tokens[i + 1]));
	}



	return valid;
}