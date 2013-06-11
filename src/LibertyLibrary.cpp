#include "include/LibertyLibrary.h"


LibertyLibrary::LibertyLibrary(const double maxTransition) : maxTransition(maxTransition)
{
}
LibertyLibrary::~LibertyLibrary()
{

}

const pair<int, int> LibertyLibrary::addCellInfo(const LibertyCellInfo & cellInfo)
{

	if(footPrintToIndex.find(cellInfo.footprint) == footPrintToIndex.end())
	{
		library.push_back(vector<LibertyCellInfo>());
		footPrintToIndex[cellInfo.footprint] = library.size() - 1;
	}

	const int footPrintIndex = footPrintToIndex[cellInfo.footprint];
	const int optionIndex = library[footPrintIndex].size();

	library[footPrintIndex].push_back(cellInfo);
	cellOptionNumber[cellInfo.name] = optionIndex;

	return make_pair(footPrintIndex, optionIndex);
}