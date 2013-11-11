#include "include/slew_degradation_experiment.h"

const double Slew_Degradation_Experiment::EPSILON = 0.01;

bool Slew_Degradation_Experiment::nearly_equals(const Transitions<double> a, const Transitions<double> b)
{
    return abs(a - b).getMax() <= Slew_Degradation_Experiment::EPSILON;
}

void Slew_Degradation_Experiment::run(Timing_Analysis::Timing_Analysis &ta)
{
//    ta.print_info();

    queue<pair<int, int> > degradation;
    queue<pair<int, int> > nDegradation;

    const double degradation_threshold = 0.2f;

    int yes = 0;
    int no  = 0;
    for(int i = 0; i < ta.timing_points_size(); i++)
    {
        const Timing_Analysis::Timing_Point & tp = ta.timing_point(i);
        if(tp.is_output_pin())
        {
            const Timing_Analysis::Timing_Net & net = tp.net();
            for(int j = 0; j < net.fanouts_size(); j++)
            {
                const Timing_Analysis::Timing_Point & fanout = net.to(j);
                if(!nearly_equals(tp.slew(), fanout.slew()))
                {
                    degradation.push(make_pair(i, j));
                    yes++;
                }
                else
                {
                    nDegradation.push(make_pair(i, j));
                    no++;
                }
            }

        }
    }

    cout << yes << " degradations; " << no << " non-degradations" << endl;

    cout << "slew degradation: " << endl;
    int i = 0;
    while(!degradation.empty())
    {
        pair<int, int> pts = degradation.front();
        degradation.pop();

        const Timing_Analysis::Timing_Point & tp1 = ta.timing_point(pts.first);
        const Timing_Analysis::Timing_Point & tp2 = tp1.net().to(pts.second);

        cout << "degradation[" << i++ << "] = " << tp1.name() << " -> " << tp2.name() << endl;
        cout << " slew degradation = " << tp2.slew() - tp1.slew() << endl;
        cout << "       wire delay = " << tp2.arrival_time() - tp1.arrival_time() << endl;
        cout << "      driver slew = " << tp1.slew() << endl;
        cout << "      fanout slew = " << tp2.slew() << endl;


        if((tp2.arrival_time() - tp1.arrival_time()).getMax() > tp1.slew().getMax() * degradation_threshold)
        {
            cout << "      wire delay (" << tp2.arrival_time() - tp1.arrival_time() << ") is MORE than 20% of max driver slew (" << tp1.slew() << ") (20% = " << tp1.slew() * 0.2f << ")" << endl;
        }
        else
        {
            cout << "      wire delay (" << tp2.arrival_time() - tp1.arrival_time() << ") is LESS than 20% of max driver slew (" << tp1.slew() << ") (20% = " << tp1.slew() * 0.2f << ")" << endl;
        }
        cout << endl;
    }


    cout << "####" << endl;
    i=0;
    while(!nDegradation.empty())
    {
        pair<int, int> pts = nDegradation.front();
        nDegradation.pop();

        const Timing_Analysis::Timing_Point & tp1 = ta.timing_point(pts.first);
        const Timing_Analysis::Timing_Point & tp2 = tp1.net().to(pts.second);

        cout << "non-degradation[" << i++ << "] = " << tp1.name() << " -> " << tp2.name() << endl;
        cout << " slew degradation = " << tp2.slew() - tp1.slew() << endl;
        cout << "       wire delay = " << tp2.arrival_time() - tp1.arrival_time() << endl;
        cout << "      driver slew = " << tp1.slew() << endl;
        cout << "      fanout slew = " << tp2.slew() << endl;

        if((tp2.arrival_time() - tp1.arrival_time()).getMax() > tp1.slew().getMax() * degradation_threshold)
        {
            cout << "      wire delay (" << tp2.arrival_time() - tp1.arrival_time() << ") is MORE than 20% of max driver slew (" << tp1.slew() << ") (20% = " << tp1.slew() * 0.2f << ")" << endl;
        }
        else
        {
            cout << "      wire delay (" << tp2.arrival_time() - tp1.arrival_time() << ") is LESS than 20% of max driver slew (" << tp1.slew() << ") (20% = " << tp1.slew() * 0.2f << ")" << endl;
        }
        cout << endl;
    }
}
