#include "include/liberty_library.h"


LibertyLibrary::LibertyLibrary(const double maxTransition) : maxTransition(maxTransition)
{
	library.push_back(vector<LibertyCellInfo>());

// DUMMY CELL TYPE TO PRIMARY OUTPUT
	LibertyCellInfo po;
	po.name = "__PO__";
	po.footprint = "__PO__";
	po.pins.resize(1);
	po.timingArcs.resize(1);
	po.primaryOutput = true;
	library[0].push_back(po);
	footPrintToIndex[po.footprint] = 0;
	cellOptionNumber[po.name] = 0;
	cellToFootprintIndex[po.name] = footPrintToIndex[po.footprint];

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
	return library.at(footPrintIndex).at(i);
}

const LibertyCellInfo & LibertyLibrary::getCellInfo(const string & cellName) const
{
	const int footPrintIndex = cellToFootprintIndex.at(cellName);
	const int optionNumber = cellOptionNumber.at(cellName);
	return library.at(footPrintIndex).at(optionNumber);
}

const LibertyCellInfo & LibertyLibrary::getCellInfo(const int & footPrintIndex, const int & optionIndex) const
{
    return library.at(footPrintIndex).at(optionIndex);
}

size_t LibertyLibrary::number_of_options(const int footprint_index) const
{
    return library.at(footprint_index).size();
}


const pair<int, int> LibertyLibrary::getCellIndex(const string &cellName) const
{
	const int footPrintIndex = cellToFootprintIndex.at(cellName);
	const int optionNumber = cellOptionNumber.at(cellName);
	return make_pair(footPrintIndex, optionNumber);
}



double LinearLibertyLookupTableInterpolator::interpolate(const LibertyLookupTable & lut, const double load, const double transition)
{
	double wTransition, wLoad, y1, y2, x1, x2;
	double t[2][2];
	int row1, row2, column1, column2;
	wTransition = 0.0f;
	wLoad = 0.0f;

	assert(load >= 0 && transition >= 0);

	row1 = lut.loadIndices.size() - 2;
	row2 = lut.loadIndices.size() - 1;

	y1 = lut.loadIndices[row1];
	y2 = lut.loadIndices[row2];

	// loads -- rows
	for(size_t i = 0; i < lut.loadIndices.size() - 1; i++)
	{
		if(load >= lut.loadIndices[i] && load <= lut.loadIndices[i + 1])
		{
			row1 = i;
			row2 = i + 1;
			y1 = lut.loadIndices[row1];
			y2 = lut.loadIndices[row2];
		}
	}

	// transitions -- columns
	if(transition < lut.transitionIndices[0])
	{
		column1 = 0;
		column2 = 1;
		x1 = lut.transitionIndices[column1];
		x2 = lut.transitionIndices[column2];
	}
	else if (transition > lut.transitionIndices[lut.transitionIndices.size()-1])
	{
		column1 = lut.transitionIndices.size() - 2;
		column2 = lut.transitionIndices.size() - 1;
		x1 = lut.transitionIndices[column1];
		x2 = lut.transitionIndices[column2];
	}
	else
	{
		for(size_t i = 0; i < lut.transitionIndices.size() - 1; i++)
		{
			if(transition >= lut.transitionIndices[i] && transition <= lut.transitionIndices[i + 1])
			{
				column1 = i;
				column2 = i + 1;
				x1 = lut.transitionIndices[column1];
				x2 = lut.transitionIndices[column2];
			}
		}
	}

	//equation for interpolation (Ref - ISPD Contest: http://www.ispd.cc/contests/12/ISPD_2012_Contest_Details.pdf), slide 17
	wTransition = (transition - x1) * (1.0f / (x2 - x1));
	wLoad = (load - y1) * (1.0f / (y2 - y1));

	t[0][0] = lut.tableVals[row1][column1];
	t[0][1] = lut.tableVals[row1][column2];
	t[1][0] = lut.tableVals[row2][column1];
	t[1][1] = lut.tableVals[row2][column2];

	return ((1 - wTransition) * (1 - wLoad) * t[0][0]) + (wTransition * (1 - wLoad) * t[0][1]) + ((1 - wTransition) * wLoad * t[1][0]) + (wTransition * wLoad * t[1][1]);
 }


const int LibertyLookupTableInterpolator::DEFAULT_DECIMAL_PLACES = 2;

 const Transitions<double> LinearLibertyLookupTableInterpolator::interpolate(const LibertyLookupTable & rise_lut, const LibertyLookupTable & fall_lut, const Transitions<double> load, const Transitions<double> transition, Unateness unateness, bool is_input_driver)
 {
    Transitions<double> result;
    double rise_delay, fall_delay;
    switch(unateness)
    {
    case NEGATIVE_UNATE:

        rise_delay = interpolate(rise_lut, load.getRise(), transition.getFall());
        fall_delay = interpolate(fall_lut, load.getFall(), transition.getRise());

        if(is_input_driver)
        {
            rise_delay -= interpolate(rise_lut, 0.0f, transition.getFall());
            fall_delay -= interpolate(rise_lut, 0.0f, transition.getRise());
        }
        break;
    case POSITIVE_UNATE:
        rise_delay = interpolate(rise_lut, load.getRise(), transition.getRise());
        fall_delay = interpolate(fall_lut, load.getFall(), transition.getFall());
        break;
    case NON_UNATE:
        rise_delay = max(interpolate(rise_lut, load.getRise(), transition.getFall()), interpolate(rise_lut, load.getRise(), transition.getRise()));
        fall_delay = max(interpolate(fall_lut, load.getFall(), transition.getRise()), interpolate(fall_lut, load.getFall(), transition.getFall()));

        break;
    }

    result = Transitions<double>(rise_delay, fall_delay);
    round(result, DEFAULT_DECIMAL_PLACES);
    return result;
 }

double LibertyLibrary::getMaxTransition() const
{
    return maxTransition;
}

void LibertyLookupTableInterpolator::round(Transitions<double> &transitions, const int decimal_places)
{
    return;
    const Transitions<int> truncated = Transitions<int>(int(transitions.getRise() * pow(10, decimal_places)), int(transitions.getFall() * pow(10, decimal_places)));
    transitions = Transitions<double>(truncated.getRise(), truncated.getFall());
    transitions /= pow(10, decimal_places);
}
