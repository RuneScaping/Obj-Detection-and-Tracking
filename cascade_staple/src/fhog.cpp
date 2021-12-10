
#include <cstring>
#include <iostream>

using namespace std;

#include "fhog.h"
#undef MIN

// platform independent aligned memory allocation (see also alFree)
void* alMalloc( size_t size, int alignment ) {
  const size_t pSize = sizeof(void*), a = alignment-1;
  void *raw = wrMalloc(size + a + pSize);
  void *aligned = (void*) (((size_t) raw + pSize + a) & ~a);
  *(void**) ((size_t) aligned-pSize) = raw;
  return aligned;
}

// platform independent alignned memory de-allocation (see also alMalloc)
void alFree(void* aligned) {
  void* raw = *(void**)((char*)aligned-sizeof(void*));
  wrFree(raw);
}

/*******************************************************************************
* Piotr's Computer Vision Matlab Toolbox      Version 3.30
* Copyright 2014 Piotr Dollar & Ron Appel.  [pdollar-at-gmail.com]
* Licensed under the Simplified BSD License [see external/bsd.txt]
*******************************************************************************/
// #include "wrappers.hpp"

#define PI 3.14159265f

// compute x and y gradients for just one column (uses sse)
void grad1( float *I, float *Gx, float *Gy, int h, int w, int x ) {
  int y, y1; float *Ip, *In, r; __m128 *_Ip, *_In, *_G, _r;
  // compute column of Gx
  Ip=I-h; In=I+h; r=.5f;
  if(x==0) { r=1; Ip+=h; } else if(x==w-1) { r=1; In-=h; }
  if( h<4 || h%4>0 || (size_t(I)&15) || (size_t(Gx)&15) ) {
    for( y=0; y<h; y++ ) *Gx++=(*In++-*Ip++)*r;
  } else {
    _G=(__m128*) Gx; _Ip=(__m128*) Ip; _In=(__m128*) In; _r = sse::SET(r);
    for(y=0; y<h; y+=4) *_G++=sse::MUL(sse::SUB(*_In++,*_Ip++),_r);
  }
  // compute column of Gy
  #define GRADY(r) *Gy++=(*In++-*Ip++)*r;
  Ip=I; In=Ip+1;
  // GRADY(1); Ip--; for(y=1; y<h-1; y++) GRADY(.5f); In--; GRADY(1);
  y1=((~((size_t) Gy) + 1) & 15)/4; if(y1==0) y1=4; if(y1>h-1) y1=h-1;
  GRADY(1); Ip--; for(y=1; y<y1; y++) GRADY(.5f);
  _r = sse::SET(.5f); _G=(__m128*) Gy;
  for(; y+4<h-1; y+=4, Ip+=4, In+=4, Gy+=4)
    *_G++=sse::MUL(sse::SUB(sse::LDu(*In),sse::LDu(*Ip)),_r);
  for(; y<h-1; y++) GRADY(.5f); In--; GRADY(1);
  #undef GRADY
}

// compute x and y gradients at each location (uses sse)
void grad2( float *I, float *Gx, float *Gy, int h, int w, int d ) {
  int o, x, c, a=w*h; for(c=0; c<d; c++) for(x=0; x<w; x++) {
    o=c*a+x*h; grad1( I+o, Gx+o, Gy+o, h, w, x );
  }
}

// build lookup table a[] s.t. a[x*n]~=acos(x) for x in [-1,1]
float* acosTable() {
  const int n=10000, b=10; int i;
  static float a[n*2+b*2]; static bool init=false;
  float *a1=a+n+b; if( init ) return a1;
  for( i=-n-b; i<-n; i++ )   a1[i]=PI;
  for( i=-n; i<n; i++ )      a1[i]=float(acos(i/float(n)));
  for( i=n; i<n+b; i++ )     a1[i]=0;
  for( i=-n-b; i<n/10; i++ ) if( a1[i] > PI-1e-6f ) a1[i]=PI-1e-6f;
  init=true; return a1;
}

