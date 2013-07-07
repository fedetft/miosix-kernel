
#include <cstdio>
#include "miosix.h"
#include "filesystem/devfs/devfs.h"

using namespace std;
using namespace miosix;

int main()
{
    //iprintf("Hello world, write your application here\n");
    DevFs *dfs=new DevFs;
    FilesystemManager& fsm=FilesystemManager::instance();
    fsm.kmount("/dev",dfs);
    FileDescriptorTable fdt;
    
}
