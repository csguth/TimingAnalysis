#ifndef WIREDELAYMODEL_H_
#define WIREDELAYMODEL_H_

#include <iostream>
using std::cout;
using std::endl;

#include "SpefNet.h"
#include "LibertyLibrary.h"
#include "Configuration.h"

class WireDelayModel
{
protected:
	double lumpedCapacitance;
	static LinearLibertyLookupTableInterpolator interpolator;

public:
	WireDelayModel(const double & lumpedCapacitance) : lumpedCapacitance(lumpedCapacitance){};

	virtual ~WireDelayModel(){};
	virtual const Transitions<double> simulate(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew) = 0;
	virtual const Transitions<double> getDelay(const string nodeName) const = 0;
	virtual const Transitions<double> getSlew(const string nodeName) const = 0;
	virtual void setFanoutPinCapacitance(const string fanoutNameAndPin, const double pinCapacitance) = 0;


	const double getLumpedCapacitance() const { return lumpedCapacitance; }
};

class LumpedCapacitanceWireDelayModel : public WireDelayModel
{
	Transitions<double> delay;
	Transitions<double> slew;
public:
	LumpedCapacitanceWireDelayModel(const SpefNet & descriptor, const string rootNode, const bool dummyEdge = false) : WireDelayModel(descriptor.netLumpedCap){

		cout << "creating lumped capacitance wire delay model to node " << descriptor.netName << ", lumped capacitance " << descriptor.netLumpedCap << endl;
	};
	const Transitions<double> simulate(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew);
	const Transitions<double> getDelay(const string nodeName) const;
	const Transitions<double> getSlew(const string nodeName) const;
	void setFanoutPinCapacitance(const string fanoutNameAndPin, const double pinCapacitance) {
		lumpedCapacitance += pinCapacitance;
	}
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


public:
	RCTreeWireDelayModel(const SpefNetISPD2013 & descriptor, const string rootNode, const bool dummyEdge = false);
	const Transitions<double> simulate(const LibertyCellInfo & cellInfo, const int input, const Transitions<double> slew);
	const Transitions<double> getDelay(const string nodeName) const;
	const Transitions<double> getSlew(const string nodeName) const;
	void setFanoutPinCapacitance(const string fanoutNameAndPin, const double pinCapacitance);
};



#endif