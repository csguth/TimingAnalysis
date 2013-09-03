#ifndef CIRCUITNETLIST_H_
#define CIRCUITNETLIST_H_

#include <string>
using std::string;

#include <ostream>
using std::ostream;

#include <vector>
using std::vector;

#include <map>
using std::map;

#include <utility>
using std::pair;

#include <iostream>
using std::cout;
using std::endl;

#include <queue>
using std::queue;

#include <cassert>

class Circuit_Netlist
{
public:

    struct Logic_Gate {
		string name;
		string cellType;
		vector<int> inNets;
		int fanoutNetIndex;
		bool inputDriver;
		bool sequential;
		bool primary_output;

        Logic_Gate(const string name, const string cellType, const unsigned inputs, int fanoutNetIndex, const bool inputDriver = false, const bool primary_output = false) :
			name(name), cellType(cellType), inNets(inputs), fanoutNetIndex(fanoutNetIndex), inputDriver(inputDriver), primary_output(primary_output)
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
    friend ostream & operator<<( ostream & out, const Circuit_Netlist netlist)
	{
		out << "-- NET TOPOLOGY ("<< netlist.nets.size()<< ")" << endl;
		for(size_t i = 0; i < netlist.nets.size(); i++)
			out << "---- netT[" << i  << " = "<< netlist.netTopology[i] <<"] "<< netlist.nets[netlist.netTopology[i]] << endl;
		out << "-- GATE TOPOLOGY" << endl;
		for(size_t i = 0; i < netlist.gates.size(); i++)
			out << "---- gateT["<<i<<" = "<< netlist.topology[i] <<"] " << netlist.gates[netlist.topology[i]] << endl;	
		return out;
	}
    friend ostream & operator<<( ostream & out, const Circuit_Netlist::Net net)
	{
		out << net.name << (net.dummyNet? " dummyNet":"") << " from (" << net.sourceNode << ", " << net.sourcePin << ") : ";
        for(size_t i = 0; i < net.sinks.size(); i++)
			out << net.sinks[i] << " ";
		return out;
	}
    friend ostream & operator<<( ostream & out, const Circuit_Netlist::Sink sink)
	{
		return out << "("<<sink.gate<<", "<<sink.pinName<<")";
	}
    friend ostream & operator<<( ostream & out, const Circuit_Netlist::Logic_Gate gate)
	{
		out << gate.cellType << " " << gate.name << (gate.inputDriver?" inputDriver":"") << endl;
        for(size_t i = 0; i < gate.inNets.size(); i++)
			out << "------ net["<<i<<"] = " << gate.inNets[i] << endl;
		return out;
	}
	map<string, int> gateNameToGateIndex;
	map<string, int> netNameToNetIndex;
    vector<Logic_Gate> gates;
	vector<Net> nets;

	vector<int> topology;
	vector<int> netTopology;

	vector<int> inverseTopology;
	vector<int> inverseNetTopology;

	int _numberOfGates;


    int addGate(const string name, const string cellType, const int inputs, const bool isInputDriver = false, const bool primary_output = false);
    int addNet(const string name, const int sourceNode, const string sourcePin);
    int addNet(const string name);
public:
	void addCellInst(const string name, const string cellType, vector<pair<string, string> > inputPinPairs, const bool isSequential = false, const bool isInputDriver = false, const bool primary_output = false);
	void updateTopology();

    size_t getNetsSize() const { return nets.size(); }
    size_t getGatesSize() const { return gates.size(); }
    Net & getNet(const size_t & i) { return nets[i]; }
    Logic_Gate & getGate(const size_t & i) { return gates[i]; }
    const Logic_Gate & getGateT(const size_t & i) const { return gates.at(topology.at(i)); }
    const Net & getNetT(const size_t & i) const { return nets.at(netTopology.at(i)); }
    int getTopologicIndex(const int & i) const { return (i==-1?-1:inverseTopology.at(i)); }
    int get_net_topologic_index(const size_t & i) const { return inverseNetTopology.at(i); }
	const vector<pair<int, string> > verilog() const;
    virtual ~Circuit_Netlist(){}
    Circuit_Netlist():_numberOfGates(0){}
};

#endif
