#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "spef_net.h"
#include "parser.h"
#include <string>
using std::string;

/*

	This is a Configuration File
	To switch between ISPD2012 (Lumped Capacitance Wire Delay Model) or ISPD2013 (Distributed RC Wire Delay Model) SPEF format

*/

// STATIC METAPROGRAMMED IF
template<bool cond, class ThenType, class ElseType>
struct IF
{
	typedef ThenType RET;
};
template<class ThenType, class ElseType>
struct IF<false, ThenType, ElseType>
{
	typedef ElseType RET;
};


// User configurable {


class Traits
{
public:
    enum Wire_Delay_Model_Factory_Types {
        PURI = 0,
        LUMPED_WITH_WIREDELAY
    };

    static const bool ISPD_2012 = false;
    static const double STD_THRESHOLD = 0.01;
    static string ispd_contest_root;
    static string ispd_contest_benchmark;
    static string arrival_time_file_name;
    static const Wire_Delay_Model_Factory_Types WIRE_DELAY_MODEL_TYPE = LUMPED_WITH_WIREDELAY;

};

// }














////////// DON'T TOUCH!!!!
typedef IF<Traits::ISPD_2012, SpefParserISPD2012, SpefParserISPD2013>::RET SpefParser;
typedef IF<Traits::ISPD_2012, SpefNetISPD2012, SpefNetISPD2013>::RET SpefNet; 
typedef IF<Traits::ISPD_2012, Parasitics2012, Parasitics2013>::RET Parasitics;


#endif
