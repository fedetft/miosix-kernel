/***************************************************************************
 *   Copyright (C) 2012-2024 by Terraneo Federico                          *
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

#include <stdexcept>
#include <memory>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>

#include "sync.h"
#include "process_pool.h"
#include "process.h"

using namespace std;

#ifdef WITH_PROCESSES

#ifdef __NO_EXCEPTIONS
#error Processes require C++ exception support
#endif //__NO_EXCEPTIONS

#ifndef WITH_FILESYSTEM
#error Processes require filesystem support
#endif //WITH_FILESYSTEM

#ifndef WITH_DEVFS
#error Processes require devfs support
#endif //WITH_DEVFS

namespace miosix {

/**
 * Used to check if a pointer passed from userspace is aligned
 */
static bool aligned(void *x) { return (reinterpret_cast<unsigned>(x) & 0b11)==0; }

/**
 * Validate that a string array parameter, such as the one passed to the execve
 * syscall belongs to the process memory.
 * \param mpu mpu object knowing the valid memory regions for the current process
 * \param a string array
 * \return the number of strings or -1 on failure
 */
static int validateStringArray(MPUConfiguration& mpu, char* const* a)
{
    for(int i=0;;i++)
    {
        //Is the array element safe to dereference?
        if(mpu.withinForReading(a+i,sizeof(char*))==false) return -1;
        //Is the char * contained in the array element the end of the array?
        if(a[i]==nullptr) return i;
        //Is the char * contained in the array a safe to dereference string?
        if(mpu.withinForReading(a[i])==false) return -1;
    }
}

/**
 * This class contains information on all the processes in the system
 */
class Processes
{
public:
    /**
     * \return the instance of this class (singleton)
     */
    static Processes& instance();
    
    ///Used to assign a new pid to a process
    pid_t pidCounter;
    ///Maps the pid to the Process instance. Includes zombie processes
    map<pid_t,ProcessBase *> processes;
    ///Uset to guard access to processes and pidCounter
    Mutex procMutex;
    ///Used to wait on process termination
    ConditionVariable genericWaiting;
    
private:
    Processes()
    {
        ProcessBase *kernel=Thread::getCurrentThread()->getProcess();
        assert(kernel->getPid()==0);
        processes[0]=kernel;
    }
    Processes(const Processes&)=delete;
    Processes& operator=(const Processes&)=delete;
};

Processes& Processes::instance()
{
    static Processes singleton;
    return singleton;
}

//
// class Process
//

pid_t Process::create(ElfProgram&& program, ArgsBlock&& args)
{
    if(program.errorCode()) return program.errorCode();
    Processes& p=Processes::instance();
    ProcessBase *parent=Thread::getCurrentThread()->proc;
    unique_ptr<Process> proc(new Process(parent->fileTable,
                                         std::move(program),std::move(args)));
    {   
        Lock<Mutex> l(p.procMutex);
        proc->pid=getNewPid();
        proc->ppid=parent->pid;
        parent->childs.push_back(proc.get());
        p.processes[proc->pid]=proc.get();
    }
    auto thr=Thread::createUserspace(Process::start,proc.get());
    if(thr==nullptr)
    {
        Lock<Mutex> l(p.procMutex);
        p.processes.erase(proc->pid);
        parent->childs.remove(proc.get());
        throw runtime_error("Thread creation failed");
    }
    //Cannot throw bad_alloc due to the reserve in Process's constructor.
    //This ensures we will never be in the uncomfortable situation where a
    //thread has already been created but there's no memory to list it
    //among the threads of a process
    proc->threads.push_back(thr);
    thr->wakeup(); //Actually start the thread, now that everything is set up
    pid_t result=proc->pid;
    proc.release(); //Do not delete the pointer
    return result;
}

pid_t Process::spawn(const char *path, const char* const* argv,
        const char* const* envp, int narg, int nenv)
{
    ArgsBlock args(argv,envp,narg,nenv);
    if(args.valid()==false) return -E2BIG;
    ElfProgram program(path);
    return Process::create(std::move(program),std::move(args));
}

