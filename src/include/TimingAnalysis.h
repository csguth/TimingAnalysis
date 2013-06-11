#ifndef TIMING_ANALYSIS_H_
#define TIMING_ANALYSIS_H_

#include <vector>
using std::vector;

#include <string>
using std::string;

#include <ostream>
using std::ostream;

#include "Transitions.h"
#include "CircuitNetList.h"
#include "WireDelayModel.h"
#include "LibertyLibrary.h"

namespace TimingAnalysis
{

	class Edge;
	class TimingPoint
	{
		friend class TimingAnalysis;
		Edge * net;
		Transitions<double> arrivalTime;
		Transitions<double> slew;

	public:
		TimingPoint();
		virtual ~TimingPoint();
		const string getNetName();
	};

	class TimingArc
	{
		Transitions<double> delay;
		Transitions<double> slew;
	public:
		TimingArc();
		virtual ~TimingArc();
	};

	class Node
	{
		friend class TimingAnalysis;

		string name;
		vector<TimingPoint> timingPoints;
		vector<TimingArc> timingArcs;

	public:
		Node(const string name = "DEFAULT_NODE_NAME", const unsigned inputs = 1);
		virtual ~Node();
	};

	class Edge
	{
		friend class TimingAnalysis;
		string netName;
		WireDelayModel * wireDelayModel;
		TimingPoint * driver;
		vector<TimingPoint*> fanouts;

	public:
		Edge(const string & netName = "DEFAULT_NET_NAME", WireDelayModel * wireDelayModel = 0, TimingPoint * driver = 0, const int numFanouts = 1);
		virtual ~Edge();
		void addFanout(TimingPoint * fanout);
		const string getNetName();
	};

	struct Option
	{
		int footprintIndex;
		int optionIndex;

		Option():footprintIndex(-1), optionIndex(-1){};
		Option(const int footprintIndex, const int optionIndex) : footprintIndex(footprintIndex), optionIndex(optionIndex){};
	};

	class TimingAnalysis
	{
		vector<Node> nodes;
		vector<Option> nodesOptions;
		vector<Edge> edges;


		const LibertyLibrary * library;
		LibertyLookupTableInterpolator * interpolator;
	public:
		TimingAnalysis(const CircuitNetList netlist, const LibertyLibrary * lib);
		virtual ~TimingAnalysis();


		// GETTERS
		const string getNodeName(const int nodeIndex) const;
		const unsigned getNumberOfNodes() const;

		const Transitions<double> getNodeDelay(const int nodeIndex, const int inputNumber);
	};

};

#endif