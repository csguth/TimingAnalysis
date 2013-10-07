#include "include/timer.h"

const double Timer::MICRO = 1000000;
const double Timer::MILI = 1000;

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
    assert(time_definition == MICRO || time_definition == MILI);
    _execution_time.set(_execution_time._time * time_definition, (time_definition == MICRO ? "us" : "ms"));
    return _execution_time;
}
