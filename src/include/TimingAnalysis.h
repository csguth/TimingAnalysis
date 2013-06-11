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

namespace TimingAnalysis
{

	class TimingPoint
	{
		Transitions<double> arrivalTime;
		Transitions<double> slew;
	public:
		TimingPoint();
		virtual ~TimingPoint();
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
		unsigned option;
		WireDelayModel * wireDelayModel;
		friend ostream& operator<<(ostream & out, Node & node)
		{
			return out << node.name << " op " << node.option << " wireDelayModel " << node.wireDelayModel << endl;
		};

	public:
		Node(const string name = "DEFAULT_NODE_NAME", const unsigned inputs = 1, WireDelayModel * wireDelayModel = 0);
		virtual ~Node();
	};

	class TimingAnalysis
	{
		vector<Node> nodes;
	public:
		TimingAnalysis(const CircuitNetList netlist);
		virtual ~TimingAnalysis();


		// GETTERS
		const string getNodeName(const int nodeIndex) const;
		const unsigned getNumberOfNodes() const;
		const double simulateRCTree(const int &nodeIndex);
	};

};

#endif