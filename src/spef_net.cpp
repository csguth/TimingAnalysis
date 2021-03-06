#include "include/spef_net.h"

void SpefNetISPD2013::set(string name, double lumpedCapacitance, double total_resistance)
{
	this->netName = name;
    this->netLumpedCap = lumpedCapacitance;
    this->total_resistance = total_resistance;
}
int SpefNetISPD2013::addNode(const string & name)
{
	if (nodeMap.find(name) != nodeMap.end())
		return nodeMap.at(name);
	const int nodeIndex = nodes.size();
	nodes.push_back(Node(nodeIndex, name));
	nodeMap[name] = nodeIndex;
	return nodeIndex;
}

void SpefNetISPD2013::addResistor(const string & node1, const string & node2, const double & value)
{
	const int node1Index = addNode(node1);
	const int node2Index = addNode(node2);
	const int newResistorIndex = resistors.size();
	resistors.push_back(Resistor(node1Index, node2Index, value));
//    if(nodes.at(node1Index).capacitance == 0.0f)
//    {
//        nodes.at(node1Index).capacitance = nodes.at(node2Index).capacitance / 2;
//        nodes.at(node2Index).capacitance = nodes.at(node2Index).capacitance / 2;
//    } else if(nodes.at(node2Index).capacitance == 0.0f)
//    {
//        nodes.at(node2Index).capacitance = nodes.at(node1Index).capacitance / 2;
//        nodes.at(node1Index).capacitance = nodes.at(node1Index).capacitance / 2;
//    }

//    if(nodes.at(node1Index).capacitance == 0.0f)
//    {
//        nodes.at(node1Index).capacitance = nodes.at(node2Index).capacitance * 2;
////        nodes.at(node2Index).capacitance = nodes.at(node2Index).capacitance / 2;
//        this->netLumpedCap += nodes.at(node1Index).capacitance;
//    } else if(nodes.at(node2Index).capacitance == 0.0f)
//    {
//        nodes.at(node2Index).capacitance = nodes.at(node1Index).capacitance * 2;
//        this->netLumpedCap += nodes.at(node2Index).capacitance;
//    }

//    if(nodes.at(node1Index).capacitance == 0.0f)
//    {
//        nodes.at(node1Index).capacitance = nodes.at(node2Index).capacitance;
//        this->netLumpedCap += nodes.at(node1Index).capacitance;
//    } else if(nodes.at(node2Index).capacitance == 0.0f)
//    {
//        nodes.at(node2Index).capacitance = nodes.at(node1Index).capacitance;
//        this->netLumpedCap += nodes.at(node1Index).capacitance;
//    }
	nodes.at(node1Index).resistors.push_back(newResistorIndex);
	nodes.at(node2Index).resistors.push_back(newResistorIndex);
}

void SpefNetISPD2013::addCapacitor(const string & node, const double & value)
{
	const int nodeIndex = addNode(node);
	capacitors.push_back(Capacitor(nodeIndex, value));
	nodes.at(nodeIndex).capacitance += value;
}

ostream& operator<<(ostream & out, const SpefNetISPD2013 & descriptor)
{
    for (size_t i = 0; i < descriptor.nodes.size(); i++)
	{
		const SpefNetISPD2013::Node & node = descriptor.nodes.at(i);
		out << "node " << node.nodeIndex << " " << node.name << " {" << endl;
		out << "  capacitance " << node.capacitance << endl;
		out << "  resistors {" << endl;
        for (size_t j = 0; j < node.resistors.size(); j++)
		{
			out << "    " << j << " " << descriptor.resistors[node.resistors[j]].value;
			out << endl;
		}
		out << "  }" << endl;
		out << "}" << endl;
		out << endl;
	}
	return out;
}

int SpefNetISPD2013::getNodeIndex(const string & name) const
{
	for (unsigned i = 0; i < nodes.size(); i++)
	{
		if (nodes.at(i).name == name)
			return i;
	}
	return -1;
}
