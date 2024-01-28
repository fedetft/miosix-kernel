#include <cstdio>
#include "miosix.h"
#include "kernel/logging.h"
#include "interfaces/arch_registers.h"

using namespace std;
using namespace miosix;

// This small program is useful for forcing a serial device to drop bytes.
// Due to Miosix's internal echoing, it echoes everything twice.

char buf[1024];

int main()
{
    for (;;)
    {
        int n = read(0, buf, 1024);
        iprintf("\033[31m");
        fwrite(buf, 1, n, stdout);
        iprintf("\033[0m\n");
    }
}
