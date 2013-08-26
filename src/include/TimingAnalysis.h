#ifndef TIMING_ANALYSIS_H_
#define TIMING_ANALYSIS_H_

#include <vector>
using std::vector;

#include <string>
using std::string;

#include <ostream>
using std::ostream;

#include <map>
using std::map;

#include <utility>
using std::pair;

#include <fstream>
using std::fstream;

#include <cstdlib>

#include "timer_interface.h"

#include "Transitions.h"
#include "CircuitNetList.h"
#include "WireDelayModel.h"
#include "LibertyLibrary.h"
#include "DesignConstraints.h"

#include "Configuration.h"

#include "Parser.h"

namespace TimingAnalysis
{
	class TimingNet;
	class TimingArc;

	enum TimingPointType
	{
		INPUT, OUTPUT, PI_INPUT, REGISTER_INPUT, PI, PO
	};

	class TimingPoint
	{
		friend class TimingAnalysis;
		friend class TimingNet;
		string name;
		TimingNet * net;
		TimingArc * arc;
		Transitions<double> slack;
		Transitions<double> slew;
		Transitions<double> arrivalTime;
		int gateNumber;

		TimingPointType type;


	public:
		TimingPoint(string name, const int gateNumber, TimingPointType type) : name(name), net(0), arc(0), slack(0.0f, 0.0f), slew(0.0f, 0.0f), arrivalTime(0.0f, 0.0f), gateNumber(gateNumber), type(type)
		{
            switch(type)
            {
            case REGISTER_INPUT:
                slew = Transitions<double>(80.0f, 80.0f);
                break;
            }

		};
		virtual ~TimingPoint(){};

		const Transitions<double> updateSlack(const Transitions<double> requiredTime)
		{
			slack = requiredTime - arrivalTime;
			return slack;
		}
		const Transitions<double> getRequiredTime() const {return slack + arrivalTime;}

		friend ostream & operator<<(ostream & out, const TimingPoint & tp)
		{
			return out << tp.name << " slack " << tp.slack << " slew " << tp.slew << " arrival " << tp.arrivalTime;
		};

		const string getName() const { return name; }
		const int getGateNumber() const  { return gateNumber; }


		bool isPO() const { return type == PO; }
		bool isPI() const { return type == PI; }
		bool isInputPin() const { return type == INPUT; }
		bool isOutputPin() const { return type == OUTPUT; }
		bool isPIInput() const { return type == PI_INPUT; }
		bool isRegInput() const { return type == REGISTER_INPUT; }


		void clearTimingInfo(){
			slack = Transitions<double>(0.0f, 0.0f);
			slew = Transitions<double>(numeric_limits<double>::min(), numeric_limits<double>::min());
			arrivalTime = Transitions<double>(numeric_limits<double>::min(), numeric_limits<double>::min());
		}

		const double load() const;

		const Transitions<double> getArrivalTime() const 
		{
			return arrivalTime;
		}



	};


	class Edge {
		TimingPoint * from;
		vector<TimingPoint *> to;
	protected:
		Edge(TimingPoint * from) : from(from) {};
		TimingPoint * _getFrom() const { return from; }
		TimingPoint * _getTo(const int i) const { return to.at(i); }
		void _addFanout(TimingPoint * tp) { to.push_back(tp); }
		void _setFanout(const int i, TimingPoint * tp)
		{
			if(to.empty() && !i)
				to.push_back(tp);
			else
				to[i] = tp;
			assert(to.size() == 1);
		}
	public:
		size_t fanoutsSize() const { return to.size(); };
	};

	class OneFanoutEdge : public Edge
	{
	public:
		OneFanoutEdge(TimingPoint * from, TimingPoint * to) : Edge(from) {
			_setFanout(0, to);
		}
		TimingPoint * getFrom() const { return _getFrom(); }
		TimingPoint * getTo() const { return _getTo(0); }
		
	};

	class MultiFanoutEdge : public Edge
	{
	public:
		MultiFanoutEdge(TimingPoint * from) : Edge(from) {};
		TimingPoint * getFrom() const { return _getFrom(); }
		TimingPoint * getTo(const int i) const { return _getTo(i); }
		
	};

	class TimingArc : public OneFanoutEdge
	{
		friend class TimingAnalysis;
		Transitions<double> delay;
		Transitions<double> slew;
		int gateNumber;
		int arcNumber;
	public:
		TimingArc(TimingPoint * from, TimingPoint * to, const int arcNumber) : delay(0.0f, 0.0f), slew(0.0f, 0.0f), arcNumber(arcNumber), OneFanoutEdge(from, to)
		{
		};
		virtual ~TimingArc(){};


