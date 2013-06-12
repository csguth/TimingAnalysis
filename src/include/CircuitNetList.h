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

class CircuitNetList
{
public:

	struct LogicGate {
		string name;
		string cellType;
		vector<int> inNets;
		int fanoutNetIndex;
		bool inputDriver;
		bool sequential;

		LogicGate(const string name, const string cellType, const unsigned inputs, int fanoutNetIndex, const bool inputDriver = false) :
			name(name), cellType(cellType), inNets(inputs), fanoutNetIndex(fanoutNetIndex), inputDriver(inputDriver)
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
		out << "-- NET TOPOLOGY ("<< netlist.nets.size()<< ")" << endl;
		for(size_t i = 0; i < netlist.nets.size(); i++)
			out << "---- netT[" << i  << " = "<< netlist.netTopology[i] <<"] "<< netlist.nets[netlist.netTopology[i]] << endl;
		out << "-- GATE TOPOLOGY" << endl;
		for(size_t i = 0; i < netlist.gates.size(); i++)
			out << "---- gateT["<<i<<" = "<< netlist.topology[i] <<"] " << netlist.gates[netlist.topology[i]] << endl;	
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
		out << gate.cellType << " " << gate.name << (gate.inputDriver?" inputDriver":"") << endl;
		for(int i = 0; i < gate.inNets.size(); i++)
			out << "------ net["<<i<<"] = " << gate.inNets[i] << endl;
		return out;
	}
	map<string, int> gateNameToGateIndex;
	map<string, int> netNameToNetIndex;
	vector<LogicGate> gates;
	vector<Net> nets;

	vector<int> topology;
	vector<int> netTopology;

	vector<int> inverseTopology;
	vector<int> inverseNetTopology;


	const int addGate(const string name, const string cellType, const int inputs, const bool isInputDriver = false);
	const int addNet(const string name, const int sourceNode, const string sourcePin);
	const int addNet(const string name);
public:
	void addCellInst(const string name, const string cellType, vector<pair<string, string> > inputPinPairs, const bool isSequential = false, const bool isInputDriver = false);
	void updateTopology();

	const size_t getNetsSize() const { return nets.size(); };
	const size_t getGatesSize() const { return gates.size(); };
	Net & getNet(const size_t & i) { return nets[i]; };
	LogicGate & getGate(const size_t & i) { return gates[i]; };
	const LogicGate & getGateT(const size_t & i) const { return gates[topology[i]]; };
	const Net & getNetT(const size_t & i) const { return nets[netTopology[i]]; };
	const int getTopologicIndex(const size_t & i) const { return (i==-1?-1:inverseTopology.at(i)); };
	const int getNetTopologicIndex(const size_t & i) const { return inverseNetTopology.at(i); };

	virtual ~CircuitNetList(){};
};

#endif