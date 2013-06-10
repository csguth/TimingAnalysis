#ifndef PARSER_H_
#define PARSER_H_

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <map>
using std::map;

#include <utility>
using std::pair;

#include <istream>
using std::istream;

#include <ostream>
using std::ostream;

#include <iostream>
using std::cout;
using std::endl;

#include <fstream>
using std::fstream;

#include <cassert>

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


class CircuitNetList
{
public:

	struct LogicGate {
		string name;
		string cellType;
		vector<int> inNets;
		int fanoutNetIndex;

		LogicGate(const string name, const string cellType, const unsigned inputs, int fanoutNetIndex) :
			name(name), cellType(cellType), inNets(inputs), fanoutNetIndex(fanoutNetIndex)
			{};
	};

	struct Sink {
		int gate;
		string pinName;
		Sink(const int gate, const string pinName) : gate(gate), pinName(pinName){};
	};

	struct Net {
		string name;
		int sourceNode;
		string sourcePin;
		vector<Sink> sinks;
		bool dummyNet;

		Net(const string name, const int sourceNode, const string sourcePin, const bool dummyNet = false) :
			name(name), sourceNode(sourceNode), sourcePin(sourcePin), dummyNet(dummyNet)
			{};

		void addSink(const Sink sinkNode) { sinks.push_back(sinkNode); }
	};

private:
	friend ostream & operator<<( ostream & out, const CircuitNetList netlist) 
	{
		out << "-- NETS" << endl;
		for(int i = 0; i < netlist.nets.size(); i++)
			out << "---- NET["<<i<<"] " << netlist.nets[i] << endl;
		out << "-- GATES" << endl;
		for(int i = 0; i < netlist.gates.size(); i++)
			out << "---- GATE["<<i<<"] " << netlist.gates[i] << endl;	
		return out;
	}
	friend ostream & operator<<( ostream & out, const CircuitNetList::Net net)
	{
		out << net.name << (net.dummyNet? " dummyNet":"") << " from (" << net.sourceNode << ", " << net.sourcePin << ") : ";
		for(int i = 0; i < net.sinks.size(); i++)
		{
			out << net.sinks[i] << " ";
		}
		return out;
	}
	friend ostream & operator<<( ostream & out, const CircuitNetList::Sink sink)
	{
		return out << "("<<sink.gate<<", "<<sink.pinName<<")";
	}
	friend ostream & operator<<( ostream & out, const CircuitNetList::LogicGate gate)
	{
		out << gate.cellType << " " << gate.name << endl;
		for(int i = 0; i < gate.inNets.size(); i++)
			out << "------ net["<<i<<"] = " << gate.inNets[i] << endl;
		return out;
	}
	map<string, int> gateNameToGateIndex;
	map<string, int> netNameToNetIndex;
	vector<LogicGate> gates;
	vector<Net> nets;

	const int addGate(const string name, const string cellType, const int inputs);
	const int addNet(const string name, const int sourceNode, const string sourcePin);
	const int addNet(const string name);
public:
	void addCellInst(const string name, const string cellType, vector<pair<string, string> > inputPinPairs, const bool isSequential = false);


	const size_t getNetsSize() const {return nets.size();};
	Net & getNet(const size_t & i) {return nets[i];};
	LogicGate & getGate(const size_t & i){return gates[i];};
	virtual ~CircuitNetList(){};
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
	bool readFile(const string filename, CircuitNetList & netlist);
	

	virtual ~VerilogParser()
	{

	};
};

#endif