#include "include/DesignConstraints.h"

// private
void DesignConstraints::setClock(const double clock)
{
	this->clock = clock;
}
bool DesignConstraints::setInputDelay(const string inputName, const Transitions<double> delay)
{
	if(inputDelay.find(inputName) != inputDelay.end())
		return false;
	inputDelay.insert(make_pair(inputName, delay));
	return true;
}
bool DesignConstraints::setOutputDelay(const string outputName, const Transitions<double> delay)
{
	if(outputDelay.find(outputName) != outputDelay.end())
		return false;
	outputDelay.insert(make_pair(outputName, delay));
	return true;
}
bool DesignConstraints::setOutputLoad(const string outputName, const double outputLoad)
{
	if(outputLoads.find(outputName) != outputLoads.end())
		return false;
	outputLoads.insert(make_pair(outputName, outputLoad));
	return true;
}
bool DesignConstraints::setDrivingCell(const string inputName, const string drivingCell) 
{
	if(drivingCells.find(inputName) != drivingCells.end())
		return false;
	drivingCells.insert(make_pair(inputName, drivingCell));
	return true;
}

bool DesignConstraints::setInputTransition(const string inputName, const Transitions<double> transition)
{
	if(inputTransition.find(inputName) != inputTransition.end())
		return false;
	inputTransition.insert(make_pair(inputName, transition));
	return true;
}


// public
const double DesignConstraints::getClock() const
{
	return clock;
}
const Transitions<double> DesignConstraints::getInputDelay(const string inputName) const
{
	return inputDelay.at(inputName);
}
const Transitions<double> DesignConstraints::getOutputDelay(const string outputName) const
{
	return outputDelay.at(outputName);
}
const double DesignConstraints::getOutputLoad(const string outputName) const
{
	return outputLoads.at(outputName);
}
const string DesignConstraints::getDrivingCell(const string inputName) const
{
	return drivingCells.at(inputName);
}

const Transitions<double> DesignConstraints::getInputTransition(const string inputName) const
{
	return inputTransition.at(inputName);
}
