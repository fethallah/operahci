#ifndef _FRACTION_H_INCLUDED_
#define _FRACTION_H_INCLUDED_

#if 0

// A class for holding rational numbers as fractions a/b.
// No .cpp file - all content defined here.

#include <math.h>

namespace NIMacro {

// Accuracy of double to fractional conversions. 
const double tiny_value=1E-6;

struct Fraction {
	int a_, b_;
	Fraction(int a, int b=1): a_(a), b_(b) {}
	Fraction(double f);
	double Double() const {return double(a_)/b_;}
	Fraction& Normalize();
};

inline Fraction operator*(const Fraction& x, int y) { return Fraction(x.a_ * y, x.b_).Normalize(); }
inline Fraction operator*(int x, const Fraction& y) { return Fraction(y.a_ * x, y.b_).Normalize(); }
inline Fraction operator/(const Fraction& x, int y) { return Fraction(x.a_, x.b_ * y).Normalize(); }
inline Fraction operator/(int x, const Fraction& y) { return Fraction(y.a_, x * y.b_).Normalize(); }
inline double operator*(const Fraction& x, double y) { return x.Double()*y; }
inline double operator*(double x, const Fraction& y) { return x*y.Double(); }
inline double operator/(const Fraction& x, double y) { return x.Double()/y;}
inline double operator/(double x, const Fraction& y) { return x*y.b_/y.a_;}
inline Fraction operator*(const Fraction& x, const Fraction& y) { return Fraction(x.a_ * y.a_, x.b_*y.b_).Normalize(); }
inline Fraction operator/(const Fraction& x, const Fraction& y) { return Fraction(x.a_ * y.b_, x.b_*y.a_).Normalize(); }
inline Fraction operator+(const Fraction& x, const Fraction& y) { return Fraction(x.a_ * y.b_ + x.b_*y.a_, x.b_*y.b_).Normalize(); }
inline bool operator!=(const Fraction& x, int y) { return !(x.a_==y && x.b_==1); }
inline bool operator==(const Fraction& x, int y) { return (x.a_==y && x.b_==1); }


inline int gcd(int a, int b) {	// inline here just to keep compilers happy by inclusion of this header in different object files.
	if (b == 0) return (a); 
	else return (gcd(b, a % b)); 
} 

inline Fraction& Fraction::Normalize() {
	int k=gcd(a_,b_);
	a_/=k;
	b_/=k;
	return *this;
}


inline Fraction::Fraction(double f) {
	// Convert double f to a fraction d/m, with small numbers d and m, if possible.
	if (f >= 1.0-tiny_value && fabs(f-iround(f)) < tiny_value) {
		// f is an integer
		a_ = iround(f);
		b_ = 1;
		return;
	}
	double g=1/f;
	if (f < 1.0) {
		if (fabs(g-iround(g)) < tiny_value) {
			// f is an inverse integer
			a_= 1;
			b_= iround(g);
			return;
		}
	}

	// Convert f to a/b; fix b and calculate a
/*
	// By powers of 10:
	if ( fabs(f*10000 - iround(f*10000))<0.001) {
		for (int x=1000; x>1; x/=10) {
			if ( fabs(f*x - iround(f*x))> 0.05) {
				int b = x*10;
				int a = iround( f*b);
				int k = gcd(a,b);
				divisor = a/k;
				multiplier = b/k;
				return;
			}
		}
	}

	int b = DENOM, a = iround(f*b);
	// reduce fraction
	int k = gcd(a,b);
	divisor = a/k;
	multiplier = b/k;
*/

	int k[] = {1,2,3,5,7,11,13,17}, n=sizeof(k)/sizeof(int), a_best=iround(f*1000), b_best=1000;
	double Goodness_so_far = 1E6;

	for (int x=0; x<n; x++) {
	for (int y=x; y<n; y++) {
	for (int z=y; z<n; z++) {
		int b = k[x]*k[y]*k[z];
		int a = iround(f*b);
		double Goodness = fabs( double(a)/b - f)*g; 
		if (Goodness < Goodness_so_far ) {
			a_best = a;
			b_best = b;
			Goodness_so_far  = Goodness;
		}
		if (Goodness_so_far < 1E-8) {
			goto Okay;
		}
	}
	}
	}

Okay:
	
	int m = gcd(a_best, b_best);
	a_ = a_best/m;
	b_ = b_best/m;
}

} // namespace NIMacro
#endif

#endif
