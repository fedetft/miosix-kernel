
#include <cstdio>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "miosix.h"

using namespace std;
using namespace miosix;

mutex m;
condition_variable cv;

void __attribute__((noinline)) f1()
{
    auto t=chrono::steady_clock::now();
    iprintf("%lld\n",t.time_since_epoch().count());
}

void __attribute__((noinline)) f2()
{
    auto t=chrono::steady_clock::now();
    t+=100ms;
    this_thread::sleep_until(t);
}

void __attribute__((noinline)) f3()
{
    unique_lock<mutex> l(m);
    if(cv.wait_for(l,100ms)!=cv_status::timeout) iprintf("Error: no timeout\n");
}

void __attribute__((noinline)) f4()
{
    unique_lock<mutex> l(m);
    auto t=chrono::steady_clock::now();
    if(cv.wait_until(l,t+100ms)!=cv_status::timeout) iprintf("Error: no timeout\n");
}

int main()
{
    while(getchar())
    {
        f1();
        f2();
        f3();
        f4();
    }
}
