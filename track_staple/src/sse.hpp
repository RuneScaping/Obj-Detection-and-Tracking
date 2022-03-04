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
RETf SET( float x, float y, float z, float w ) { return 