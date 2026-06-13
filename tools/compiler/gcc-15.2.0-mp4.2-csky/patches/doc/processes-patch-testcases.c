
extern const int aRodata;
const int aRodata2=42;
extern int aData;
int aData=0;
const char str[]="Hello world\n";
//extern const char *str;

int get1() { return aRodata; }
int get2() { return aData; }
const char *get3() { return str; }
int get4() { return aRodata2; }
const int *get5() { return &aRodata2; }

// If this produces a GOTOFF relocation, it's broken
int *ptr = { 0 };
int *get6()
{
    return ptr;
}

void f();
typedef void (*fp)();
fp g() { return &f; }

// Like extern struct _reent *const _global_impure_ptr __ATTRIBUTE_IMPURE_PTR__;
extern int * const cptr;

const int *get7()
{
    return cptr;
}

// ptr1 ends in .data.rel, as its value needs to be relocated  while ptr2 ends up in .rodata.
int * const ptr1=0;
int * const ptr2=&aData;
 
const int * gg() { return ptr2; } //Interesting, due to optimization this references directly aData

// What if it's not a pointer, like extern int * const cptr; but a struct, or an array?
// If that struct contains pointers, then the entire struct gets promoted out of .rodata
struct Q { int *p; int i; };
const struct Q q1 = { 0,0 };
const struct Q q2 = { &aData,0 };




