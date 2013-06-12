#ifndef WIREDELAYMODEL_H_
#define WIREDELAYMODEL_H_

#include <iostream>
using std::cout;
using std::endl;

#include "SpefNet.h"
#include "LibertyLibrary.h"

class WireDelayModel
{
protected:
	double lumpedCapacitance;
public:
	WireDelayModel(const double & lumpedCapacitance) : lumpedCapacitance(lumpedCapacitance){};
	virtual ~WireDelayModel(){};

	virtual const double simulate(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew) = 0;
};

class LumpedCapacitanceWireDelayModel : public WireDelayModel
{
public:
	LumpedCapacitanceWireDelayModel(const double & lumpedCapacitance) : WireDelayModel(lumpedCapacitance){};
	const double simulate(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew);
};

class RCTreeWireDelayModel : public WireDelayModel
{
	struct Node
	{
		int parent;
		double nodeCapacitance;
		double totalCapacitance;
		Transitions<double> effectiveCapacitance;
		double resistance;
		Transitions<double> slew;
		Transitions<double> delay;
		bool sink;
		static vector<string> nodesNames;
		Node() :
				parent(-1), nodeCapacitance(0.0f), totalCapacitance(0.0f), effectiveCapacitance(0.0f, 0.0f), resistance(0.0f), slew(0.0f, 0.0f), delay(0.0f, 0.0f), sink(false)
		{
		}
		;
	};

	struct NodeAndResistor
	{
		int nodeIndex;
		int resistorIndex;
		NodeAndResistor(const int & node, const int & resistor) :
				nodeIndex(node), resistorIndex(resistor)
		{
		}
		;
	};


	void updateSlews(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew);
	void updateEffectiveCapacitances();
	void updateDownstreamCapacitances();
	void initializeEffectiveCapacitances();

	vector<Node> nodes;
	vector<string> nodesNames;
	vector<Transitions<double> > _slews;
	vector<Transitions<double> > _delays;
	map<std::string, int> fanoutNameToNodeNumber;

	static LinearLibertyLookupTableInterpolator interpolator;
public:
	RCTreeWireDelayModel(const SpefNetISPD2013 & descriptor, const bool dummyEdge, const string rootNode);
	const double simulate(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew);
};






#endif