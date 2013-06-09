#include "include/TimingAnalysis.h"

namespace TimingAnalysis
{
/*

	TIMING ANALYSIS

*/

	TimingAnalysis::TimingAnalysis() : nodes(5)
	{
		nodes[0] = Node("Source");
		nodes[1] = Node("Node 1", 2);
		nodes[2] = Node("Node 2", 3);
		nodes[3] = Node("Node 3", 1);
		nodes[4] = Node("Sink", 1);
	}

	TimingAnalysis::~TimingAnalysis()
	{

	}

	// GETTERS
	const string TimingAnalysis::getNodeName(const int nodeIndex) const
	{
		return nodes[nodeIndex].name;
	}

	const unsigned TimingAnalysis::getNumberOfNodes() const
	{
		return nodes.size();
	}


/*

	NODE

*/
	Node::Node(const string name, const unsigned inputs) :	name(name),
															timingPoints(inputs + 1),
															timingArcs(inputs)
	{

	}
	Node::~Node()
	{

	}

/*

	TIMING POINT

*/

	TimingPoint::TimingPoint() :	arrivalTime(0.0f, 0.0f),
									slew(0.0f, 0.0f)
	{
	}
	TimingPoint::~TimingPoint()
	{

	}

/*

	TIMING ARC

*/
	TimingArc::TimingArc() :	delay(0.0f, 0.0f),
								slew(0.0f, 0.0f)
	{

	}
	TimingArc::~TimingArc()
	{

	}

};