/*
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

class SpefNetISPD2012
{
public:
	string netName;
	double netLumpedCap;
    double total_resistance;

    SpefNetISPD2012():netName("DEFAULT_NET_NAME"), netLumpedCap(0), total_resistance(0){}
};

class SpefNetISPD2013
{
public:
	struct Resistor
	{
		int node1;
		int node2;
		double value;
        Resistor(const int & node1, const int & node2, const double & value) : node1(node1), node2(node2), value(value){}
        int getOtherNode(const int & node) const { return (node == node1 ? node2 : node1); }

	};
	struct Capacitor
	{
		int node;
		double value;
        Capacitor(const int & node, const double & value) : 	node(node), value(value) {}
	};
	struct Node
	{
		int nodeIndex;
		string name;
		vector<int> resistors;
		double capacitance;
        Node(const int & index, const string & name) : nodeIndex(index), name(name), capacitance(1e-6) { }
	};
private:
	vector<Node> nodes;
	vector<Resistor> resistors;
	vector<Capacitor> capacitors;
	map<string, int> nodeMap;
    int addNode(const string & name);

public:
    SpefNetISPD2013(){}
    virtual ~SpefNetISPD2013(){}
	void addResistor(const string & node1, const string & node2, const double & value);
	void addCapacitor(const string & node, const double & value);

	friend ostream& operator<<(ostream & out, const SpefNetISPD2013 & descriptor);
    size_t nodesSize() const {return nodes.size();}
    const Node & getNode(const unsigned & i) const { return nodes.at(i);}
    int getNodeIndex(const string & name) const;
    size_t resistorsSize() const { return resistors.size(); }
    const Resistor & getResistor(const unsigned & i) const { return resistors.at(i); }

    void set(string name, double lumpedCapacitance, double total_resistance);
	string netName;
	double netLumpedCap;
    double total_resistance;




};

typedef map<string, SpefNetISPD2012> Parasitics2012;
typedef map<string, SpefNetISPD2013> Parasitics2013;

#endif /* SPEFNET_H_ */
