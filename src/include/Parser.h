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

class Parser
{
protected:
	bool isSpecialChar(const char & c);
	bool readLineAsTokens(istream& is, vector<string>& tokens, bool includeSpecialChars = false);
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

		Net(const string name, const int sourceNode, const string sourcePin) :
			name(name), sourceNode(sourceNode), sourcePin(sourcePin)
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
		out << net.name << " " << net.sourceNode << " " << net.sourcePin << " : ";
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
	void addCellInst(const string name, const string cellType, vector<pair<string, string> > inputPinPairs);
	virtual ~CircuitNetList(){};
};


class VerilogParser : public Parser
{
public:
	bool readFile(const string filename, CircuitNetList & netlist);
	virtual ~VerilogParser()
	{

	};
};

#endif