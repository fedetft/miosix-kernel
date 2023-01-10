
// TODO move code to cpu-specific header and delete this arch specific header

#ifdef WITH_PROCESSES

namespace {
/*
 * ARM syscall parameter mapping
 * Syscall id is r3, saved at registers[3]
 *
 * Parameter 1 is r0, saved at registers[0]
 * Parameter 2 is r1, saved at registers[1]
 * Parameter 3 is r2, saved at registers[2]
 * Parameter 4 is r12, saved at registers[4]
 */
constexpr unsigned int armSyscallMapping[]={0,1,2,4};
}

//
// class SyscallParameters
//

inline SyscallParameters::SyscallParameters(unsigned int *context) :
        registers(reinterpret_cast<unsigned int*>(context[0])) {}

inline int SyscallParameters::getSyscallId() const
{
    return registers[3];
}

inline unsigned int SyscallParameters::getParameter(unsigned int index) const
{
    assert(index<4);
    return registers[armSyscallMapping[index]];
}

inline void SyscallParameters::setParameter(unsigned int index, unsigned int value)
{
    assert(index<4);
    registers[armSyscallMapping[index]]=value;
}

inline void portableSwitchToUserspace()
{
    asm volatile("movs r3, #1\n\t"
                 "svc  0"
                 :::"r3");
}

#endif //WITH_PROCESSES