pid_t Process::spawn(const char *path, const char* const* argv,
        const char* const* envp)
{
    int narg=0,nenv=0;
    if(argv) while(argv[narg]) narg++;
    if(envp) while(envp[nenv]) nenv++;
    return Process::spawn(path,argv,envp,narg,nenv);
}

pid_t Process::getppid(pid_t proc)
{
    Processes& p=Processes::instance();
    Lock<Mutex> l(p.procMutex);
    map<pid_t,ProcessBase *>::iterator it=p.processes.find(proc);
    if(it==p.processes.end()) return -1;
    return it->second->ppid;
}

pid_t Process::waitpid(pid_t pid, int* exit, int options)
{
    Processes& p=Processes::instance();
    Lock<Mutex> l(p.procMutex);
    ProcessBase *self=Thread::getCurrentThread()->proc;
    if(pid<=0)
    {
        //Wait for a generic child process
        if(self->zombies.empty() && (options & WNOHANG)) return 0;
        while(self->zombies.empty())
        {
            if(self->childs.empty()) return -ECHILD;
            p.genericWaiting.wait(l);
        }
        Process *joined=self->zombies.front();
        self->zombies.pop_front();
        p.processes.erase(joined->pid);
        if(joined->waitCount!=0) errorHandler(UNEXPECTED);
        if(exit!=nullptr) *exit=joined->exitCode;
        pid_t result=joined->pid;
        delete joined;
        return result;
    } else {
        //Wait on a specific child process
        map<pid_t,ProcessBase *>::iterator it=p.processes.find(pid);
        if(it==p.processes.end() || it->second->ppid!=self->pid
                || pid==self->pid) return -ECHILD;
        //Since the case when pid==0 has been singled out, this cast is safe
        Process *joined=static_cast<Process*>(it->second);
        if(joined->zombie==false)
        {
            //Process hasn't terminated yet
            if(options & WNOHANG) return 0;
            joined->waitCount++;
            joined->waiting.wait(l);
            joined->waitCount--;
            if(joined->waitCount<0 || joined->zombie==false)
                errorHandler(UNEXPECTED);
        }
        pid_t result=-1;
        if(joined->waitCount==0)
        {
            result=joined->pid;
            if(exit!=nullptr) *exit=joined->exitCode;
            self->zombies.remove(joined);
            p.processes.erase(joined->pid);
            delete joined;
        }
        return result;
    }
}

Process::~Process() {}

Process::Process(const FileDescriptorTable& fdt, ElfProgram&& program,
        ArgsBlock&& args) : ProcessBase(fdt), waitCount(0), zombie(false)
{
    //This is required so that bad_alloc can never be thrown when the first
    //thread of the process will be stored in this vector
    threads.reserve(1);
    load(std::move(program),std::move(args));
}

