#ifndef SLEW_DEGRADATION_EXPERIMENT_H
#define SLEW_DEGRADATION_EXPERIMENT_H

#include "timing_analysis.h"

#include <utility>
using std::pair;
using std::make_pair;

#include <queue>
using std::queue;

class Slew_Degradation_Experiment
{
    static bool nearly_equals(const Transitions<double> a, const Transitions<double> b);
    static const double EPSILON;
public:
    static void run(Timing_Analysis::Timing_Analysis &ta);
};

#endif // SLEW_DEGRADATION_EXPERIMENT_H
