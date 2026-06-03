/***************************************************************************
 *   Copyright (C) 2026 by Lorenzo Pigato                                  *
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

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "filesystem/automounter/sd_automounter.h"
#include "util/util.h"

using namespace miosix;

static char cwd[128] = "/";
static char line[256];
static char arg[256];

static void cmd_cd(const char *path)
{
    if(chdir(path) != 0) iprintf("cd: %s\n", strerror(errno));
    getcwd(cwd, sizeof(cwd));
}

static void cmd_ls(const char *path)
{
    const char *target = path[0] ? path : ".";
    DIR *d = opendir(target);
    if(!d)
    {
        iprintf("ls: %s\n", strerror(errno));
        return;
    }

    dirent *e;
    while((e = readdir(d)) != nullptr)
    {
        struct stat st;
        char full[1024];
        snprintf(full, sizeof(full), "%s/%s", target, e->d_name);
        bool isDir = stat(full, &st) == 0 && S_ISDIR(st.st_mode);
        iprintf("  %s%s\n", e->d_name, isDir ? "/" : "");
    }
    closedir(d);
}

static void cmd_stat(const char *path)
{
    struct stat st;
    if(stat(path, &st) != 0)
    {
        iprintf("stat: %s\n", strerror(errno));
        return;
    }
    iprintf("  dev=%d size=%d mode=0%o\n",
            static_cast<int>(st.st_dev),
            static_cast<int>(st.st_size),
            static_cast<unsigned>(st.st_mode));
}

static void cmd_cat(const char *path)
{
    FILE *f = fopen(path, "r");
    if(!f)
    {
        iprintf("cat: %s\n", strerror(errno));
        return;
    }
    char buf[128];
    while(fgets(buf, sizeof(buf), f)) iprintf("%s", buf);
    fclose(f);
    iprintf("\n");
}

static void cmd_echo(const char *args)
{
    const char *sep = strstr(args, ">>");
    bool append = true;
    if(!sep)
    {
        sep = strstr(args, ">");
        append = false;
    }

    if(!sep)
    {
        iprintf("%s\n", args);
        return;
    }

    char text[256];
    size_t textLen = static_cast<size_t>(sep - args);
    while(textLen > 0 && args[textLen - 1] == ' ') textLen--;
    if(textLen >= sizeof(text)) textLen = sizeof(text) - 1;
    memcpy(text, args, textLen);
    text[textLen] = '\0';

    const char *path = sep + (append ? 2 : 1);
    while(*path == ' ') path++;
    if(*path == '\0')
    {
        iprintf("echo: missing filename\n");
        return;
    }

    FILE *f;
    if(append)
    {
        f = fopen(path, "r+");
        if(f) fseek(f, 0, SEEK_END);
        else f = fopen(path, "w");
    } else {
        f = fopen(path, "w");
    }
    if(!f)
    {
        iprintf("echo: %s\n", strerror(errno));
        return;
    }
    fprintf(f, "%s\n", text);
    fclose(f);
}

static void cmd_touch(const char *path)
{
    if(*path == '\0')
    {
        iprintf("touch: missing filename\n");
        return;
    }

    struct stat st;
    if(stat(path, &st) == 0) return;
    FILE *f = fopen(path, "w");
    if(!f)
    {
        iprintf("touch: %s\n", strerror(errno));
        return;
    }
    fclose(f);
}

static void cmd_rm(const char *path)
{
    if(*path == '\0')
    {
        iprintf("rm: missing filename\n");
        return;
    }
    if(remove(path) != 0) iprintf("rm: %s\n", strerror(errno));
}

static void cmd_mkdir(const char *path)
{
    if(*path == '\0')
    {
        iprintf("mkdir: missing dirname\n");
        return;
    }
    if(mkdir(path, 0755) != 0) iprintf("mkdir: %s\n", strerror(errno));
}

static void cmd_pwd()
{
    iprintf("%s\n", cwd);
}

static void cmd_enable()
{
    SdAutomounter::instance().enable();
    iprintf("automounter enabled\n");
}

static void cmd_disable()
{
    SdAutomounter::instance().disable();
    iprintf("automounter disabled\n");
}

static void cmd_fetch()
{
    // Logo fits best on terminals >= 96 cols
    iprintf("\n");
    iprintf("0000000000000000  0000           0000     000                                 000\n");
    iprintf("0            000  00000         00000     000                                 000\n");
    iprintf("0           0000  000000        00000\n");
    iprintf("0          00000  000000       000000     000      00000000      000000000    000   000     000\n");
    iprintf("0         000000  000 000     000 000     000     000    000    000           000    000   000\n");
    iprintf("0        0000000  000  000    00  000     000    000      000   0000          000     000 000\n");
    iprintf("0       00000000  000  000   000  000     000   000        000    00000       000       0000\n");
    iprintf("0      000000000  000   000 000   000     000   000        000       00000    000      00000\n");
    iprintf("0     0000000000  000    00000    000     000    000      000          000    000     000 000\n");
    iprintf("0    00000000000  000    00000    000     000     000    000    00     000    000    000   000\n");
    iprintf("0000000000000000  000     000     000     000      00000000     000000000     000   000     000\n");
    iprintf("\n");
    iprintf("------------------------------------------------------------------------\n");

    // Board & CPU
    iprintf("  Board   : %s\n", _MIOSIX_BOARDNAME);
    iprintf("  CPU     : %u MHz", cpuFrequency / 1000000u);
    if(oscillatorType == OscillatorType::HSE)
        iprintf("  (HSE %u MHz)\n", hseFrequency / 1000000u);
    else
        iprintf("  (HSI)\n");

    // Serial
    iprintf("  Serial  : USART%u @ %u baud\n", defaultSerial, defaultSerialSpeed);

    // RAM
    unsigned int heapFree  = MemoryProfiling::getCurrentFreeHeap();
    unsigned int heapTotal = MemoryProfiling::getHeapSize();
    iprintf("  Heap    : %u / %u B free\n", heapFree, heapTotal);

    // Uptime
    long long ns = getTime();
    unsigned int secs = static_cast<unsigned int>(ns / 1000000000LL);
    iprintf("  Uptime  : %uh %02um %02us\n", secs / 3600, (secs % 3600) / 60, secs % 60);

    // Automounter + SD mount status
    bool enabled = SdAutomounter::instance().isEnabled();
    iprintf("  Automnt : %s\n", enabled ? "enabled" : "disabled");

    struct stat rootSt, sdSt;
    bool sdMounted = stat("/", &rootSt) == 0 && stat("/sd", &sdSt) == 0
                     && rootSt.st_dev != sdSt.st_dev;
    iprintf("  SD      : %s\n", sdMounted ? "mounted" : "not mounted");

    iprintf("------------------------------------------------------------------------\n");
}

static void cmd_help()
{
    iprintf("  cd    <path>  change directory\n");
    iprintf("  ls    [path]  list directory\n");
    iprintf("  pwd           print working directory\n");
    iprintf("  stat  <path>  file info\n");
    iprintf("  cat   <path>  print file\n");
    iprintf("  echo  <text> [> file]  write text (>> appends)\n");
    iprintf("  touch <path>  create empty file\n");
    iprintf("  rm    <path>  remove file\n");
    iprintf("  mkdir <path>  create directory\n");
    iprintf("  fetch         system info\n");
    iprintf("  enable        enable SD automounter\n");
    iprintf("  disable       disable SD automounter\n");
    iprintf("  help          this message\n");
}

int main()
{
    getcwd(cwd, sizeof(cwd));
    for(;;)
    {
        iprintf("%s >> ", cwd);

        if(!fgets(line, sizeof(line), stdin)) continue;

        char *nl = strchr(line, '\n');
        if(nl) *nl = '\0';
        if(line[0] == '\0') continue;

        arg[0] = '\0';
        char *sp = strchr(line, ' ');
        if(sp)
        {
            *sp = '\0';
            strncpy(arg, sp + 1, sizeof(arg) - 1);
            arg[sizeof(arg) - 1] = '\0';
        }

        if     (strcmp(line, "cd")      == 0) cmd_cd(arg[0] ? arg : "/");
        else if(strcmp(line, "ls")      == 0) cmd_ls(arg);
        else if(strcmp(line, "pwd")     == 0) cmd_pwd();
        else if(strcmp(line, "stat")    == 0) cmd_stat(arg);
        else if(strcmp(line, "cat")     == 0) cmd_cat(arg);
        else if(strcmp(line, "echo")    == 0) cmd_echo(arg);
        else if(strcmp(line, "touch")   == 0) cmd_touch(arg);
        else if(strcmp(line, "rm")      == 0) cmd_rm(arg);
        else if(strcmp(line, "mkdir")   == 0) cmd_mkdir(arg);
        else if(strcmp(line, "fetch")   == 0) cmd_fetch();
        else if(strcmp(line, "enable")  == 0) cmd_enable();
        else if(strcmp(line, "disable") == 0) cmd_disable();
        else if(strcmp(line, "help")    == 0) cmd_help();
        else iprintf("unknown command - try 'help'\n");
    }
}