void Process::load(ElfProgram&& program, ArgsBlock&& args)
{
    this->program=std::move(program);
    //Done here so if not enough memory the new process is not even created
    image.load(this->program);
    //Do the final size check that could not be done when validating the elf
    //file alone because the size of the args block was unknown.
    //The args block is not considered part of the stack, so it pushes the main
    //stack down and eats away from the heap. This check should only fail if
    //the area reserved for the heap is so small that becomes negative
    if(image.getDataBssSize()+WATERMARK_LEN+image.getMainStackSize()+args.size()
        > image.getProcessImageSize()) throw runtime_error("Args block overflow");
    auto ptr=reinterpret_cast<char*>(image.getProcessBasePointer());
    //iprintf("image    addr = %p size = %d\n",ptr,image.getProcessImageSize());
    //iprintf("data/bss addr = %p size = %d\n",ptr,image.getDataBssSize());
    //iprintf("stack    addr = %p size = %d\n",ptr+image.getProcessImageSize()
    //    -args.size()-image.getMainStackSize()-WATERMARK_LEN,
    //    image.getMainStackSize());
    //iprintf("args     addr = %p size = %d\n",ptr+image.getProcessImageSize()
    //    -args.size(),args.size());
    ptr+=image.getProcessImageSize()-args.size();
    args.relocateTo(ptr);
    argc=args.getNumberOfArguments();
    argvSp=ptr; //Argument array is at the start of the args block
    envp=ptr+args.getEnvIndex();
    auto elfBase=reinterpret_cast<const unsigned int*>(this->program.getElfBase());
    unsigned int elfSize=this->program.getElfSize();
    //XIP filesystems may store elf programs without the required alignment to
    //support MPU operation, thus round up the elf region so it fits the minimum
    //MPU-capable region. This makes it possible for a process in a XIP
    //filesystem to access more than the elf itself, but since the access is
    //read-only, memory protection is preserved. TODO: use ARM MPU sub-region
    //disable feature to further limit region size
    if(this->program.isCopiedInRam()==false)
        tie(elfBase,elfSize)=MPUConfiguration::roundRegionForMPU(elfBase,elfSize);
    mpu=MPUConfiguration(elfBase,elfSize,
            image.getProcessBasePointer(),image.getProcessImageSize());
}

void *Process::start(void *)
{
    //This function is never called with a kernel thread, so the cast is safe
    Process *proc=static_cast<Process*>(Thread::getCurrentThread()->proc);
    bool running=true;
    do {
        unsigned int entry=proc->program.getEntryPoint();
        Thread::setupUserspaceContext(entry,proc->argc,proc->argvSp,proc->envp,
            proc->image.getProcessBasePointer(),proc->image.getMainStackSize());
        SvcResult svcResult=Resume;
        do {
            miosix_private::SyscallParameters sp=Thread::switchToUserspace();

            bool fault=proc->fault.faultHappened();
            //Handle svc only if no fault occurred
            if(fault==false) svcResult=proc->handleSvc(sp);

            if(Thread::testTerminate() || svcResult==Exit) running=false;
            if(fault || svcResult==Segfault)
            {
                running=false;
                proc->exitCode=SIGSEGV; //Segfault
                #ifdef WITH_ERRLOG
                iprintf("Process %d terminated due to a fault\n"
                        "* Code base address was 0x%x\n"
                        "* Data base address was %p\n",proc->pid,
                        proc->program.getElfBase(),
                        proc->image.getProcessBasePointer());
                proc->mpu.dumpConfiguration();
                if(fault) proc->fault.print();
                #endif //WITH_ERRLOG
            }
        } while(running && svcResult!=Execve);
        if(svcResult==Execve) proc->fileTable.cloexec();
    } while(running);
    proc->fileTable.closeAll();
    {
        Processes& p=Processes::instance();
        Lock<Mutex> l(p.procMutex);
        proc->zombie=true;
        list<Process*>::iterator it;
        for(it=proc->childs.begin();it!=proc->childs.end();++it) (*it)->ppid=0;
        for(it=proc->zombies.begin();it!=proc->zombies.end();++it) (*it)->ppid=0;
        ProcessBase *kernel=p.processes[0];
        kernel->childs.splice(kernel->childs.begin(),proc->childs);
        kernel->zombies.splice(kernel->zombies.begin(),proc->zombies);
        
        map<pid_t,ProcessBase *>::iterator it2=p.processes.find(proc->ppid);
        if(it2==p.processes.end()) errorHandler(UNEXPECTED);
        it2->second->childs.remove(proc);
        if(proc->waitCount>0) proc->waiting.broadcast();
        else {
            it2->second->zombies.push_back(proc);
            p.genericWaiting.broadcast();
        }
    }
    return nullptr;
}

