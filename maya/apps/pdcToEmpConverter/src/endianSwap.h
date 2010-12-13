#ifndef _endianSwap
#define _endianSwap

#include <assert.h>

template<class T> inline void swap_endianity(T &x)
{
   assert(sizeof(T)<=8); // should not be called on composite types: instead specialize swap_endianity if needed.
   T old=x;
   for(unsigned int k=0; k<sizeof(T); ++k)
      ((char*)&x)[k] = ((char*)&old)[sizeof(T)-1-k];
}

#endif
