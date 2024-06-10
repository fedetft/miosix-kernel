
#include <cstdio>
#include <cstdlib>

struct TestClass
{
    TestClass()
    {
        iprintf("ctor\n");
    }

    void method()
    {
        if(member!=123) iprintf("member_bad\n");
        else iprintf("member_ok\n");
    }

    ~TestClass()
    {
        iprintf("dtor\n");
    }

    int member=123;
};

TestClass testObject;

int main(int argc, char *argv[])
{
    testObject.method();
    return 0;
}
