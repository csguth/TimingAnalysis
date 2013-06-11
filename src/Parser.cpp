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



// LIBERTY PARSER
// No need to parse the 3D LUTs, because they will be ignored
void LibertyParser::_skip_lut_3D () {

  std::vector<string> tokens ;
  
  bool valid = readLineAsTokens (is, tokens) ;
  assert (valid) ;
  assert (tokens[0] == "index_1") ;
  assert (tokens.size() >= 2) ;
  int size1 = tokens.size() - 1 ;

  valid = readLineAsTokens (is, tokens) ;
  assert (valid) ;
  assert (tokens[0] == "index_2") ;
  assert (tokens.size() >= 2) ;
  int size2 = tokens.size() - 1 ;  

  valid = readLineAsTokens (is, tokens) ;
  assert (valid) ;
  assert (tokens[0] == "index_3") ;
  assert (tokens.size() >= 2) ;
  int size3 = tokens.size() - 1 ;  

  valid = readLineAsTokens (is, tokens) ;
  assert (valid) ;
  assert (tokens.size() == 1 && tokens[0] == "values") ;
  
  for (int i=0; i < size1; ++i) {
    for (int j=0; j < size2; ++j) {

      valid = readLineAsTokens (is, tokens) ;
      assert (valid) ;
      assert (tokens.size() == size3) ;
    }
  }

}

void LibertyParser::_begin_read_lut (LibertyLookupTable& lut) {

  std::vector<string> tokens ;
  bool valid = readLineAsTokens (is, tokens) ;

  assert (valid) ;
  assert (tokens[0] == "index_1") ;
  assert (tokens.size() >= 2) ;
  
  int size1 = tokens.size()-1 ;
  lut.loadIndices.resize(size1) ;
  for (int i=0; i < tokens.size()-1; ++i) {
    
    lut.loadIndices[i] = std::atof(tokens[i+1].c_str()) ;
  }

  valid = readLineAsTokens (is, tokens) ;

  assert (valid) ;
  assert (tokens[0] == "index_2") ;
  assert (tokens.size() >= 2) ;

  int size2 = tokens.size()-1 ;
  lut.transitionIndices.resize(size2) ;
  for (int i=0; i < tokens.size()-1; ++i) {
    
    lut.transitionIndices[i] = std::atof(tokens[i+1].c_str()) ;
  }
  
  valid = readLineAsTokens (is, tokens) ;
  assert (valid) ;
  assert (tokens.size() == 1 && tokens[0] == "values") ;

  lut.tableVals.resize(size1) ;
  for (int i=0 ; i < lut.loadIndices.size(); ++i) {
    valid = readLineAsTokens (is, tokens) ;
    assert (valid) ;
    assert (tokens.size() == lut.transitionIndices.size()) ;

    lut.tableVals[i].resize(size2) ;
    for (int j=0; j < lut.transitionIndices.size(); ++j) {
      lut.tableVals[i][j] = std::atof(tokens[j].c_str()) ;

    }
  }

  
}

void LibertyParser::_begin_read_timing_info (string toPin, LibertyTimingInfo& timing) {

  timing.toPin = toPin ;
  
  bool finishedReading = false ;

  std::vector<string> tokens ;  
  while (!finishedReading) {

    bool valid = readLineAsTokens (is, tokens) ;
    assert (valid) ;
    assert (tokens.size() >= 1) ;

    if (tokens[0] == "cell_fall") {
      _begin_read_lut (timing.fallDelay) ;

    } else if (tokens[0] == "cell_rise") {
      _begin_read_lut (timing.riseDelay) ;

    } else if (tokens[0] == "fall_transition") {
      _begin_read_lut (timing.fallTransition) ;
      
    } else if (tokens[0] == "rise_transition") {
      _begin_read_lut (timing.riseTransition) ;

    } else if (tokens[0] == "fall_constraint") {

      _skip_lut_3D() ; // will ignore fall constraints      

    } else if (tokens[0] == "rise_constraint") {

      _skip_lut_3D() ; // will ignore rise constraints

    } else if (tokens[0] == "timing_sense") {
      timing.timingSense = tokens[1] ;

    } else if (tokens[0] == "related_pin") {

      assert (tokens.size() == 2) ;
      timing.fromPin = tokens[1] ;

    } else if (tokens[0] == "End") {

      assert (tokens.size() == 2) ;
      assert (tokens[1] == "timing") ;
      finishedReading = true ;
      
    } else if (tokens[0] == "timing_type") {
      // ignore data

    } else if (tokens[0] == "related_output_pin") {
      // ignore data

    } else {

      cout << "Error: Unknown keyword: " << tokens[0] << endl ;
      assert (false) ; // unknown keyword
    }
    
  }


}


