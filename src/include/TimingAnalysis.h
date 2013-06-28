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
#include "SpefNet.h"
#include "DesignConstraints.h"

namespace TimingAnalysis
{


	class Edge;
	class Node;
	class TimingPoint
	{
		friend class TimingAnalysis;
		friend class Edge;
		string name;
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
		friend class TimingAnalysis;
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
		bool inputDriver;
		bool sequential;
		bool primaryOutput;

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
		Edge(const string & netName = "DEFAULT_NET_NAME", WireDelayModel * wireDelayModel = 0, TimingPoint * driver = 0, const int numFanouts = 0);
		virtual ~Edge();
		void addFanout(TimingPoint * fanout, const double pinCapacitance);
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
		const Parasitics * parasitics;
		LibertyLookupTableInterpolator * interpolator;


		double targetDelay;

		void updateTiming(const int i);
		const Transitions<double> getNodeDelay(const int nodeIndex, const int inputNumber, const Transitions<double> transition, const Transitions<double> ceff);
	public:
		TimingAnalysis(const CircuitNetList netlist, const LibertyLibrary * lib, const Parasitics * parasitics, const DesignConstraints * sdc);
		virtual ~TimingAnalysis();
		void fullTimingAnalysis();

		// SETTERS
		void setNodeOption(const int nodeIndex, const int optionNumber);

		// GETTERS
		const string getNodeName(const int nodeIndex) const;
		const unsigned getNumberOfNodes() const;

		void printInfo();


	};

};

#endif