/**
 * SpefNet.h
 *
 *  Created on: Jun 4, 2013
 *      Author: chrystian
 */

#ifndef SPEFNET_H_
#define SPEFNET_H_

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <map>
using std::map;

#include <ostream>
using std::ostream;

#include <iostream>
using std::endl;

#include <queue>
using std::queue;

#include "transitions.h"

#include <cassert>
/** @brief SPEF stands for Standard Parasitic Exchange Format. Describes the set of parasitic elements of the wires in a circuit, within its descriptions and values.
*
*/
class SpefNetISPD2012
{
public:
	string netName;
	double netLumpedCap;

    SpefNetISPD2012():netName("DEFAULT_NET_NAME"), netLumpedCap(0){}
};

class SpefNetISPD2013
{
public:
	/** @brief Struct which represents a resistor.
	*
	*/
	struct Resistor
	{
		int node1;
		int node2;
		double value;
	/** @brief Resistor constructor 
	*	
	* @param const string & node1, const string & node2, const double & value
	*
	*/
        Resistor(const int & node1, const int & node2, const double & value) : node1(node1), node2(node2), value(value){}
     /** @brief If called by node1, returns node2, and vice versa
	 *
	 * @param const string & node1, const string & node2, const double & value
	 *
	 * @return int
	 */
        int getOtherNode(const int & node) const { return (node == node1 ? node2 : node1); }

	};
	/** @brief Struct which represents a capacitor.
	*
	*/
	struct Capacitor
	{
		int node;
		double value;
	/** @brief Capacitor constructor
	 *
	 * @param const string & node, const double & value
	 *
	 */
        Capacitor(const int & node, const double & value) : node(node), value(value) {}
	};
	/** @brief Struct which represents a node of the circuit. A node is a point of the circuit where two or more elements meet.
	*
	*/
	struct Node
	{
		int nodeIndex;
		string name;
		vector<int> resistors;
		double capacitance;
	/** @brief Node constructor
	*
	* @param const int & index, const string & name
	*
	*/
        Node(const int & index, const string & name) : nodeIndex(index), name(name), capacitance(0.0f) { }
	};
private:
	vector<Node> nodes;
	vector<Resistor> resistors;
	vector<Capacitor> capacitors;
	map<string, int> nodeMap;
	/** @brief Adds node to nodes list, returns its index
 	*
 	* @param const string & name
	*
 	* @return int
	*/
    int addNode(const string & name);

public:
	string netName;
	double netLumpedCap;

    /** @brief Empty SpefNet constructor
     *
     */
    SpefNetISPD2013(){}
   /** @brief Empty SpefNet destructor
    *
    */
    virtual ~SpefNetISPD2013(){}

	 /** @brief Adds new resistor to nodes list
	 *
	 * @param const string & node1, const string & node2, const double & value
	 *
	 * @return void
	 */
	void addResistor(const string & node1, const string & node2, const double & value);
	 /** @brief Adds new capacitor to nodes list
	 *
	 * @param const string & node, const double & value
	 *
	 * @return void
	 */
	void addCapacitor(const string & node, const double & value);

   	 /** @brief Redefinition of << operator. Inserts description including node name, index, capacitance and resistors list
	 *
	 */
	friend ostream& operator<<(ostream & out, const SpefNetISPD2013 & descriptor);
	/** @brief Returns number of nodes
	 *
	 * @return size_t
	 */
    size_t nodesSize() const {return nodes.size();}
	/** @brief Gets node at index i
	 *
	 * @param const unsigned & i
	 *
	 * @return const Node 
	 */
    const Node & getNode(const unsigned & i) const { return nodes.at(i);}
	/** @brief Returns index of node. If not found, returns -1
	 *
	 * @param const string & name
	 *
	 * @return int
	 */
    int getNodeIndex(const string & name) const;
	/** @brief Retuns number of resistors
	 *
	 * @return size_t
	 */
    size_t resistorsSize() const { return resistors.size(); }
	/** @brief Returns resistor at index i
	 *
	 * @param const unsigned & i
	 *
	 * @return const Resistor
	 */
    const Resistor & getResistor(const unsigned & i) const { return resistors.at(i); }

	/** @brief Sets name and lumpedCapacitance attributes
	 *
	 * @param string name,double lumpedCapacitance
	 *
	 * @return void
	 */
	void set(string name, double lumpedCapacitance);

};

	/** @brief Name redefinition: from map<string> to SpefNetISPD2012>
	 *
	 */
typedef map<string, SpefNetISPD2012> Parasitics2012;
	/** @brief Name redefinition: from map<string> to SpefNetISPD2013>
	 *
	 */
typedef map<string, SpefNetISPD2013> Parasitics2013;

#endif /* SPEFNET_H_ */
