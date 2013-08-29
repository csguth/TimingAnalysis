#ifndef LIBERTYLIBRARY_H_
#define LIBERTYLIBRARY_H_

#include <utility>
using std::pair;
using std::make_pair;

#include <vector>
using std::vector;

#include <string>
using std::string;

#include <map>
using std::map;

#include <limits>
using std::numeric_limits;

#include <ostream>
using std::ostream;

#include <cassert>

#include "Transitions.h"

// Look up table to store delay or slew functions
struct LibertyLookupTable {

  // Look up table is indexed by the output load and the input transition values
  // Example:
  //   Let L = loadIndices[i]
  //       T = transitionIndices[j]
  //   Then, the table value corresponding to L and T will be:
  //       table[i][j]
  //
  vector<double> loadIndices ;
  vector<double> transitionIndices ;
  vector<vector<double> > tableVals ;

};

ostream& operator<< (ostream& os, LibertyLookupTable& lut) ;

struct LibertyTimingInfo {

  string fromPin ;
  string toPin ;
  string timingSense ; // "non_unate" or "negative_unate" or "positive_unate".
  // Note that ISPD-13 library will have only negative-unate combinational cells. The clock arcs
  // for sequentials will be non_unate (which can be ignored because of the simplified sequential
  // timing model for ISPD-13).

  
  LibertyLookupTable fallDelay ;
  LibertyLookupTable riseDelay ;
  LibertyLookupTable fallTransition ;
  LibertyLookupTable riseTransition ;

} ;

ostream& operator<< (ostream& os, LibertyTimingInfo& timing) ;

struct LibertyPinInfo {

  string name ; // pin name
  double capacitance ; // input pin cap (not defined for output pins)
  double maxCapacitance ; // the max load this pin can drive
  bool isInput ; // whether the pin is input or output pin
  bool isClock ; // whether the pin is a clock pin or not

  LibertyPinInfo () : capacitance (0.0)
  , maxCapacitance (std::numeric_limits<double>::max())
  , isInput(true)
  , isClock(false) {};
  
};


ostream& operator<< (ostream& os, LibertyPinInfo& pin) ;

struct LibertyCellInfo {

  string name ; // cell name
  string footprint ; // only the cells with the same footprint are swappable
  double leakagePower ; // cell leakage power
  double area ; // cell area (will not be a metric for ISPD-13)
  bool isSequential ; // if true then sequential cell, else combinational
  bool dontTouch ; // is the sizer allowed to size this cell? 
  bool primaryOutput;
  
  vector<LibertyPinInfo> pins ;
  vector<LibertyTimingInfo> timingArcs ;

  LibertyCellInfo () : leakagePower (0.0), area (0.0), isSequential (false), dontTouch(false), primaryOutput(false) {}
  
} ;

ostream& operator<< (ostream& os, LibertyCellInfo& cell) ;


class LibertyLibrary
{
  double maxTransition;
  vector< vector<LibertyCellInfo> > library;

  map<string, int> footPrintToIndex;
  map<string, int> cellOptionNumber; // ex cellOptionNumber[in01f01] = 0 
  map<string, int> cellToFootprintIndex;
  // fazer um map de celltype para footprint

public:
  LibertyLibrary(const double maxTransition = 0.0f);
  virtual ~LibertyLibrary();


  const pair<int, int> addCellInfo(const LibertyCellInfo & cellInfo); // return = [footprint index][option index]


  const LibertyCellInfo & getCellInfo(const string & footPrint, const int & i) const;
  const LibertyCellInfo & getCellInfo(const string & cellName) const;
  const LibertyCellInfo & getCellInfo(const int & footPrintIndex, const int & optionIndex) const;

  const pair<int, int> getCellIndex(const string &cellName) const;
  double getMaxTransition() const ;



  /* data */
};

enum Unateness {
  NEGATIVE_UNATE, POSITIVE_UNATE, NON_UNATE
};

class LibertyLookupTableInterpolator
{
protected:
    static const int DEFAULT_DECIMAL_PLACES;
    void round(Transitions<double> & transitions, const int decimal_places);
public:
  virtual double interpolate(const LibertyLookupTable & lut, const double load, const double transition) = 0;
  virtual const Transitions<double> interpolate(const LibertyLookupTable & riseLut, const LibertyLookupTable & fallLut, const Transitions<double> load, const Transitions<double> transition, Unateness unateness = NEGATIVE_UNATE) = 0;

  /* data */
};

class LinearLibertyLookupTableInterpolator : public LibertyLookupTableInterpolator
{
public:
  double interpolate(const LibertyLookupTable & lut, const double load, const double transition);
  const Transitions<double> interpolate(const LibertyLookupTable & riseLut, const LibertyLookupTable & fallLut, const Transitions<double> load, const Transitions<double> transition, Unateness unateness = NEGATIVE_UNATE);

};

#endif