		friend ostream & operator<<(ostream & out, const TimingArc & ta)
		{
			return out << ta.getFrom()->getName() << " -> " << ta.getTo()->getName();
		};

	};

	class TimingNet : public MultiFanoutEdge
	{
		friend class TimingAnalysis;
		friend class TimingPoint;
		string netName;
		WireDelayModel * wireDelayModel;

	public:
		TimingNet(const string & netName, TimingPoint * from, WireDelayModel * wireDelayModel)
		:netName(netName), MultiFanoutEdge(from), wireDelayModel(wireDelayModel)
		{
		}
		virtual ~TimingNet(){

		}
		void addFanout(TimingPoint * tp) {
			_addFanout(tp);
		};

		const string getName() const
		{
			return netName;
		}
		friend ostream & operator<<(ostream & out, const TimingNet & tn)
		{
			out << tn.netName << " " << (tn.getFrom() != 0 ? tn.getFrom()->getName() : "source") << " -> (";
			for(int i = 0; i < tn.fanoutsSize(); i++)
				out << (tn.getTo(i) != 0 ? tn.getTo(i)->getName() : "sink") << " ";
			out << ")";
			return out;
		};
	};

	struct Option
	{
		int footprintIndex;
		int optionIndex;
		bool dontTouch;

		Option():footprintIndex(-1), optionIndex(-1), dontTouch(false){};
		Option(const int footprintIndex, const int optionIndex) : footprintIndex(footprintIndex), optionIndex(optionIndex), dontTouch(false){};
	};

	

	

	

	class TimingAnalysis
	{
		vector<TimingPoint> points;
		vector<TimingArc> arcs;
		vector<TimingNet> nets;

		vector<Option> options;

		map<int, double> poLoads;
		map<string, int> pinNameToTimingPointIndex;
		static const Transitions<double> ZERO_TRANSITIONS;
		static const Transitions<double> MIN_TRANSITIONS;
		static const Transitions<double> MAX_TRANSITIONS;

		const LibertyLibrary * library;
		const Parasitics * parasitics;
		LibertyLookupTableInterpolator * interpolator;


		Transitions<double> targetDelay;
		Transitions<double> maxTransition;

		void updateTiming(const int i);
		void updateSlacks(const int i);

		Transitions<double> criticalPathValues;
		Transitions<double> totalNegativeSlack;
		Transitions<double> slewViolations;
		Transitions<double> capacitanceViolations;


		vector<pair<int, string> > _verilog;
        vector<bool> _dirty;


		const Transitions<double> getGateDelay(const int gateIndex, const int inputNumber, const Transitions<double> transition, const Transitions<double> ceff);



		const LibertyCellInfo & option(const int nodeIndex)
		{
			return library->getCellInfo(options.at(nodeIndex).footprintIndex, options.at(nodeIndex).optionIndex);
		}

		// TOPOLOGY INIT
		const pair<size_t, size_t> createTimingPoints(const int i,const CircuitNetList::LogicGate & gate,const pair<int, int> cellIndex, const LibertyCellInfo & cellInfo);
		void getNumberOfTimingPointsAndTimingArcs(int & numberOfTimingPoints, int & numberOfTimingArcs, const CircuitNetList & netlist, const LibertyLibrary * lib);
		void createTimingArcs(const pair<size_t, size_t> tpIndexes, const bool is_pi , const bool is_po );


		// PRIMETIME CALLING
		const vector<pair<string, string> > get_sizes_vector();
		bool check_timing_file(const string timing_file);

		// OUTPUT METHODS
		void writeSizesFile(const string filename);

	public:
		const double pinCapacitance(const int timingPointIndex);
		TimingAnalysis(const CircuitNetList netlist, const LibertyLibrary * lib, const Parasitics * parasitics, const DesignConstraints * sdc);
		virtual ~TimingAnalysis();
		void fullTimingAnalysis();


		// GETTERS
		const size_t timingPointsSize() { return points.size(); } 
		const TimingPoint & timingPoint( const int i ) { return points.at(i); } 

		const size_t timingArcsSize() { return arcs.size();	}
		const TimingArc & timingArc( const int i ) { return arcs.at(i); }

		const size_t timingNetsSize() { return nets.size(); }
		const TimingNet & timingNet( const int i ) { return nets.at(i); }

		// SETTERS
		bool setGateOption(const int gateIndex, const int optionNumber)
		{
			if(options.at(gateIndex).dontTouch)
				return false;
			options.at(gateIndex).optionIndex = optionNumber;
			return true;
		}

		void printInfo();
		void printCircuitInfo();


		bool validate_with_prime_time();


	};

};

#endif
