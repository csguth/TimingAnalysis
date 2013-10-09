#ifndef TIMER_H
#define TIMER_H

#include <sys/time.h>
#include <cstdlib>

#include <string>
using std::string;
#include <cassert>
#include <ostream>
using std::ostream;
	/** @brief Represents a Timer
	 */
class Timer
{
    struct timeval _start_time;
    struct timeval _stop_time;


public:
	/** @brief Represents the result from the Timer class
	 */
    class Result
    {
        friend class Timer;
        float _time;
        string _unity;


public:
	/** @brief Sets time and unity atributes
	 *
	 *@param float time, string unity
	 *
	 *@return void
	 */
        void set(float time, string unity){
            _time = time;
            _unity = unity;
        }
	/** @brief Redefinition of << operator. Inserts formatted description of Timer containing its result time within its unity
	 *
	 *@param ostream & out, const Timer::Result & result
	 *
	 */
        friend ostream & operator << (ostream & out, const Timer::Result & result)
        {
            return out << result._time << " " << result._unity;
        }
    };

private:
    Timer::Result _execution_time;
public:
	/** @brief Timer empty constructor
	 */
    Timer();
	/** @brief Timer empty destructor
	 */
    virtual ~Timer();

	/** @brief Starts Timer counting
	 *
	 *@return void
	 */
    void start();
	/** @brief Ends the counting and sets the execution time
	 *
	 *@return void
	 */
    void end();

	/** @brief Returns execution time
	 *
	 *@param const double time_definition
	 *
	 *@return const Timer::Result &
	 */
    const Timer::Result &value(const double time_definition);

    static const double MICRO;
    static const double MILI;
};

#endif // TIMER_H
