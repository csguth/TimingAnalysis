#ifndef TIMER_H
#define TIMER_H

#include <sys/time.h>
#include <cstdlib>

#include <string>
using std::string;
#include <cassert>
#include <ostream>
using std::ostream;

class Timer
{
    struct timeval _start_time;
    struct timeval _stop_time;


public:

    class Result
    {
        friend class Timer;
        float _time;
        string _unity;


         public:
        void set(float time, string unity){
            _time = time;
            _unity = unity;
        }
        friend ostream & operator << (ostream & out, const Timer::Result & result)
        {
            return out << result._time << " " << result._unity;
        }

        float time() const { return _time; }
    };

private:
    Timer::Result _execution_time;
public:
    Timer();
    virtual ~Timer();


    void start();
    void end();

    const Timer::Result &value(const double time_definition);

    static const double MICRO;
    static const double MILI;
};

#endif // TIMER_H