void LibertyParser::_begin_read_pin_info (string pinName, LibertyCellInfo& cell, LibertyPinInfo& pin) {

  pin.name = pinName ;
  pin.isClock = false ;
  pin.maxCapacitance = std::numeric_limits<double>::max() ;
  
  bool finishedReading = false ;

  std::vector<string> tokens ;  
  while (!finishedReading) {

    bool valid = readLineAsTokens (is, tokens) ;
    assert (valid) ;
    assert (tokens.size() >= 1) ;

    if (tokens[0] == "direction") {

      assert (tokens.size() == 2) ;
      if (tokens[1] == "input")
        pin.isInput = true ;
      else if (tokens[1] == "output")
        pin.isInput = false ;
      else
        assert (false) ; // undefined direction

    } else if (tokens[0] == "capacitance") {

      assert (tokens.size() == 2) ;
      pin.capacitance = std::atof(tokens[1].c_str()) ;

    } else if (tokens[0] == "max_capacitance") {

      assert (tokens.size() == 2) ;
      pin.maxCapacitance = std::atof(tokens[1].c_str()) ;


    } else if (tokens[0] == "timing") {

      cell.timingArcs.push_back(LibertyTimingInfo()) ; // add an empty TimingInfo object
      _begin_read_timing_info (pinName, cell.timingArcs.back()) ; // pass the empty object to the function to be filled

    } else if (tokens[0] == "clock") {

      pin.isClock = true ;

    } else if (tokens[0] == "End") {

      assert (tokens.size() == 2) ;
      assert (tokens[1] == "pin") ;
      finishedReading = true ;

    } else if (tokens[0] == "function") {

      // ignore data

    } else if (tokens[0] == "min_capacitance") {

      // ignore data

    } else if (tokens[0] == "nextstate_type") {

      // ignore data

    } else {
      cout << "Error: Unknown keyword: " << tokens[0] << endl ;      
      assert (false) ; // unknown keyword 

    }

  }


}

void LibertyParser::_begin_read_cell_info (string cellName, LibertyCellInfo& cell) {

  cell.name = cellName ;
  cell.isSequential = false ;
  cell.dontTouch = false ;
  
  bool finishedReading = false ;

  std::vector<string> tokens ;  
  while (!finishedReading) {

    bool valid = readLineAsTokens (is, tokens) ;
    assert (valid) ;
    assert (tokens.size() >= 1) ;
    
    if (tokens[0] == "cell_leakage_power") {

      assert (tokens.size() == 2) ;
      cell.leakagePower = std::atof(tokens[1].c_str()) ;

    } else if (tokens[0] == "cell_footprint") {
        
      assert (tokens.size() == 2) ;
      cell.footprint = tokens[1] ;

    } else if (tokens[0] == "area") {

      assert (tokens.size() == 2) ;
      cell.area = std::atof(tokens[1].c_str()) ;

    } else if (tokens[0] == "clocked_on") {

      cell.isSequential = true ;

    } else if (tokens[0] == "dont_touch") {

      cell.dontTouch = true ;
      
    } else if (tokens[0] == "pin") {

      assert (tokens.size() == 2) ;

      cell.pins.push_back(LibertyPinInfo()) ; // add empty PinInfo object
      _begin_read_pin_info (tokens[1], cell, cell.pins.back()) ; // pass the new PinInfo object to be filled

    } else if (tokens[0] == "End") {

      assert (tokens.size() == 3) ;
      assert (tokens[1] == "cell") ;
      assert (tokens[2] == cellName) ;
      finishedReading = true ;

    } else if (tokens[0] == "cell_footprint") {

      // ignore data

    } else if (tokens[0] == "ff") {

      // ignore data

    } else if (tokens[0] == "next_state") {

      // ignore data

    } else if (tokens[0] == "dont_use") {

      // ignore data
      
    } else {

      cout << "Error: Unknown keyword: " << tokens[0] << endl ;
      assert (false) ; // unknown keyword
    }
  } 

}


// Read the default max_transition defined for the library.
// Return value indicates if the last read was successful or not.  
// This function must be called in the beginning before any read_cell_info function call.
bool LibertyParser::read_default_max_transition (double& maxTransition) {

  maxTransition = 0.0 ;
  vector<string> tokens ;

  bool valid = readLineAsTokens (is, tokens) ;

  while (valid) {

    if (tokens.size() == 2 && tokens[0] == "default_max_transition") {
      maxTransition = std::atof(tokens[1].c_str()) ;
      return true ;
    }

    valid = readLineAsTokens (is, tokens) ;
  }

  return false ;
}



