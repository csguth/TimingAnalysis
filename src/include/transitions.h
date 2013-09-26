#ifndef TRANSITIONS_H_
#define	TRANSITIONS_H_

#include <algorithm>
using std::swap;
using std::max;
using std::min;

#include <ostream>
using std::ostream;

#include <cmath>

#include <limits>
using std::numeric_limits;

#define MAKE_SELF_OPERATOR( OP ) \
friend void operator OP ( Transitions<T> &v0, const Transitions<T> v1 ) { v0[RISE] OP v1[RISE], v0[FALL] OP v1[FALL]; } \
friend void operator OP ( Transitions<T> &v0, const T            v1 ) { v0[RISE] OP v1; v0[FALL] OP v1; }

#define MAKE_OPERATOR( OP ) \
friend Transitions<T> operator OP ( const Transitions<T> v0, const Transitions<T> v1 ) { return Transitions<T>(v0[RISE] OP v1[RISE], v0[FALL] OP v1[FALL]); } \
friend Transitions<T> operator OP ( const T            v0, const Transitions<T> v1 ) { return Transitions<T>(v0       OP v1[RISE], v0       OP v1[FALL]); } \
friend Transitions<T> operator OP ( const Transitions<T> v0, const T            v1 ) { return Transitions<T>(v0[RISE] OP v1      , v0[FALL] OP v1      ); }

enum EdgeType {
	RISE = 0, FALL = 1
};

template<typename T>
/** @brief Template class which encapsulates any [rise,fall] twosome values and provides methods to work with it
*/
class Transitions {

	/** @brief Redefinition of << operator. Inserts formatted description including rise and fall times, in this order
	 *
	 *@param ostream &out, const Transitions<T> array
	 */
	friend ostream &operator<<(ostream &out, const Transitions<T> array ) {
		return out << "(" << array[RISE] << ", " << array[FALL] << ")";
	} // end operator

	MAKE_OPERATOR(+);
	MAKE_OPERATOR(-);
	MAKE_OPERATOR(*);
	MAKE_OPERATOR(/);

	MAKE_SELF_OPERATOR(+=);
	MAKE_SELF_OPERATOR(-=);
	MAKE_SELF_OPERATOR(*=);
	MAKE_SELF_OPERATOR(/=);
	/** @brief Redefinition of (-) operator. Inserts formatted description of Transitions<T> containing its sign of rise and fall time values with inverted signs
	 *
	 *@param const Transitions<T> &v0
	 *
	 *@return Transition<T>
	 */
	friend Transitions<T> operator-( const Transitions<T> &v0 ) { return Transitions<T>(-v0[RISE], -v0[FALL]); }
	/** @brief Definition of max operator. Returns Transition<T> with the higher values for rise and fall transition times
	 *
	 *@param const Transitions<T> v0
	 *
	 *@return Transition<T>
	 */
	friend Transitions<T> max( const Transitions<T> v0, const Transitions<T> v1 ) { return Transitions<T>(max(v0[RISE],v1[RISE]),max(v0[FALL],v1[FALL])); }
	/** @brief Definition of min operator. Returns Transition<T> with the lower values for rise and fall transition times
	 *
	 *@param const Transitions<T> v0, const Transitions<T> v1
	 *
	 *@return Transition<T>
	 */
	friend Transitions<T> min( const Transitions<T> v0, const Transitions<T> v1 ) { return Transitions<T>(min(v0[RISE],v1[RISE]),min(v0[FALL],v1[FALL])); }
	/** @brief Definition of abs operator. Returns Transition<T> with the absolute values in IEEE floating point representation of rise and fall transition times
	 *
	 *@param const Transitions<T> v
	 *
	 *@return Transition<T>
	 */
	friend Transitions<T> abs( const Transitions<T> v ) { return Transitions<T>(fabs(v[RISE]),fabs(v[FALL])); }
	/** @brief Definition of pow operator. Returns Transition<T> with rise and fall transition time values raised to exp
	 *
	 *@param const Transitions<T> v, const double exp
	 *
	 *@return Transition<T>
	 */
	friend Transitions<T> pow( const Transitions<T> v, const double exp ) { return Transitions<T>(pow(v[RISE], exp),pow(v[FALL], exp)); }
	/** @brief Definition of abs operator. Returns Transition<T> with the square root values of rise and fall transition times 
	 *
	 *@param const Transitions<T> v
	 *
	 *@return Transition<T>
	 */
	friend Transitions<T> sqrt( const Transitions<T> v) { return Transitions<T>(sqrt(v[RISE]),sqrt(v[FALL])); }
	/** @brief Definition of abs operator. Returns Transition<T> with rise and fall transition time values in the form: <e^(rise time), e^(fall time)>, where 'e' is the Euler number e=2.718...
	 *
	 *@param const Transitions<T> v
	 *
	 *@return Transition<T>
	 */
	friend Transitions<T> exp( const Transitions<T> v) { return Transitions<T>(exp(v[RISE]),exp(v[FALL])); }

private:
	T clsValue[2];
public:
	/** @brief Refefinition of [] operator. Returns value of edgeType
 	 *
	 *@param const EdgeType edgeType
	 *
	 */
	T &operator[]( const EdgeType edgeType ) {return clsValue[edgeType];}
	/** @brief Refefinition of [] operator. Returns value of edgeType
 	 *
	 *@param const EdgeType edgeType
	 *
	 */
	T  operator[]( const EdgeType edgeType ) const {return clsValue[edgeType];}

