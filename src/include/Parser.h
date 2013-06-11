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

#include <cstdlib>

#include "CircuitNetList.h"
#include "LibertyLibrary.h"

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

// See test_lib_parser () function in parser_helper.cpp for an
// example of how to use this class.
class LibertyParser : public Parser {

  void _skip_lut_3D () ;
  void _begin_read_lut (LibertyLookupTable& lut) ;
  void _begin_read_timing_info (string pinName, LibertyTimingInfo& cell) ;
  void _begin_read_pin_info (string pinName, LibertyCellInfo& cell, LibertyPinInfo& pin) ;
  void _begin_read_cell_info (string cellName, LibertyCellInfo& cell) ;
    // Read the default max_transition defined for the library.
  // Return value indicates if the last read was successful or not.  
  // This function must be called in the beginning before any read_cell_info function call.
  bool read_default_max_transition (double& maxTransition) ;

  
  // Read the next standard cell definition.
  // Return value indicates if the last read was successful or not.  
  bool read_cell_info (LibertyCellInfo& cell) ;
public:

  const LibertyLibrary readFile(const string filename);

};


#endif