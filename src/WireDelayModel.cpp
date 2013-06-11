#include "include/WireDelayModel.h"

const double LumpedCapacitanceWireDelayModel::simulate()
{
	return lumpedCapacitance;
}
const double RCTreeWireDelayModel::simulate()
{
	const double ceff = 1.2f;
	return ceff;
}
