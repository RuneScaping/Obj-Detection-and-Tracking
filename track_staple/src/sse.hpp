/*******************************************************************************
* Piotr's Computer Vision Matlab Toolbox      Version 3.23
* Copyright 2014 Piotr Dollar.  [pdollar-at-gmail.com]
* Licensed under the Simplified BSD License [see external/bsd.txt]
*******************************************************************************/
#ifndef _SSE_HPP_
#define _SSE_HPP_
#include <emmintrin.h> // SSE2:<e*.h>, SSE3:<p*.h>, SSE4:<s*.h>

namespace sse{

#define RETf inline __m128
#define RETi inline __m128i

// set, load and store values
RETf SET( const float &x ) { return _mm_set1_ps(x); }
RETf SET( float x, float y, float z, float w ) { return _mm_set_ps(x,y,z,w); }
RETi SET( const int &x ) { return _mm_set1_epi32(x); }
RETf LD( const float &x ) { return _mm_load_ps(&x); }
RETf LDu( const float &x ) { return _mm_loadu_ps(&x); }
RETf STR( float &x, const __m128 y ) { _mm_store_ps(&x,y); return y; }
RETf STR1( float &x, const __m128 y ) { _mm_store_ss(&x,y); return y; }
RETf STRu( float &x, const __m128 y ) { _mm_storeu_ps(&x,y); return y; }
RETf STR( float &x, const float y ) { return STR(x,SET(y)); }

// arithmetic operators
RETi ADD( const __m128i x, const __m128i y ) { return _mm_add_epi32(x,y); }
RETf ADD( const __m128 x, const __m128 y ) { return _mm_add_ps(x,y); }
RETf ADD( const __m128 x, const __m128 y, const __m128 z ) {
  return ADD(ADD(x,y),z); }
RETf ADD( const __m128 a, const __m128 b, const __m128 c, const __m128 &d ) {
  return ADD(ADD(ADD(