// compute gradient magnitude and orientation at each location (uses sse)
void gradMag( float *I, float *M, float *O, int h, int w, int d, bool full ) {
  int x, y, y1, c, h4, s; float *Gx, *Gy, *M2; __m128 *_Gx, *_Gy, *_M2, _m;
  float *acost = acosTable(), acMult=10000.0f;
  // allocate memory for storing one column of output (padded so h4%4==0)
  h4=(h%4==0) ? h : h-(h%4)+4; s=d*h4*sizeof(float);
  M2=(float*) alMalloc(s,16); _M2=(__m128*) M2;
  Gx=(float*) alMalloc(s,16); _Gx=(__m128*) Gx;
  Gy=(float*) alMalloc(s,16); _Gy=(__m128*) Gy;
  // compute gradient magnitude and orientation for each column
  for( x=0; x<w; x++ ) {
    // compute gradients (Gx, Gy) with maximum squared magnitude (M2)
    for(c=0; c<d; c++) {
      grad1( I+x*h+c*w*h, Gx+c*h4, Gy+c*h4, h, w, x );
      for( y=0; y<h4/4; y++ ) {
        y1=h4/4*c+y;
        _M2[y1]=sse::ADD(sse::MUL(_Gx[y1],_Gx[y1]),sse::MUL(_Gy[y1],_Gy[y1]));
        if( c==0 ) continue; _m = sse::CMPGT( _M2[y1], _M2[y] );
        _M2[y] = sse::OR( sse::AND(_m,_M2[y1]), sse::ANDNOT(_m,_M2[y]) );
        _Gx[y] = sse::OR( sse::AND(_m,_Gx[y1]), sse::ANDNOT(_m,_Gx[y]) );
        _Gy[y] = sse::OR( sse::AND(_m,_Gy[y1]), sse::ANDNOT(_m,_Gy[y]) );
      }
    }
    // compute gradient mangitude (M) and normalize Gx
    for( y=0; y<h4/4; y++ ) {
      _m = sse::MIN( sse::RCPSQRT(_M2[y]), sse::SET(1e10f) );
      _M2[y] = sse::RCP(_m);
      if(O) _Gx[y] = sse::MUL( sse::MUL(_Gx[y],_m), sse::SET(acMult) );
      if(O) _Gx[y] = sse::XOR( _Gx[y], sse::AND(_Gy[y], sse::SET(-0.f)) );
    };
    memcpy( M+x*h, M2, h*sizeof(float) );
    // compute and store gradient orientation (O) via table lookup
    if( O!=0 ) for( y=0; y<h; y++ ) O[x*h+y] = acost[(int)Gx[y]];
    if( O!=0 && full ) {
      y1=((~size_t(O+x*h)+1)&15)/4; y=0;
      for( ; y<y1; y++ ) O[y+x*h]+=(Gy[y]<0)*PI;
      for( ; y<h-4; y+=4 ) sse::STRu( O[y+x*h],
        sse::ADD( sse::LDu(O[y+x*h]), sse::AND(sse::CMPLT(sse::LDu(Gy[y]),sse::SET(0.f)),sse::SET(PI)) ) );
      for( ; y<h; y++ ) O[y+x*h]+=(Gy[y]<0)*PI;
    }
  }
  alFree(Gx); alFree(Gy); alFree(M2);
}

// normalize gradient magnitude at each location (uses sse)
void gradMagNorm( float *M, float *S, int h, int w, float norm ) {
  __m128 *_M, *_S, _norm; int i=0, n=h*w, n4=n/4;
  _S = (__m128*) S; _M = (__m128*) M; _norm = sse::SET(norm);
  bool sse = !(size_t(M)&15) && !(size_t(S)&15);
  if(sse) for(; i<n4; i++) { *_M=sse::MUL(*_M,sse::RCP(sse::ADD(*_S++,_norm))); _M++; }
  if(sse) i*=4; for(; i<n; i++) M[i] /= (S[i] + norm);
}

// helper for gradHist, quantize O and M into O0, O1 and M0, M1 (uses sse)
void gradQuantize( float *O, float *M, int *O0, int *O1, float *M0, float *M1,
  int nb, int n, float norm, int nOrients, bool full, bool interpolate )
{
  // assumes all *OUTPUT* matrices are 4-byte aligned
  int i, o0, o1; float o, od, m;
  __m128i _o0, _o1, *_O0, *_O1; __m128 _o, _od, _m, *_M0, *_M1;
  // define useful constants
  const float oMult=(float)nOrients/(full?2*PI:PI); const int oMax=nOrients*nb;
  const __m128 _norm=sse::SET(norm), _oMult=sse::SET(oMult), _nbf=sse::SET((float)nb);
  const __m128i _oMax=sse::SET(oMax), _nb=sse::SET(nb);
  // perform the majority of the work with sse
  _O0=(__m128i*) O0; _O1=(__m128i*) O1; _M0=(__m128*) M0; _M1=(__m128*) M1;
  if( interpolate ) for( i=0; i<=n-4; i+=4 ) {
    _o=sse::MUL(sse::LDu(O[i]),_oMult); _o0=sse::CVT(_o); _od=sse::SUB(_o,sse::CVT(_o0));
    _o0=sse::CVT(sse::MUL(sse::CVT(_o0),_nbf)); _o0=sse::AND(sse::CMPGT(_oMax,_o0),_o0); *_O0++=_o0;
    _o1=sse::ADD(_o0,_nb); _o1=sse::AND(sse::CMPGT(_oMax,_o1),_o1); *_O1++=_o1;
    _m=sse::MUL(sse::LDu(M[i]),_norm); *_M1=sse::MUL(_od,_m); *_M0++=sse::SUB(_m,*_M1); _M1++;
  } else for( i=0; i<=n-4; i+=4 ) {
    _o=sse::MUL(sse::LDu(O[i]),_oMult); _o0=sse::CVT(sse::ADD(_o,sse::SET(.5f)));
    _o0=sse::CVT(sse::MUL(sse::CVT(_o0),_nbf)); _o0=sse::AND(sse::CMPGT(_oMax,_o0),_o0); *_O0++=_o0;
    *_M0++=sse::MUL(sse::LDu(M[i]),_norm); *_M1++=sse::SET(0.f); *_O1++=sse::SET(0);