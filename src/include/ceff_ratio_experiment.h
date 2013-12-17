#ifndef CEFF_RATIO_EXPERIMENT_H
#define CEFF_RATIO_EXPERIMENT_H

#include "timing_analysis.h"
#include <queue>
using std::priority_queue;

#include <ostream>
using std::ostream;

#include "transitions.h"
class Ceff_Ratio_Experiment
{

    struct ratio_t {
        double ratio;
        double total_resistance;
        double slew;

        bool operator >(const ratio_t & o) const
        {
            return true;
            return ratio > o.ratio;
        }
        friend ostream & operator << (ostream & out, ratio_t & r)
        {
            return out << r.ratio << "\t" << r.total_resistance;
        }
    };

    struct resistance_comparator {
        bool operator()(const ratio_t & a, const ratio_t & b)
        {
            return a.total_resistance > b.total_resistance;
        }
    };

    struct slew_comparator {
        bool operator()(const ratio_t & a, const ratio_t & b)
        {
            return a.slew > b.slew;
        }
    };

public:
    static void run_sorted_by_wire_size(Timing_Analysis::Timing_Analysis & ta);
    static void run_sorted_by_slew(Timing_Analysis::Timing_Analysis & ta);
    static void run_average_calculation(Timing_Analysis::Timing_Analysis & ta);

};

#endif // CEFF_RATIO_EXPERIMENT_H