Process::SvcResult Process::handleSvc(miosix_private::SyscallParameters sp)
{
    try {
        switch(static_cast<Syscall>(sp.getSyscallId()))
        {
            case Syscall::OPEN:
            {
                auto name=reinterpret_cast<const char*>(sp.getParameter(0));
                int flags=sp.getParameter(1);
                if(mpu.withinForReading(name))
                {
                    int fd=fileTable.open(name,flags,
                        (flags & O_CREAT) ? sp.getParameter(2) : 0);
                    sp.setParameter(0,fd);
                } else sp.setParameter(0,-EFAULT);
                break;
            }

            case Syscall::CLOSE:
            {
                int result=fileTable.close(sp.getParameter(0));
                sp.setParameter(0,result);
                break;
            }

            case Syscall::READ:
            {
                int fd=sp.getParameter(0);
                void *ptr=reinterpret_cast<void*>(sp.getParameter(1));
                size_t size=sp.getParameter(2);
                if(mpu.withinForWriting(ptr,size))
                {
                    ssize_t result=fileTable.read(fd,ptr,size);
                    sp.setParameter(0,result);
                } else sp.setParameter(0,-EFAULT);
                break;
            }

            case Syscall::WRITE:
            {
                int fd=sp.getParameter(0);
                void *ptr=reinterpret_cast<void*>(sp.getParameter(1));
                size_t size=sp.getParameter(2);
                if(mpu.withinForReading(ptr,size))
                {
                    ssize_t result=fileTable.write(fd,ptr,size);
                    sp.setParameter(0,result);
                } else sp.setParameter(0,-EFAULT);
                break;
            }

            case Syscall::LSEEK:
            {
                off_t pos=sp.getParameter(2);
                pos|=static_cast<off_t>(sp.getParameter(1))<<32;
                off_t result=fileTable.lseek(sp.getParameter(0),pos,
                    sp.getParameter(3));
                sp.setParameter(0,result & 0xffffffff);
                sp.setParameter(1,result>>32);
                break;
            }

            case Syscall::STAT:
            {
                auto file=reinterpret_cast<const char*>(sp.getParameter(0));
                auto pstat=reinterpret_cast<struct stat*>(sp.getParameter(1));
                if(mpu.withinForReading(file) &&
                   mpu.withinForWriting(pstat,sizeof(struct stat)) && aligned(pstat))
                {
                    int result=fileTable.stat(file,pstat);
                    sp.setParameter(0,result);
                } else sp.setParameter(0,-EFAULT);
                break;
            }

            case Syscall::LSTAT:
            {
                auto file=reinterpret_cast<const char*>(sp.getParameter(0));
                auto pstat=reinterpret_cast<struct stat*>(sp.getParameter(1));
                if(mpu.withinForReading(file) &&
                   mpu.withinForWriting(pstat,sizeof(struct stat)) && aligned(pstat))
                {
                    int result=fileTable.lstat(file,pstat);
                    sp.setParameter(0,result);
                } else sp.setParameter(0,-EFAULT);
                break;
            }

            case Syscall::FSTAT:
            {
                auto pstat=reinterpret_cast<struct stat*>(sp.getParameter(1));
                if(mpu.withinForWriting(pstat,sizeof(struct stat)) && aligned(pstat))
                {
                    int result=fileTable.fstat(sp.getParameter(0),pstat);
                    sp.setParameter(0,result);
                } else sp.setParameter(0,-EFAULT);
                break;
            }

            case Syscall::FCNTL:
            {
                //NOTE: some fcntl operations have an optional third parameter
                //which can be either missing, an int or a pointer.
                //Currenlty we do not support any with a pointer third arg, but
                //we arr on the side of safety and either pass the int or pass
                //zero. When we'll support those with the pointer, we'll
                //validate it here.
                int result,cmd=sp.getParameter(1);
                switch(cmd)
                {
                    case F_DUPFD: //Third parameter is int, no validation needed
                    case F_SETFD:
                    case F_SETFL:
                        result=fileTable.fcntl(sp.getParameter(0),cmd,
                                               sp.getParameter(2));
                        break;
                    default:
                        result=fileTable.fcntl(sp.getParameter(0),cmd,0);
                }
                sp.setParameter(0,result);
                break;
            }

            case Syscall::IOCTL:
            {
                //TODO: need a way to validate ARG, for now we reject it
                //Not easy to move checks forward down filesystem code as that
                //code can also be called through kercalls from kthreads
                sp.setParameter(0,-EFAULT);
                break;
            }

            case Syscall::ISATTY:
            {
                int result=fileTable.isatty(sp.getParameter(0));
                sp.setParameter(0,result);
                break;
            }

            case Syscall::GETCWD:
            {
                char *buf=reinterpret_cast<char*>(sp.getParameter(0));
                size_t size=sp.getParameter(1);
                if(mpu.withinForWriting(buf,size))
                {
                    int result=fileTable.getcwd(buf,size);
                    // Do not overwrite r0 on purpose to preserve the pointer
                    sp.setParameter(1,result);
                } else sp.setParameter(1,-EFAULT);
                break;
            }

            case Syscall::CHDIR:
            {
                auto str=reinterpret_cast<const char*>(sp.getParameter(0));
                if(mpu.withinForReading(str))
                {
                    int result=fileTable.chdir(str);
                    sp.setParameter(0,result);
                } else sp.setParameter(0,-EFAULT);
                break;
            }

            case Syscall::GETDENTS:
            {
                int fd=sp.getParameter(0);
                void *buf=reinterpret_cast<void*>(sp.getParameter(1));
                size_t size=sp.getParameter(2);
                if(mpu.withinForWriting(buf,size))
                {
                    int result=fileTable.getdents(fd,buf,size);
                    sp.setParameter(0,result);
                } else sp.setParameter(0,-EFAULT);
                break;
            }

            case Syscall::MKDIR:
            {
                auto path=reinterpret_cast<const char*>(sp.getParameter(0));
                if(mpu.withinForReading(path))
                {
                    int result=fileTable.mkdir(path,sp.getParameter(1));
                    sp.setParameter(0,result);
                } else sp.setParameter(0,-EFAULT);
                break;
            }

            case Syscall::RMDIR:
            {
                auto str=reinterpret_cast<const char*>(sp.getParameter(0));
                if(mpu.withinForReading(str))
                {
                    int result=fileTable.rmdir(str);
                    sp.setParameter(0,result);
                } else sp.setParameter(0,-EFAULT);
                break;
            }

            case Syscall::LINK:
            {
                sp.setParameter(0,-EMLINK); //Currently no fs supports hardlinks
                break;
            }

            case Syscall::UNLINK:
            {
                auto file=reinterpret_cast<const char*>(sp.getParameter(0));
                if(mpu.withinForReading(file))
                {
                    int result=fileTable.unlink(file);
                    sp.setParameter(0,result);
                } else sp.setParameter(0,-EFAULT);
                break;
            }

            case Syscall::SYMLINK:
            {
//                 auto tgt=reinterpret_cast<const char*>(sp.getParameter(0));
//                 auto link=reinterpret_cast<const char*>(sp.getParameter(1));
//                 if(mpu.withinForReading(tgt) && mpu.withinForReading(link))
//                 {
//                     int result=fileTable.symlink(tgt,link);
//                     sp.setParameter(0,result);
//                 } else sp.setParameter(0,-EFAULT);
                sp.setParameter(0,-ENOENT); //Currently no writable fs supports symlinks
                break;
            }

            case Syscall::READLINK:
            {
                auto path=reinterpret_cast<const char*>(sp.getParameter(0));
                auto buf=reinterpret_cast<char*>(sp.getParameter(1));
                size_t size=sp.getParameter(2);
                if(mpu.withinForReading(path) &&
                   mpu.withinForWriting(buf,size))
                {
                    int result=fileTable.readlink(path,buf,size);
                    sp.setParameter(0,result);
                } else sp.setParameter(0,-EFAULT);
                break;
            }

            case Syscall::TRUNCATE:
            {
                auto path=reinterpret_cast<const char*>(sp.getParameter(0));
                off_t size=sp.getParameter(2);
                size|=static_cast<off_t>(sp.getParameter(1))<<32;
                if(mpu.withinForReading(path))
                {
                    int result=fileTable.truncate(path,size);
                    sp.setParameter(0,result);
                } else sp.setParameter(0,-EFAULT);
                break;
            }

            case Syscall::FTRUNCATE:
            {
                off_t size=sp.getParameter(2);
                size|=static_cast<off_t>(sp.getParameter(1))<<32;
                int result=fileTable.ftruncate(sp.getParameter(0),size);
                sp.setParameter(0,result);
                break;
            }

            case Syscall::RENAME:
            {
                auto oldp=reinterpret_cast<const char*>(sp.getParameter(0));
                auto newp=reinterpret_cast<const char*>(sp.getParameter(1));
                if(mpu.withinForReading(oldp) && mpu.withinForReading(newp))
                {
                    int result=fileTable.rename(oldp,newp);
                    sp.setParameter(0,result);
                } else sp.setParameter(0,-EFAULT);
                break;
            }

            case Syscall::CHMOD:
            {
                sp.setParameter(0,-EFAULT); //TODO: stub
                break;
            }

            case Syscall::FCHMOD:
            {
                sp.setParameter(0,-EFAULT); //TODO: stub
                break;
            }

            case Syscall::CHOWN:
            {
                sp.setParameter(0,-EFAULT); //TODO: stub
                break;
            }

            case Syscall::FCHOWN:
            {
                sp.setParameter(0,-EFAULT); //TODO: stub
                break;
            }

            case Syscall::LCHOWN:
            {
                sp.setParameter(0,-EFAULT); //TODO: stub
                break;
            }

            case Syscall::DUP:
            {
                int result=fileTable.dup(sp.getParameter(0));
                sp.setParameter(0,result);
                break;
            }

            case Syscall::DUP2:
            {
                int oldfd=sp.getParameter(0);
                int newfd=sp.getParameter(1);
                int result=fileTable.dup2(oldfd,newfd);
                sp.setParameter(0,result);
                break;
            }

            case Syscall::PIPE:
            {
                int fds[2];
                int result=fileTable.pipe(fds);
                sp.setParameter(1,result); //Does not overwrite r0 on purpose
                sp.setParameter(2,fds[0]);
                sp.setParameter(3,fds[1]);
                break;
            }

            case Syscall::ACCESS:
            {
                sp.setParameter(0,-EFAULT); //TODO: stub
                break;
            }

            case Syscall::GETTIME:
            {
                //TODO: sp.getParameter(0) is clockid_t by there's no support yet
                long long t=getTime();
                sp.setParameter(0,t & 0xffffffff);
                sp.setParameter(1,t>>32);
                break;
            }

            case Syscall::SETTIME:
            {
//                 int clockid=sp.getParameter(0);
//                 long long t=sp.getParameter(2);
//                 t|=static_cast<long long>(sp.getParameter(1))<<32;
                sp.setParameter(0,EFAULT); //NOTE: positive error codes
                break;
            }

            case Syscall::NANOSLEEP:
            {
                int clockidAndFlags=sp.getParameter(3);
                long long t=sp.getParameter(0);
                t|=static_cast<long long>(sp.getParameter(1))<<32;
                //TODO support for clockid is not implemented yet
                if((clockidAndFlags & (1<<8))==0) t+=getTime(); //Relative sleep?
                Thread::nanoSleepUntil(t);
                sp.setParameter(0,0);
                break;
            }

            case Syscall::GETRES:
            {
                struct timespec tv;
                sp.setParameter(0,clock_getres(sp.getParameter(0),&tv));
                //tv_sec not returned, clock resolutions >=1 second unsupported
                sp.setParameter(2,tv.tv_nsec);
                break;
            }

            case Syscall::ADJTIME:
            {
                sp.setParameter(0,-EFAULT); //TODO: stub
                break;
            }

            case Syscall::EXIT:
            {
                exitCode=(sp.getParameter(0) & 0xff)<<8;
                return Exit;
            }

            case Syscall::EXECVE:
            {
                auto path=reinterpret_cast<const char*>(sp.getParameter(0));
                auto argv=reinterpret_cast<char* const*>(sp.getParameter(1));
                auto envp=reinterpret_cast<char* const*>(sp.getParameter(2));
                int narg=validateStringArray(mpu,argv);
                int nenv=validateStringArray(mpu,envp);
                if(mpu.withinForReading(path) && narg>=0 && nenv>=0)
                {
                    ArgsBlock args(argv,envp,narg,nenv);
                    if(args.valid())
                    {
                        ElfProgram program(path);
                        if(program.errorCode()==0)
                        {
                            try {
                                //TODO: when threads within processes are
                                //implemented, kill all other threads
                                load(std::move(program),std::move(args));
                            } catch(exception& e) {
                                //TODO currently load causes the old process
                                //ram to be deallocated before allocating the
                                //new one. If the new allocation fails, then
                                //we should get back to the old process with an
                                //error code but we can't because its memory is
                                //gone. Until a safe realloc function is
                                //implemented in the process pool, we have to
                                //segfault here
                                return Segfault;
                            }
                            return Execve;
                        } else sp.setParameter(0,program.errorCode());
                    } else sp.setParameter(0,-E2BIG);
                } else sp.setParameter(0,-EFAULT);
                break;
            }

            case Syscall::SPAWN:
            {
                auto pidp=reinterpret_cast<pid_t*>(sp.getParameter(0));
                auto path=reinterpret_cast<const char*>(sp.getParameter(1));
                auto argv=reinterpret_cast<char* const*>(sp.getParameter(2));
                auto envp=reinterpret_cast<char* const*>(sp.getParameter(3));
                int narg=validateStringArray(mpu,argv);
                int nenv=validateStringArray(mpu,envp);
                if((!pidp || mpu.withinForWriting(pidp,sizeof(pid_t))) &&
                   mpu.withinForReading(path) && narg>=0 && nenv>=0)
                {
                    auto pid=Process::spawn(path,argv,envp,narg,nenv);
                    if(pid>=0)
                    {
                        if(pidp) *pidp=pid;
                        sp.setParameter(0,0);
                    } else sp.setParameter(0,-pid); //NOTE: positive error codes
                } else sp.setParameter(0,EFAULT); //NOTE: positive error codes
                break;
            }

            case Syscall::KILL:
            {
                sp.setParameter(0,-EFAULT); //TODO: stub
                break;
            }

            case Syscall::WAITPID:
            {
                int pid=sp.getParameter(0);
                auto wstatus=reinterpret_cast<int*>(sp.getParameter(1));
                int options=sp.getParameter(2);
                if(mpu.withinForWriting(wstatus,sizeof(int)) && aligned(wstatus))
                {
                    int result=Process::waitpid(pid,wstatus,options);
                    sp.setParameter(0,result);
                } else sp.setParameter(0,-EFAULT);
                break;
            }

            case Syscall::GETPID:
            {
                sp.setParameter(0,this->pid);
                break;
            }

            case Syscall::GETPPID:
            {
                sp.setParameter(0,this->ppid);
                break;
            }

            case Syscall::GETUID:
            {
                sp.setParameter(0,-EFAULT); //TODO: stub
                break;
            }

            case Syscall::GETGID:
            {
                sp.setParameter(0,-EFAULT); //TODO: stub
                break;
            }

            case Syscall::GETEUID:
            {
                sp.setParameter(0,-EFAULT); //TODO: stub
                break;
            }

            case Syscall::GETEGID:
            {
                sp.setParameter(0,-EFAULT); //TODO: stub
                break;
            }

            case Syscall::SETUID:
            {
                sp.setParameter(0,-EFAULT); //TODO: stub
                break;
            }

            case Syscall::SETGID:
            {
                sp.setParameter(0,-EFAULT); //TODO: stub
                break;
            }

            case Syscall::MOUNT:
            {
                sp.setParameter(0,-EFAULT); //TODO: stub
                break;
            }

            case Syscall::UMOUNT:
            {
                sp.setParameter(0,-EFAULT); //TODO: stub
                break;
            }

            case Syscall::MKFS:
            {
                sp.setParameter(0,-EFAULT); //TODO: stub
                break;
            }

            default:
                exitCode=SIGSYS; //Bad syscall
                #ifdef WITH_ERRLOG
                iprintf("Unexpected syscall number %d\n",sp.getSyscallId());
                #endif //WITH_ERRLOG
                return Segfault;
        }
    } catch(exception& e) {
        sp.setParameter(0,-ENOMEM);
    }
    return Resume;
}