	/** @brief Refefinition of = operator. Sets caller transition time values to array time values and returns a pointer to it.
 	 *
	 *@param const Transitions & array
	 *
	 */
    Transitions &operator=(const Transitions & array) {
        clsValue[RISE] = array[RISE];
        clsValue[FALL] = array[FALL];
		return *this;
	} // end operator

	/** @brief Transitions default constructor
	 *
	 *@param const T rise, const T fall
	 *
	 */
	Transitions(const T rise, const T fall ) {
		clsValue[RISE] = rise;
		clsValue[FALL] = fall;
	} // end constructor

   /** @brief Empty Transitions constructor
	*
	*/
	Transitions(){};

	/** @brief Sets rise and fall time values
	 *
	 *@param const T rise, const T fall
	 *
	 *@return void
	 */
	void set( const T rise, const T fall ) {
		clsValue[RISE] = rise;
		clsValue[FALL] = fall;
	} // end method

	/** @brief Returns max value between rise and fall time
 	 *
	 *@return T
	 */
	T getMax() const { return max(clsValue[RISE], clsValue[FALL]); }
	/** @brief Returns min value between rise and fall time
 	 *
	 *@return T
	 */
	T getMin() const { return min(clsValue[RISE], clsValue[FALL]); }
	/** @brief Returns rise time value
 	 *
	 *@return T
	 */
	T getRise() const { return clsValue[RISE]; }
	/** @brief Returns fall time value
 	 *
	 *@return T
	 */
	T getFall() const { return clsValue[FALL]; }

	/** @brief Returns Transitions with exchanged value. Ex: If called by Transitions<rise, fall>, returns Transitions<fall, rise>
 	 *
	 *@return Transitions<T>
	 */
	Transitions<T> getReversed() const { return Transitions(getFall(), getRise()); }

	/** @brief Exchanges rise and fall time values. Ex: Transitions<rise, fall> becomes Transitions<fall, rise>
 	 *
	 *@return T
	 */
	void reverse() { return swap(clsValue[RISE], clsValue[FALL]); }

	/** @brief Returns the sum of rise and fall time values
 	 *
	 *@return T
	 */
	T aggregate() const { return clsValue[RISE] + clsValue[FALL]; }
}; // end class

namespace std {
    template<>
	/** @brief Class used to make easier to work with double variable numeric limits
	 *
	 */
    class numeric_limits<Transitions<double> > {
    public:
		/** @brief Sets Returns a Transition with the minimum values of rise and fall times possible in IEEE floating point double precision representation
		*
		*@return Transitions<double>
		*/
        static Transitions<double> min(){return Transitions<double>(numeric_limits<double>::min(), numeric_limits<double>::min());}
		/** @brief Sets Returns a Transition with the maximum values of rise and fall times possible in IEEE floating point double precision representation
		*
		*@return Transitions<double>
		*/
        static Transitions<double> max(){return Transitions<double>(numeric_limits<double>::max(), numeric_limits<double>::max());}
		/** @brief Sets Returns a Transition with value zero for rise and fall times
		*
		*@return Transitions<double>
		*/
        static Transitions<double> zero(){return Transitions<double>(0.0f, 0.0f);}
    };
}

#endif

