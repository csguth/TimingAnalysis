#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include "include/timing_analysis.h"
#include "include/parser.h"
#include "include/circuit_netlist.h"
#include "include/spef_net.h"

#include "include/configuration.h"
#include "include/timer.h"
#include "include/ceff_ratio_experiment.h"
#include "include/slew_degradation_experiment.h"

#include <cstdio>
#include <queue>
using std::priority_queue;

#include <ostream>
using std::ostream;

using std::make_pair;

struct PassingArgs {
    string _contest_root;
    string _contest_benchmark;
    PassingArgs(string contestRoot,	string contestBenchmark) : _contest_root(contestRoot), _contest_benchmark(contestBenchmark){}
};

struct ISPDContestFiles
{
    string verilog;
    string spef;
    string liberty;
    string designConstraints;
    ISPDContestFiles(string contestRoot, string contestBenchmark) {
        verilog = contestRoot + "/" + contestBenchmark + "/" + contestBenchmark + ".v";
        spef = contestRoot + "/" + contestBenchmark + "/" + contestBenchmark + ".spef";
        designConstraints = contestRoot + "/" + contestBenchmark + "/" + contestBenchmark + ".sdc";
        liberty = contestRoot + "/lib/contest.lib";
    }
};

int main(int argc, char const *argv[])
{
    if(argc != 3)
    {
        cerr << "Using: " << argv[0] << " <CONTEST_ROOT> <CONTEST_BENCHMARK>" << endl;
        return -1;
    }

    const PassingArgs args(argv[1], argv[2]);
    Traits::ispd_contest_root = argv[1];
    Traits::ispd_contest_benchmark = argv[2];

    VerilogParser vp;
    LibertyParser lp;
    SpefParser sp;
    SDCParser dcp;

    ISPDContestFiles files(argv[1], argv[2]);

    const Circuit_Netlist netlist = vp.readFile(files.verilog);
    const LibertyLibrary library = lp.readFile(files.liberty);
    const Parasitics parasitics = sp.readFile(files.spef);
    const Design_Constraints constraints = dcp.readFile(files.designConstraints);




    Timing_Analysis::Timing_Analysis ta(netlist, &library, &parasitics, &constraints);
//    ta.full_timing_analysis();
//    ta.call_prime_time();
//    ta.print_circuit_info();

//    ta.full_timing_analysis();
//    ta.print_circuit_info();
    ta.full_timing_analysis();


    for(int i = 0; i < ta.timing_points_size(); i++)
    {
        cout << ta.timing_point(i).name() << ",\t";
    }
    cout << endl;

    cout << "[";
    for(int i = 0; i < ta.timing_points_size(); i++)
    {
//        cout << i+1 << "\t";
        const Timing_Analysis::Timing_Point & tp = ta.timing_point(i);
        for(int j = 0; j < ta.timing_points_size(); j++)
        {
            if(tp.is_input_pin())
            {
                const Timing_Analysis::Timing_Arc & arc = tp.arc();
                cout << (&arc.to() - &ta.timing_point(0) == j ? arc.to().arrival_time().getRise()-tp.arrival_time().getRise() : 0) << (j == ta.timing_points_size()-1 && i != ta.timing_points_size()-1?";\n":",");
            } else if(tp.is_reg_input() || tp.is_PI_input())
            {
                const Timing_Analysis::Timing_Arc & arc = tp.arc();
                cout << (&arc.to() - &ta.timing_point(0) == j ? arc.to().arrival_time().getRise()-tp.arrival_time().getRise() : 0) << (j == ta.timing_points_size()-1 && i != ta.timing_points_size()-1?";\n":",");
            } else if(tp.is_output_pin() || tp.is_PI())
            {
                const Timing_Analysis::Timing_Net & net = tp.net();
                for(int k = 0; k < net.fanouts_size(); k++)
                    cout << (&net.to(k) - &ta.timing_point(0) == j ? net.to(k).arrival_time().getRise()-tp.arrival_time().getRise() : 0) << (j == ta.timing_points_size()-1 && i != ta.timing_points_size()-1?";\n":",");
            }
            else if(tp.is_PO())
            {
                if(i == ta.timing_points_size() - 1)
                    cout << 0 << (j == ta.timing_points_size()-1?"":",");
                else
                    cout << 0 << (j == ta.timing_points_size()-1?";\n":",");
            }
        }

    }

    cout << "]\n";


    ta.print_circuit_info();
    return 0;
}
