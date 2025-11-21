#ifndef FFT_H
#define FFT_H

#include <complex>
#include <valarray>

// Renamed to Cpx to avoid X11 collision
typedef std::complex<double> Cpx;
typedef std::valarray<Cpx> CArray;

void computeFFT(CArray& x);

#endif