pid_t Process::getNewPid()
{
    auto& p=Processes::instance();
    for(;;p.pidCounter++)
    {
        if(p.pidCounter<0) p.pidCounter=1;
        if(p.pidCounter==0) continue; //Zero is not a valid pid
        map<pid_t,ProcessBase*>::iterator it=p.processes.find(p.pidCounter);
        if(it!=p.processes.end()) continue; //Pid number already used
        return p.pidCounter++;
    }
}

//
// class ArgsBlock
//

ArgsBlock::ArgsBlock(const char* const* argv, const char* const* envp, int narg,
        int nenv) : narg(narg)
{
    constexpr int maxArg=MAX_PROCESS_ARGS;
    constexpr unsigned int maxArgsBlockSize=MAX_PROCESS_ARGS_BLOCK_SIZE;
    //Long long to prevent malicious overflows
    unsigned long long arrayBlockSize=sizeof(char*)*(narg+nenv+2);
    unsigned long long argBlockSize=arrayBlockSize;
    for(int i=0;i<narg;i++) argBlockSize+=strlen(argv[i])+1;
    for(int i=0;i<nenv;i++) argBlockSize+=strlen(envp[i])+1;
    if(narg>maxArg || nenv>maxArg || argBlockSize>maxArgsBlockSize) return;
    blockSize=argBlockSize;

    //Although the args block is is not considered part of the stack, it
    //defines the initial stack pointer value when the process starts.
    //As such, its size must be aligned to the platform-defined alignment
    unsigned int blockSizeBeforeAlign=blockSize;
    blockSize+=CTXSAVE_STACK_ALIGNMENT-1;
    blockSize/=CTXSAVE_STACK_ALIGNMENT;
    blockSize*=CTXSAVE_STACK_ALIGNMENT;
    //May exceed max size because of alignment
    if(blockSize>maxArgsBlockSize) return;

    block=new char[blockSize];
    //Zero the padding introduced for alignment
    memset(block+blockSizeBeforeAlign,0,blockSize-blockSizeBeforeAlign);

    envArrayIndex=sizeof(char*)*(narg+1);
    char **arrayBlock=reinterpret_cast<char**>(block);
    char *stringBlock=block+arrayBlockSize;
    //NOTE: also arg array ends with nullptr
    auto add=[&](const char* const* a, int n)
    {
        for(int i=0;i<n;i++)
        {
            *arrayBlock++=stringBlock;
            strcpy(stringBlock,a[i]);
            stringBlock+=strlen(a[i])+1;
        }
        *arrayBlock++=nullptr;
    };
    add(argv,narg);
    add(envp,nenv);
}

void ArgsBlock::relocateTo(char *target)
{
    memcpy(target,block,blockSize);
    auto relocate=[=](char *a)
    {
        char **element=reinterpret_cast<char**>(a);
        for(;*element!=nullptr;element++) *element=*element-block+target;
    };
    relocate(target);
    relocate(target+envArrayIndex);
    //memDump(target,blockSize); //For debugging
}

ArgsBlock::~ArgsBlock()
{
    if(block) delete[] block;
}

} //namespace miosix

#endif //WITH_PROCESSES
