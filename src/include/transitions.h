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
class Transitions {

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

	friend Transitions<T> operator-( const Transitions<T> &v0 ) { return Transitions<T>(-v0[RISE], -v0[FALL]); }

	friend Transitions<T> max( const Transitions<T> v0, const Transitions<T> v1 ) { return Transitions<T>(max(v0[RISE],v1[RISE]),max(v0[FALL],v1[FALL])); }
	friend Transitions<T> min( const Transitions<T> v0, const Transitions<T> v1 ) { return Transitions<T>(min(v0[RISE],v1[RISE]),min(v0[FALL],v1[FALL])); }

	friend Transitions<T> abs( const Transitions<T> v ) { return Transitions<T>(fabs(v[RISE]),fabs(v[FALL])); }
	friend Transitions<T> pow( const Transitions<T> v, const double exp ) { return Transitions<T>(pow(v[RISE], exp),pow(v[FALL], exp)); }
	friend Transitions<T> sqrt( const Transitions<T> v) { return Transitions<T>(sqrt(v[RISE]),sqrt(v[FALL])); }
	friend Transitions<T> exp( const Transitions<T> v) { return Transitions<T>(exp(v[RISE]),exp(v[FALL])); }

private:
	T clsValue[2];
public:
	T &operator[]( const EdgeType edgeType ) {return clsValue[edgeType];}
	T  operator[]( const EdgeType edgeType ) const {return clsValue[edgeType];}

    Transitions &operator=(const Transitions & array) {
        clsValue[RISE] = array[RISE];
        clsValue[FALL] = array[FALL];
		return *this;
	} // end operator

	Transitions(const T rise, const T fall ) {
		clsValue[RISE] = rise;
		clsValue[FALL] = fall;
	} // end constructor

	Transitions(){};

	void set( const T rise, const T fall ) {
		clsValue[RISE] = rise;
		clsValue[FALL] = fall;
	} // end method

	T getMax() const { return max(clsValue[RISE], clsValue[FALL]); }
	T getMin() const { return min(clsValue[RISE], clsValue[FALL]); }
	T getRise() const { return clsValue[RISE]; }
	T getFall() const { return clsValue[FALL]; }

	Transitions<T> getReversed() const { return Transitions(getFall(), getRise()); }

	void reverse() { return swap(clsValue[RISE], clsValue[FALL]); }

	T aggregate() const { return clsValue[RISE] + clsValue[FALL]; }
}; // end class


namespace std {
    template<>
    class numeric_limits<Transitions<double> > {
    public:
        static Transitions<double> min(){return Transitions<double>(numeric_limits<double>::min(), numeric_limits<double>::min());}
        static Transitions<double> max(){return Transitions<double>(numeric_limits<double>::max(), numeric_limits<double>::max());}
        static Transitions<double> zero(){return Transitions<double>(0.0f, 0.0f);}
    };
}

#endif

