#ifndef PARSER_H_
#define PARSER_H_

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <istream>
using std::istream;

#include <fstream>
using std::fstream;

#include <cassert>

#include "CircuitNetList.h"

class Parser
{
protected:
	bool isSpecialChar(const char & c);
	bool readLineAsTokens(istream& is, vector<string>& tokens, bool includeSpecialChars = false);
	fstream is;
public:
	Parser();
	virtual ~Parser();
};


class VerilogParser : public Parser
{
	static const string SEQUENTIAL_CELL;
	static const string INPUT_DRIVER_CELL;
	static const string PRIMARY_OUTPUT_CELL;
	static const string CLOCK_NET;

	// Read the module definition
	bool read_module(string& moduleName);

	// Read the next primary input.
	// Return value indicates if the last read was successful or not.
	bool read_primary_input(string& primaryInput);

	// Read the next primary output.
	// Return value indicates if the last read was successful or not.
	bool read_primary_output(string& primaryInput);

	// Read the next net.
	// Return value indicates if the last read was successful or not.
	bool read_wire(string& wire);

	// Read the next cell instance.
	// Return value indicates if the last read was successful or not.
	bool read_cell_inst(string& cellType, string& cellInstName, vector<std::pair<string, string> >& pinNetPairs);
	bool read_assign(pair<string, string> & assignment);
public:
	const CircuitNetList readFile(const string filename);
	

	virtual ~VerilogParser()
	{

	};
};

#endif