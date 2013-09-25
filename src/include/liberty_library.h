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

#include "transitions.h"

//! Struct representing a lookup table to store delay or slew functions
/*!

 Look up table is indexed by the output load and the input transition values
 
   Let L = loadIndices[i]
       T = transitionIndices[j]
   Then, the table value corresponding to L and T will be:
       table[i][j]

*/
struct LibertyLookupTable {

  vector<double> loadIndices ;
  vector<double> transitionIndices ;
  vector<vector<double> > tableVals ;

};
/** @brief Redefinition of <<s operator. Inserts formatted description of *****not implemented
*
*@param ostream& os, LibertyLookupTable& lut
*/
ostream& operator<< (ostream& os, LibertyLookupTable& lut) ;

//!   Struct which has informations about timing
/*!   
	  "non_unate" or "negative_unate" or "positive_unate".
      Note that ISPD-13 library will have only negative-unate combinational cells. The clock arcs
      for sequentials will be non_unate (which can be ignored because of the simplified sequential
*/
struct LibertyTimingInfo {

  string fromPin ;
  string toPin ;
  string timingSense ;
  timing model for ISPD-13).

  
  LibertyLookupTable fallDelay ;
  LibertyLookupTable riseDelay ;
  LibertyLookupTable fallTransition ;
  LibertyLookupTable riseTransition ;

} ;
/** @brief Redefinition of << operator. Inserts formatted description***** not implemented
 *
 *  @param ostream& os, LibertyTimingInfo& timing
 */
ostream& operator<< (ostream& os, LibertyTimingInfo& timing) ;

/** @brief Struct which has informations about a pin
*/
struct LibertyPinInfo {

  string name ; // pin name
  double capacitance ; // input pin cap (not defined for output pins)
  double maxCapacitance ; // the max load this pin can drive
  bool isInput ; // whether the pin is input or output pin
  bool isClock ; // whether the pin is a clock pin or not

/** @brief LibertyPinInfo default constructor
*/
  LibertyPinInfo () : capacitance (0.0)
  , maxCapacitance (std::numeric_limits<double>::max())
  , isInput(true)
  , isClock(false) {};
  
};

/** @brief Redefinition of << operator. Inserts formatted description***** not implemented
 *
 *  @param ostream& os, LibertyPinInfo& pin
 */
ostream& operator<< (ostream& os, LibertyPinInfo& pin) ;

/** @brief Struct which has informations about a cell
*/
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

/** @brief LibertyCellInfo default constructor
*
*/
  LibertyCellInfo () : leakagePower (0.0), area (0.0), isSequential (false), dontTouch(false), primaryOutput(false) {}
  
} ;

/** @brief Redefinition of << operator. Inserts formatted description***** not implemented
 *
 *  @param ostream& os, LibertyCellInfo& cell
 */
ostream& operator<< (ostream& os, LibertyCellInfo& cell) ;

/** @brief Library which makes easier to get the description of a particular cell
*/
class LibertyLibrary
{
  double maxTransition;
  vector< vector<LibertyCellInfo> > library;

  map<string, int> footPrintToIndex;
  map<string, int> cellOptionNumber; // ex cellOptionNumber[in01f01] = 0 
  map<string, int> cellToFootprintIndex;
  // fazer um map de celltype para footprint

public:
/** @brief LibertyLibrary default constructor
 *
 *  @param const double MaxTransition
 */
  LibertyLibrary(const double maxTransition = 0.0f);

/** @brief Empty LibertyLibrary desctructor
 *
 */
  virtual ~LibertyLibrary();

/** @brief Returns pair<footprint index, option index>
 *
 *  @param const LibertyCellInfo & cellInfo
 *
 *  @return const pair<int, int>
 */
  const pair<int, int> addCellInfo(const LibertyCellInfo & cellInfo); // return = [footprint index][option index]

/** @brief Returns LibertyCellInfo in footPrint parameter, at index i
 *
 *  @param const string & footPrint, const int & i
 *
 *  @return const LibertyCellInfo &
 */
  const LibertyCellInfo & getCellInfo(const string & footPrint, const int & i) const;
/** @brief Returns LibertyCellInfo of cellName name
 *
 *  @param const string & cellName
 *
 *  @return const LibertyCellInfo &
 */
  const LibertyCellInfo & getCellInfo(const string & cellName) const;
/** @brief Returns LibertyCellInfo at index i, of footPrint footPrintIndex
 *
 *  @param const string & footPrintIndex, const int & optionIndex
 *
 *  @return const LibertyCellInfo &
 */
  const LibertyCellInfo & getCellInfo(const int & footPrintIndex, const int & optionIndex) const;

/** @brief Returns pair<footprint index, option index>
 *
 *  @param const string &cellName
 *
 *  @return const pair<int, int>
 */
  const pair<int, int> getCellIndex(const string &cellName) const;
/** @brief Returns maxTransition
 *
 *  @return double
 */
  double getMaxTransition() const ;

  /* data */
};

enum Unateness {
  NEGATIVE_UNATE, POSITIVE_UNATE, NON_UNATE
};
/** @brief Interpolation calculator
*/
class LibertyLookupTableInterpolator
{
protected:
    static const int DEFAULT_DECIMAL_PLACES;
/** @brief Truncates Transitions<double> to Transitions<double> ****************** does nothing...
 *
 *  @param Transitions<double> & transitions, const int decimal_places
 *
 *  @return void
 */
    void round(Transitions<double> & transitions, const int decimal_places);
public:
/** @brief Returns interpolated value
 *
 *  @param const LibertyLookupTable & lut, const double load, const double transition
 *
 *  @return double
 */
  virtual double interpolate(const LibertyLookupTable & lut, const double load, const double transition) = 0;
/** @brief Returns Transitions<double, double> with its rise and fall delay values interpolated
 *
 *  @param const LibertyLookupTable & riseLut, const LibertyLookupTable & fallLut, const Transitions<double> load, const Transitions<double> transition, Unateness unateness(default NEGATIVE_UNATE), bool is_input_driver(default false)
 *
 *  @return const Transitions<double>
 */
  virtual const Transitions<double> interpolate(const LibertyLookupTable & riseLut, const LibertyLookupTable & fallLut, const Transitions<double> load, const Transitions<double> transition, Unateness unateness = NEGATIVE_UNATE, bool is_input_driver = false) = 0;

  /* data */
};
/** @brief Linear interpolation calculator
*/
class LinearLibertyLookupTableInterpolator : public LibertyLookupTableInterpolator
{
public:
/** @brief Returns interpolated value
 *
 *  @param const LibertyLookupTable & lut, const double load, const double transition
 *
 *  @return double
 */
  double interpolate(const LibertyLookupTable & lut, const double load, const double transition);
/** @brief Returns Transitions<double, double> with its rise and fall delay values linearly interpolated
 *
 *  @param const LibertyLookupTable & riseLut, const LibertyLookupTable & fallLut, const Transitions<double> load, const Transitions<double> transition, Unateness unateness(default NEGATIVE_UNATE), bool is_input_driver(default false)
 *
 *  @return const Transitions<double>
 */
  const Transitions<double> interpolate(const LibertyLookupTable & riseLut, const LibertyLookupTable & fallLut, const Transitions<double> load, const Transitions<double> transition, Unateness unateness = NEGATIVE_UNATE, bool is_input_driver = false);

};

#endif
