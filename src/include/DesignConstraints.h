#ifndef DESIGNCONSTRAINTS_H_
#define DESIGNCONSTRAINTS_H_

#include <string>
using std::string;

#include "Transitions.h"

#include <map>
using std::map;

#include <utility>
using std::make_pair;

class DesignConstraints
{
	double clock;
	map<string, string> drivingCells;
	map<string, Transitions<double> > inputTransition;
	map<string, Transitions<double> > inputDelay;
	map<string, Transitions<double> > outputDelay;
	map<string, double> outputLoads;
public:

	void setClock(const double clock);
	bool setInputDelay(const string inputName, const Transitions<double> delay);
	bool setOutputDelay(const string outputName, const Transitions<double> delay);
	bool setOutputLoad(const string outputName, const double outputLoad);
	bool setDrivingCell(const string inputName, const string drivingCell);
	bool setInputTransition(const string inputName, const Transitions<double> transition);

	const double getClock() const;
	const Transitions<double> getInputDelay(const string inputName) const;
	const Transitions<double> getOutputDelay(const string outputName) const;
	const double getOutputLoad(const string outputName) const;
	const string getDrivingCell(const string inputName) const;
	const Transitions<double> getInputTransition(const string inputName) const;
	/* data */
};

#endif