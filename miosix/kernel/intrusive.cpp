
#ifndef TEST_ALGORITHM

#include "intrusive.h"

#else //TEST_ALGORITHM

#include <iostream>
#include <cassert>

// Unused stubs as the test code only tests IntrusiveList
inline int atomicSwap(volatile int*, int) { return 0; }
void *atomicFetchAndIncrement(void *const volatile*, int, int) { return nullptr; }

//C++ glassbox testing trick
#define private public
#define protected public
#include "intrusive.h"
#undef private
#undef public

using namespace std;
using namespace miosix;

#endif //TEST_ALGORITHM





//Testsuite for IntrusiveList. Compile with:
//g++ -DTEST_ALGORITHM -DINTRUSIVE_LIST_ERROR_CHECK -fsanitize=address -m32
//    -std=c++14 -Wall -O2 -o test intrusive.cpp; ./test
#ifdef TEST_ALGORITHM

void emptyCheck(IntrusiveListItem& x)
{
    //Glass box check
    assert(x.next==nullptr); assert(x.prev==nullptr);
}

void emptyCheck(IntrusiveList<IntrusiveListItem>& list)
{
    //Black box check
    assert(list.empty());
    assert(list.begin()==list.end());
    //Glass box check
    assert(list.head==nullptr);
    assert(list.tail==nullptr);
}

void oneItemCheck(IntrusiveList<IntrusiveListItem>& list, IntrusiveListItem& a)
{
    IntrusiveList<IntrusiveListItem>::iterator it;
    //Black box check
    assert(list.empty()==false);
    assert(list.front()==&a);
    assert(list.back()==&a);
    assert(list.begin()!=list.end());
    assert(*list.begin()==&a);
    assert(++list.begin()==list.end());
    it=list.begin(); it++; assert(it==list.end());
    assert(--list.end()==list.begin());
    it=list.end(); it--; assert(it==list.begin());
    //Glass box check
    assert(list.head==&a);
    assert(list.tail==&a);
    assert(a.prev==nullptr);
    assert(a.next==nullptr);
}

void twoItemCheck(IntrusiveList<IntrusiveListItem>& list, IntrusiveListItem& a,
                  IntrusiveListItem& b)
{
    IntrusiveList<IntrusiveListItem>::iterator it;
    //Black box check
    assert(list.empty()==false);
    assert(list.front()==&a);
    assert(list.back()==&b);
    assert(list.begin()!=list.end());
    it=list.begin();
    assert(*it++==&a);
    assert(*it++==&b);
    assert(it==list.end());
    it=list.begin();
    assert(*it==&a);
    ++it;
    assert(*it==&b);
    ++it;
    assert(it==list.end());
    it=list.end();
    it--;
    assert(*it==&b);
    it--;
    assert(*it==&a);
    assert(it==list.begin());
    it=list.end();
    assert(*--it==&b);
    assert(*--it==&a);
    assert(it==list.begin());
    //Glass box check
    assert(list.head==&a);
    assert(list.tail==&b);
    assert(a.prev==nullptr);
    assert(a.next==&b);
    assert(b.prev==&a);
    assert(b.next==nullptr);
}

void threeItemCheck(IntrusiveList<IntrusiveListItem>& list, IntrusiveListItem& a,
                  IntrusiveListItem& b, IntrusiveListItem& c)
{
    IntrusiveList<IntrusiveListItem>::iterator it;
    //Black box check
    assert(list.empty()==false);
    assert(list.front()==&a);
    assert(list.back()==&c);
    assert(list.begin()!=list.end());
    it=list.begin();
    assert(*it++==&a);
    assert(*it++==&b);
    assert(*it++==&c);
    assert(it==list.end());
    it=list.begin();
    assert(*it==&a);
    ++it;
    assert(*it==&b);
    ++it;
    assert(*it==&c);
    ++it;
    assert(it==list.end());
    it=list.end();
    it--;
    assert(*it==&c);
    it--;
    assert(*it==&b);
    it--;
    assert(*it==&a);
    assert(it==list.begin());
    it=list.end();
    assert(*--it==&c);
    assert(*--it==&b);
    assert(*--it==&a);
    assert(it==list.begin());
    //Glass box check
    assert(list.head==&a);
    assert(list.tail==&c);
    assert(a.prev==nullptr);
    assert(a.next==&b);
    assert(b.prev==&a);
    assert(b.next==&c);
    assert(c.prev==&b);
    assert(c.next==nullptr);
}

int main()
{
    IntrusiveListItem a,b,c;
    IntrusiveList<IntrusiveListItem> list;
    emptyCheck(a);
    emptyCheck(b);
    emptyCheck(c);
    emptyCheck(list);

    //
    // Testing push_back / pop_back
    //
    list.push_back(&a);
    oneItemCheck(list,a);
    list.push_back(&b);
    twoItemCheck(list,a,b);
    list.pop_back();
    oneItemCheck(list,a);
    emptyCheck(b);
    list.pop_back();
    emptyCheck(list);
    emptyCheck(a);

    //
    // Testing push_front / pop_front
    //
    list.push_front(&a);
    oneItemCheck(list,a);
    list.push_front(&b);
    twoItemCheck(list,b,a);
    list.pop_front();
    oneItemCheck(list,a);
    emptyCheck(b);
    list.pop_front();
    emptyCheck(list);
    emptyCheck(a);

    //
    // Testing insert / erase
    //
    list.insert(list.end(),&a);
    oneItemCheck(list,a);
    list.insert(list.end(),&b);
    twoItemCheck(list,a,b);
    list.erase(++list.begin()); //Erase second item first
    oneItemCheck(list,a);
    emptyCheck(b);
    list.erase(list.begin());   //Erase only item
    emptyCheck(list);
    emptyCheck(a);
    list.insert(list.begin(),&a);
    oneItemCheck(list,a);
    list.insert(list.begin(),&b);
    twoItemCheck(list,b,a);
    list.erase(list.begin());   //Erase first item first
    oneItemCheck(list,a);
    emptyCheck(b);
    list.erase(list.begin());   //Erase only item
    emptyCheck(list);
    emptyCheck(a);
    list.insert(list.end(),&a);
    oneItemCheck(list,a);
    list.insert(list.end(),&c);
    twoItemCheck(list,a,c);
    list.insert(++list.begin(),&b); //Insert in the middle
    threeItemCheck(list,a,b,c);
    list.erase(++list.begin());     //Erase in the middle
    twoItemCheck(list,a,c);
    emptyCheck(b);
    list.erase(list.begin());
    oneItemCheck(list,c);
    emptyCheck(a);
    list.erase(list.begin());
    emptyCheck(list);
    emptyCheck(c);

    cout<<"Test passed"<<endl;
    return 0;
}

#endif //TEST_ALGORITHM
