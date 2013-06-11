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
	cellToFootprintIndex[cellInfo.name] = footPrintIndex;

	return make_pair(footPrintIndex, optionIndex);
}

const LibertyCellInfo & LibertyLibrary::getCellInfo(const string & footPrint, const int & i) const
{
	const int footPrintIndex = footPrintToIndex.at(footPrint);
	return library[footPrintIndex][i];
}

const LibertyCellInfo & LibertyLibrary::getCellInfo(const string & cellName) const
{
	const int footPrintIndex = cellToFootprintIndex.at(cellName);
	const int optionNumber = cellOptionNumber.at(cellName);
	return library[footPrintIndex][optionNumber];
}