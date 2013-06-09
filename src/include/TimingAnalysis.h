#ifndef TIMING_ANALYSIS_H_
#define TIMING_ANALYSIS_H_

#include <vector>
using std::vector;

#include <string>
using std::string;

#include "Transitions.h"

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

	public:
		Node(const string name = "DEFAULT_NODE_NAME", const unsigned inputs = 1);
		virtual ~Node();
	};

	class TimingAnalysis
	{
		vector<Node> nodes;
	public:
		TimingAnalysis();
		virtual ~TimingAnalysis();


		// GETTERS
		const string getNodeName(const int nodeIndex) const;
		const unsigned getNumberOfNodes() const;
	};

};

#endif