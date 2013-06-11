#ifndef WIREDELAYMODEL_H_
#define WIREDELAYMODEL_H_

class WireDelayModel
{
protected:
	double lumpedCapacitance;
public:
	WireDelayModel(const double & lumpedCapacitance) : lumpedCapacitance(lumpedCapacitance){};
	virtual ~WireDelayModel(){};

	virtual const double simulate() = 0;
};

class LumpedCapacitanceWireDelayModel : public WireDelayModel
{
public:
	LumpedCapacitanceWireDelayModel(const double & lumpedCapacitance) : WireDelayModel(lumpedCapacitance){};
	const double simulate();
};

class RCTreeWireDelayModel : public WireDelayModel
{
public:
	RCTreeWireDelayModel(const double & lumpedCapacitance /*deverá receber um descritor lido do spef*/) : WireDelayModel(lumpedCapacitance){};
	const double simulate();
};

#endif