// Read the next standard cell definition.
// Return value indicates if the last read was successful or not.  
bool LibertyParser::read_cell_info (LibertyCellInfo& cell) {

  vector<string> tokens ;
  bool valid = readLineAsTokens (is, tokens) ;


  while (valid) {

    if (tokens.size() == 2 && tokens[0] == "cell") {
      _begin_read_cell_info (tokens[1], cell) ;

      return true ;
    }

    valid = readLineAsTokens (is, tokens) ;
  }

  return false ;
}

ostream& operator<< (ostream& os, LibertyLookupTable& lut) {

  if (lut.loadIndices.empty() && lut.transitionIndices.empty() && lut.tableVals.empty())
    return os ;

    // We should have either all empty or none empty.
  assert (!lut.loadIndices.empty() && !lut.transitionIndices.empty() && !lut.tableVals.empty()) ;
  
  assert (lut.tableVals.size() == lut.loadIndices.size()) ;
  assert (lut.tableVals[0].size() == lut.transitionIndices.size()) ;

  os << "\t" ;
  for (int i=0; i < lut.transitionIndices.size(); ++i) {
    os << lut.transitionIndices[i] << "\t" ;
  }
  os << endl ;

  
  for (int i=0; i < lut.loadIndices.size(); ++i) {
    os << lut.loadIndices[i] << "\t" ;

    for (int j=0; j < lut.transitionIndices.size(); ++j)
      os << lut.tableVals[i][j] << "\t" ;

    os << endl ;
    
  }

  return os ;
}


ostream& operator<< (ostream& os, LibertyTimingInfo& timing) {

  os << "Timing info from " << timing.fromPin << " to " << timing.toPin << ": " << endl ;
  os << "Timing sense: " << timing.timingSense << endl ;

  os << "Fall delay LUT: " << endl ;
  os << timing.fallDelay ;

  os << "Rise delay LUT: " << endl ;
  os << timing.riseDelay ;
  
  os << "Fall transition LUT: " << endl ;
  os << timing.fallTransition ;

  os << "Rise transition LUT: " << endl ;
  os << timing.riseTransition ;

  return os ;
}


ostream& operator<< (ostream& os, LibertyPinInfo& pin) {

  os << "Pin " << pin.name << ":" << endl ;
  os << "capacitance: " << pin.capacitance << endl ;
  os << "maxCapacitance: " << pin.maxCapacitance << endl ;
  os << "isInput? " << (pin.isInput ? "true" : "false") << endl ;
  os << "isClock? " << (pin.isClock ? "true" : "false") << endl ;
  os << "End pin" << endl ;

  return os ;
}


ostream& operator<< (ostream& os, LibertyCellInfo& cell) {

  os << "Library cell " << cell.name << ": " << endl ;

  os << "Footprint: " << cell.footprint << endl ;
  os << "Leakage power: " << cell.leakagePower << endl ;
  os << "Area: " << cell.area << endl ;
  os << "Sequential? " << (cell.isSequential ? "yes" : "no") << endl ;
  os << "Dont-touch? " << (cell.dontTouch ? "yes" : "no") << endl ;      

  os << "Cell has " << cell.pins.size() << " pins: " << endl ;
  for (int i=0; i < cell.pins.size(); ++i) {
    os << cell.pins[i] << endl ;
  }

  os << "Cell has " << cell.timingArcs.size() << " timing arcs: " << endl ;
  for (int i=0; i < cell.timingArcs.size(); ++i) {
    os << cell.timingArcs[i] << endl ;
  }

  os << "End of cell " << cell.name << endl << endl ;

  return os ;
}

const LibertyLibrary LibertyParser::readFile(const string filename)
{
	is.open(filename.c_str(), fstream::in);

	double maxTransition = 0.0f;
	bool valid = read_default_max_transition(maxTransition) ;

	LibertyLibrary lib(maxTransition);

	assert (valid) ;
	// cout << "The default max transition defined is " << maxTransition << endl ;

	int readCnt = 0 ;
	do {
		LibertyCellInfo cell ;
		valid = read_cell_info (cell) ;

		if (valid) {
			++readCnt ;

			// cout << cell << endl ;
			lib.addCellInfo(cell);
		}

	} while (valid) ;

	// cout << "Read " << readCnt << " number of library cells" << endl ;

	is.close();
	return lib;
}