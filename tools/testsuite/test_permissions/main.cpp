/***************************************************************************
 *   Copyright (C) 2024 by Daniele Cattaneo                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <spawn.h>

using namespace std;

extern char **environ;

// Processes implies filesystem & DevFs
#define WITH_FILESYSTEM
#define WITH_DEVFS
#define WITH_PROCESSES
#define WITH_POSIX_PERMISSIONS

constexpr const char* AS_ROOT="root";
constexpr const char* SAME_UID_AND_GID_TEST="same-uid-gid";
constexpr const char* SAME_GID_ONLY_TEST="same-gid-only";
constexpr const char* OTHER_TEST="other";

static int doConcreteTest()
{
    char* argv[] = {(char* const)"/bin/test_permissions", NULL};
    return execv(argv[0], argv);
}

static int doNoExecTest()
{
    if(access("/bin/non-executable.txt", F_OK)!=0) return -1;

    // access must check that we don't have execute permissions on the file
    // since the file is not an executable
    if(access("/bin/non-executable.txt", X_OK)!=-1) return -1;

    char* argv[] = {(char* const)"/bin/test_permissions", NULL};
    auto result = execv(argv[0], argv);
    if (result==-1 && errno==EACCES) return 0;
    return -1; 
}

int main(int argc, char *argv[], char *envp[])
{
    if(argc == 0) return 1;
    else if(argc == 1) return 0; // we just want to check that we can run it so we can return immediately
    else if(argc == 3)
    {
        if(strcmp(AS_ROOT, argv[1])==0) 
        {
            if(strcmp("test-noexec", argv[2])) return doNoExecTest();
            return doConcreteTest();
        }    
        if(strcmp(SAME_UID_AND_GID_TEST, argv[1])==0) 
        {
            setgid(1000);
            setuid(1000);
            // we shouldn't be able to run the process since it is not an executable even if we have the
            // same uid and gid
            if(strcmp("test-noexec", argv[2])) return doNoExecTest();
            // we should be able to run the process as is since user 1000 and group 1000 have 
            // both execute permissions
            return doConcreteTest();
        }
        if(strcmp(SAME_GID_ONLY_TEST, argv[1])==0)
        {
            setgid(1000);
            setuid(2000);
            if(strcmp("test-noexec", argv[2])) return doNoExecTest();
            return doConcreteTest();
        }
        if(strcmp(OTHER_TEST, argv[1])==0)
        {
            setgid(2000);
            setuid(2000);
            if(strcmp("test-noexec", argv[2])) return doNoExecTest();
            bool result=doConcreteTest()==-1; // here we expect it to fail
            if (result && errno==EACCES) return 0;
            return -1;
        }
    }
    else return 1;
    return 0;
}