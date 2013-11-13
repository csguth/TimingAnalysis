#include "include/timer.h"

const double Timer::MICRO = 1000000.0f;
const string Timer::micro = "us";
const double Timer::MILI = 1000.0f;
const string Timer::mili = "ms";
const double Timer::SECOND = 1.0f;
const string Timer::second = "s";

Timer::Timer()
{

}

Timer::~Timer()
{

}

void Timer::start()
{
    gettimeofday(&_start_time, NULL);
}

void Timer::end()
{
    gettimeofday(&_stop_time, NULL);
    float time = (float) (_stop_time.tv_sec - _start_time.tv_sec);
    time += (float) (_stop_time.tv_usec - _start_time.tv_usec) / MICRO;
    _execution_time.set(time, string("s"));
}

const Timer::Result & Timer::value(const double time_definition)
{
    assert(time_definition == MICRO || time_definition == MILI || time_definition == SECOND);

    string def;
    if(time_definition == MICRO)
        def = Timer::micro;
    else if (time_definition == MILI)
        def = Timer::mili;
    else if (time_definition == SECOND)
        def = Timer::second;

    _execution_time.set(_execution_time._time * time_definition, def);
    return _execution_time;
}
