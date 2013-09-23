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
/** @brief Describes que set of elements of the circuit, within its nets(which connect them together)
*
*/
class Circuit_Netlist
{
public:
	/** @brief Struct which represents a logic gate
	*
	*/
    struct Logic_Gate {
		string name;
		string cellType;
		vector<int> inNets;
		int fanoutNetIndex;
		bool inputDriver;
		bool sequential;
		bool primary_output;
		/** @brief Logic_Gate constructor 
		*
		* @param const string name, const string cellType, const unsigned inputs, int fanoutNetIndex, const bool inputDriver(false default), const bool primary_output(false default)
		*
		*/
        Logic_Gate(const string name, const string cellType, const unsigned inputs, int fanoutNetIndex, const bool inputDriver = false, const bool primary_output = false) :
			name(name), cellType(cellType), inNets(inputs), fanoutNetIndex(fanoutNetIndex), inputDriver(inputDriver), primary_output(primary_output)
			{};
	};
	/** @brief Struct which represents a Sink. A sink is a vertex which has input edges, but no output edges(outdegree zerp). 
	*
	*/
	struct Sink {
		int gate;
		string pinName;
		/** @brief Sink constructor 
		*
		* @param const int gate, const string pinName
		*/
		Sink(const int gate, const string pinName) : gate(gate), pinName(pinName){};
	};
	/** @brief Struct which represents a Net, which connect two or more circuit elements together
	*
	*/
	struct Net {
		string name;
		int sourceNode;
		string sourcePin;
		vector<Sink> sinks;
		bool dummyNet;
		/** @brief Net constructor 
		*
		* @param const string name, const int sourceNode, const string sourcePin, const bool dummyNet(false default)
		*/
		Net(const string name, const int sourceNode, const string sourcePin, const bool dummyNet = false) :
			name(name), sourceNode(sourceNode), sourcePin(sourcePin), dummyNet(dummyNet)
			{};
		/** @brief Adds sink to sinks list
	 	*
	 	* @param const Sink sinkNode
		*
	 	* @return void
		*/
		void addSink(const Sink sinkNode) { sinks.push_back(sinkNode); }
	};

private:
	/** @brief Redefinition of << operator. Inserts formatted description of Netlist including net/gate lists' size and elements
	 *
	 *@param ostream & out, const Circuit_Netlist netlist
	 */
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

	/** @brief Redefinition of << operator. Inserts formatted description of Net including source node/pin and list of sinks
	 *
	 *@param ostream & out, const Circuit_Netlist::Net net
	 */
    friend ostream & operator<<( ostream & out, const Circuit_Netlist::Net net)
	{
		out << net.name << (net.dummyNet? " dummyNet":"") << " from (" << net.sourceNode << ", " << net.sourcePin << ") : ";
        for(size_t i = 0; i < net.sinks.size(); i++)
			out << net.sinks[i] << " ";
		return out;
	}
	/** @brief Redefinition of << operator. Inserts formatted description of Sink including gate and pin name
	 *
	 *@param ostream & out, const Sink sink
	 */
    friend ostream & operator<<( ostream & out, const Circuit_Netlist::Sink sink)
	{
		return out << "("<<sink.gate<<", "<<sink.pinName<<")";
	}
	/** @brief Redefinition of << operator. Inserts formatted description of Logic_Gate including cell type, name, and input identifiers. 
	 *
	 *@param ostream & out, const Logic_Gate gate
	 */
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
    int _timing_arcs;
    int _timing_points;
	/** @brief Adds gate to gates list. Returns its index
	*
	* @param const Sink sinkNode
	*
	* @return int
	*/
    int addGate(const string name, const string cellType, const int inputs, const bool isInputDriver = false, const bool primary_output = false);
	/** @brief Adds net to nets list. Returns its index
	*
	* @param const string name, const int sourceNode, const string sourcePin
	*
	* @return int
	*/
    int addNet(const string name, const int sourceNode, const string sourcePin);
	/** @brief Adds net to nets list. Returns its index
	*
	* @param const string name
	*
	* @return int
	*/
    int addNet(const string name);
public:

	/** @brief Circuit_Netlist default constructor
	*
	*/
    Circuit_Netlist():_numberOfGates(0), _timing_arcs(0), _timing_points(0){}
	/** @brief Empty Circuit_Netlist destructor
	*
	*/
    virtual ~Circuit_Netlist(){}

	/** @brief ********************************
	*
	* @param const string name, const string cellType, vector<pair<string, string> > inputPinPairs, const bool isSequential(false default), const bool isInputDriver(false default), const bool *primary_output(false default)
	*
	* @return void
	*/
	void addCellInst(const string name, const string cellType, vector<pair<string, string> > inputPinPairs, const bool isSequential = false, const bool isInputDriver = false, const bool primary_output = false);

	/** @brief Updates topology**********************
	*
	* @return void
	*/
	void updateTopology();
	/** @brief Returns number of nets
	*
	* @return size_t
	*/
    size_t getNetsSize() const { return nets.size(); }
	/** @brief Returns Net at index i
	*
	*@param const size_t & i
	*
	* @return Net &
	*/
    Net & getNet(const size_t & i) { return nets[i]; }
	/** @brief Returns Logic_Gate at index i****************************************
	*
	*@param const size_t & i
	*
	* @return const Logic_Gate &
	*/
    const Logic_Gate & getGateT(const size_t & i) const { return gates.at(topology.at(i)); }
	/** @brief Returns Topologic index at index i********************
	*
	*@param const size_t & i
	*
	* @return int
	*/
    int getTopologicIndex(const int & i) const { return (i==-1?-1:inverseTopology.at(i)); }

	/** @brief Returns number of gates
	*
	* @return size_t
	*/
    size_t getGatesSize() const { return gates.size(); }
	/** @brief Returns Logic_Gate at index i
	*
	*@param const size_t & i
	*
	* @return Logic_Gate &
	*/
    Logic_Gate & getGate(const size_t & i) { return gates[i]; }
	/** @brief Returns Net at index i***************************
	*
	*@param const size_t & i
	*
	* @return const Net &
	*/
    const Net & getNetT(const size_t & i) const { return nets.at(netTopology.at(i)); }
	/** @brief Returns index of NetTopology****************************
	*
	*@param const size_t & i
	*
	* @return int
	*/
    int get_net_topologic_index(const size_t & i) const { return inverseNetTopology.at(i); }
	/** @brief Returns ***************************
	*
	*
	* @return const vector<pair<int, string> >
	*/
	const vector<pair<int, string> > verilog() const;

	/** @brief Returns number of timings arcs
	*
	*@return int
	*/
    int timing_arcs() const;
	/** @brief Returns number of timings points
	*
	*@return int
	*/
    int timing_points() const;

};

#endif
