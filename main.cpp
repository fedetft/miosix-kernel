
#include <cstdio>
#include <cmath>
#include <cassert>
#include <pthread.h>
#include <utility>
#include "miosix.h"

using namespace std;
using namespace miosix;

volatile float f1=3.0f; //Volatile to prevent compiler optimization
volatile float f2=2.0f; //from moving the computation out of the loop

static float approxSqrt1()
{
    float result=f1;
    for(int j=0;j<10;j++)
    {
        for(int i=0;i<1000000/10;i++) result=(result+f1/result)/2.0f;
        delayMs(2); //To test code that first uses fp. then stops and restarts
    }
    return result;
}

static float approxSqrt2()
{
    float result=f2;
    for(int i=0;i<1000000;i++) result=(result+f2/result)/2.0f;
    return result;
}

void *thread(void*)
{
    for(;;)
    {
        volatile float value=approxSqrt1();
        //assert(fabsf(value-sqrt(3.0f))<0.000001f);
        printf("b: %12.10f\n",value);
    }
}

int main()
{
    pthread_t t;
    pthread_create(&t,0,thread,0);
    for(;;)
    {
        volatile float value=approxSqrt2();
        //assert(fabsf(value-sqrt(2.0f))<0.000001f);
        printf("a: %12.10f\n",value);
    }
}

//#define sarcazzo
//
//int exchange_and_add(volatile int* __mem, int __val)
//  {
//    int __result;
//
//    #ifdef sarcazzo
//    int __ok;
//    do {
//      asm volatile("ldrex %0, [%1]"     : "=r"(__result) : "r"(__mem)             : "memory");
//      int __tmp = __result + __val;
//      asm volatile("strex %0, %1, [%2]" : "=r"(__ok)     : "r"(__tmp), "r"(__mem) : "memory");
//    } while(__ok);
//    #else
//    __result = *__mem;
//    *__mem += __val;
//    #endif
//
//    return __result;
//  }
//
//int atomic_add(volatile int* __mem, int __val)
//{
//	int result;
//    #ifdef sarcazzo
//    int __ok;
//    do {
//      int __tmp;
//      asm volatile("ldrex %0, [%1]"     : "=r"(__tmp) : "r"(__mem)             : "memory");
//      __tmp += __val;
//      asm volatile("strex %0, %1, [%2]" : "=r"(__ok)  : "r"(__tmp), "r"(__mem) : "memory");
//	  result++;
//    } while(__ok);
//    #else //sarcazzo
//    int __tmp=*__mem;
//	__tmp += __val;
//	*__mem=__tmp;
//	result=1;
//    #endif //sarcazzo
//	return result;
//}
//
//int mazz;
//int k;
//int w;
//
//void *thread(void*)
//{
//	mazz=0;
//	for(int i=0;i<1000;i++)
//	{
//		mazz=max(mazz,atomic_add(&k, 1));
//		mazz=max(mazz,atomic_add(&k,-1));
//		exchange_and_add(&w, 1);
//		exchange_and_add(&w,-1);
//	}
//	return 0;
//}
//
//int main()
//{
//	getchar();
//	for(;;)
//	{
//		k=0;
//		w=0;
//		pthread_t t;
//		pthread_create(&t,0,thread,0);
//		int maz=0;
//		for(int i=0;i<1000;i++)
//		{
//			maz=max(maz,atomic_add(&k, 1));
//			maz=max(maz,atomic_add(&k,-1));
//			exchange_and_add(&w, 1);
//			exchange_and_add(&w,-1);
//		}
//		pthread_join(t,0);
//		iprintf("Main: k=%d, w=%d, max1=%d max2=%d\n",k,w,maz,mazz);
//	}
//}
