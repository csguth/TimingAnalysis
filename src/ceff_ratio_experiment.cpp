#include "include/ceff_ratio_experiment.h"


void Ceff_Ratio_Experiment::run_sorted_by_wire_size(Timing_Analysis::Timing_Analysis &ta)
{
    priority_queue<ratio_t, std::vector<ratio_t>, resistance_comparator > pq;
    priority_queue<ratio_t, std::vector<ratio_t>, std::greater<ratio_t> > pq1;
    priority_queue<ratio_t, std::vector<ratio_t>, std::greater<ratio_t> > pq2;
    priority_queue<ratio_t, std::vector<ratio_t>, std::greater<ratio_t> > pq3;

    double min_resistance = numeric_limits<double>::max();
    double max_resistance = numeric_limits<double>::min();
    double average_resistance = 0.0f;
    int num_items = 0;
    for(int i = 0; i < ta.timing_points_size(); i++)
    {
        const Timing_Analysis::Timing_Point & tp = ta.timing_point(i);
        if(tp.is_PI() || tp.is_output_pin())
        {
            num_items++;
            const double total_resistance = tp.net().wire_delay_model()->total_resistance();
            average_resistance += total_resistance;
            max_resistance = max(max_resistance, total_resistance);
            min_resistance = min(min_resistance, total_resistance);
            pq.push(ratio_t { tp.ceff().getMax() / tp.net().wire_delay_model()->lumped_capacitance(), total_resistance});
        }
    }
    average_resistance /= num_items;

    int count = 0;
    while(!pq.empty())
    {
        ratio_t item = pq.top();
        pq.pop();
        if(count <= num_items * 1/3)
            pq1.push(item);
        else if(count <= num_items * 2/3)
            pq2.push(item);
        else if(count <= num_items)
            pq3.push(item);
        count++;
    }

    cout << "####" << Traits::ispd_contest_benchmark << endl;
    cout << "max " << max_resistance << " min " << min_resistance << " avg " << average_resistance << endl;
    cout << "ratio\tresistance" << endl;
    while(!pq1.empty())
    {
        ratio_t item = pq1.top();
        pq1.pop();
        cout << item << endl;
    }
    cout << "#" << endl;
    while(!pq2.empty())
    {
        ratio_t item = pq2.top();
        pq2.pop();
        cout << item << endl;
    }
    cout << "#" << endl;
    while(!pq3.empty())
    {
        ratio_t item = pq3.top();
        pq3.pop();
        cout << item << endl;
    }

}

void Ceff_Ratio_Experiment::run_sorted_by_slew(Timing_Analysis::Timing_Analysis &ta)
{
    priority_queue<ratio_t, std::vector<ratio_t>, slew_comparator > pq;
    priority_queue<ratio_t, std::vector<ratio_t>, std::greater<ratio_t> > pq1;
    priority_queue<ratio_t, std::vector<ratio_t>, std::greater<ratio_t> > pq2;
    priority_queue<ratio_t, std::vector<ratio_t>, std::greater<ratio_t> > pq3;

    double min_resistance = numeric_limits<double>::max();
    double max_resistance = numeric_limits<double>::min();
    double average_resistance = 0.0f;
    int num_items = 0;
    for(int i = 0; i < ta.timing_points_size(); i++)
    {
        const Timing_Analysis::Timing_Point & tp = ta.timing_point(i);
        if(tp.is_PI() || tp.is_output_pin())
        {
            num_items++;
            const double total_resistance = tp.net().wire_delay_model()->total_resistance();
            average_resistance += total_resistance;
            max_resistance = max(max_resistance, total_resistance);
            min_resistance = min(min_resistance, total_resistance);

            int num_of_timing_arcs = 0;
            int tp_index = i-1;
            while(tp_index >= 0 && ta.timing_point(tp_index).gate_number() == tp.gate_number())
            {
                tp_index--;
                num_of_timing_arcs++;
            }

            Transitions<double> slew = numeric_limits<Transitions<double> >::min();
            for(int j = i - num_of_timing_arcs; j < i; j++)
            {
                const Timing_Analysis::Timing_Point & input_pin = ta.timing_point(j);
                slew = max(slew, input_pin.arc().slew());
            }


            pq.push(ratio_t { tp.ceff().getMax() / tp.net().wire_delay_model()->lumped_capacitance(), total_resistance, slew.getMax() });
        }
    }
    average_resistance /= num_items;

    int count = 0;
    while(!pq.empty())
    {
        ratio_t item = pq.top();
        pq.pop();
        if(count <= num_items * 1/3)
            pq1.push(item);
        else if(count <= num_items * 2/3)
            pq2.push(item);
        else if(count <= num_items)
            pq3.push(item);
        count++;
    }

    cout << "####" << Traits::ispd_contest_benchmark << endl;
    cout << "max " << max_resistance << " min " << min_resistance << " avg " << average_resistance << endl;
    cout << "ratio\tresistance" << endl;
    while(!pq1.empty())
    {
        ratio_t item = pq1.top();
        pq1.pop();
        cout << item << endl;
    }
    cout << "#" << endl;
    while(!pq2.empty())
    {
        ratio_t item = pq2.top();
        pq2.pop();
        cout << item << endl;
    }
    cout << "#" << endl;
    while(!pq3.empty())
    {
        ratio_t item = pq3.top();
        pq3.pop();
        cout << item << endl;
    }
}
