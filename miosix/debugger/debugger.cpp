#include "debugger.h"

#ifdef PROCESS_DEBUGGER

#include <cstring>
#include <fcntl.h>
#include <kernel/process.h>
#include <kernel/thread.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <interfaces/endianness.h>

namespace miosix {

AttachedProcessInfo Debugger::attached;
RegisterFile Debugger::registerFile;
bool Debugger::failed = false;
Thread* Debugger::thread = nullptr;

#define BUF_FORMAT(x, ...) x.setLen(sniprintf(x.getData(), x.size, __VA_ARGS__))

#define layoutMacro(m,p)\
    extern char __##m##_##p asm("_"#m"_"#p); \
    const auto _##m##_##p = reinterpret_cast<const unsigned int>(&__##m##_##p)

// Accessory functions only used here
// Only makes sense with characters 0-9 and a-f
unsigned char asciiToHex(char character) {
    return (character <= '9')
        ? character - '0'
        : character - 'a' + 0xa
        ;
}

char hexToAscii(unsigned char value) {
    static const char * const asciiMap = "0123456789abcdef";
    return asciiMap[value & 0xf];
}

int GDBBuffer::appendChar(char character) {
    if (available() <= 1) return 0;
    data[head++] = character;
    data[head] = '\0';
    return 1;
}

int GDBBuffer::appendString(const char* str, unsigned int len) {
    if (available() <= static_cast<int>(len)) return 0;
    memcpy(&data[head], str, len);
    head += len;
    data[head] = '\0';
    return len;
}

int GDBBuffer::setReturnCode(GDBReturnCode code) {
    const auto len = (code == GDBReturnCode::OK)
        ? sniprintf(data, size, "OK")
        : sniprintf(data, size, "E%02x", code)
        ;
    if (available() <= len) return 0;
    head = len;
    return len;
}

int GDBBuffer::appendRegister32(unsigned int value) {
    const auto len = sniprintf(&data[head], available(), "%08x", value);
    if (available() <= len) return 0;
    head += len;
    return len;
}

int GDBBuffer::appendRegister64(unsigned long long value) {
    const auto len = sniprintf(&data[head], available(), "%016llx", value);
    if (available() <= len) return 0;
    head += len;
    return len;
}

int GDBBuffer::appendBytes(const unsigned int* addr, unsigned int size) {
    const auto len = size * 2;
    if (available() <= len) return 0;
    for(unsigned int i = 0; i < size; i++) {
        const char byte = reinterpret_cast<const char*>(addr)[i];
        data[head++] = hexToAscii(byte >>   4);
        data[head++] = hexToAscii(byte &  0xf);
    }
    data[head] = '\0';
    return len;
}

void Debugger::listen(char serialName[]) {
    failed = false;
    serial = open(serialName, O_RDWR | O_NOCTTY);
    if(serial < 0) {
        perror("open");
        fail();
        return;
    }
    fiprintf(stderr, "Debugger listening on %s\n", serialName);
    struct termios tio;
    tcgetattr(serial, &tio);
    // Both input and local modes:
    // - Disable canonical mode
    // - Disable echo
    // - Disable signals, pass command as is
    tio.c_lflag &= ~(ICANON | ECHO | ISIG);
    tio.c_iflag &= ~(ICANON | ECHO | ISIG);
    // Disable output processing (newline etc.)
    tio.c_oflag &= ~OPOST;
    tcsetattr(serial, TCSANOW, &tio);

    thread = Thread::getCurrentThread();

    while (!failed) {
        recvPacket();
        handleCommand();
        sendPacket();
    }

    thread = nullptr;

    fiprintf(stderr, "Debugger terminated\n");
    close(serial);
}

void Debugger::recvPacket() {
    char cc;
    char sum = 0;
    enum MessageState {
        BEGIN,
        PAYLOAD,
        CHECKSUM_0,
        CHECKSUM_1
    } state = BEGIN;

    while(!failed) {
        if(read(serial, &cc, 1) <= 0) {
            perror("read");
            fail();
            return;
        }
        
        // Handle special characters
        if(cc == ((char) 0x03)) {
            write(serial, "+", 1); continue;
        }
        if(cc == ((char) 0xf0)) {
            write(serial, "+", 1); continue;
        }

        switch (state) {
        case BEGIN: {
            if (cc == '$') {
                buffer.clear();
                state = PAYLOAD;
            }
        } break;
        case PAYLOAD: {
            (cc == '#')
                ? state = CHECKSUM_0
                : buffer.appendChar(cc);
                ;
        } break;
        case CHECKSUM_0: {
            sum = asciiToHex(cc) << 4;
            state = CHECKSUM_1;
        } break;
        case CHECKSUM_1:
            sum |= asciiToHex(cc);
            if (sum == buffer.checksum()) {
                // ACK + and return
                write(serial, "+", 1);
                return;
            }
            write(serial, "-", 1);
            state = BEGIN;
        }
    }
}

void Debugger::sendPacket() {
    if (failed) return;
    const auto sum = buffer.checksum();
    const char strSum[2] = {
        hexToAscii(sum >> 4),
        hexToAscii(sum & 0xf)
    };
    char cc = '-';
    do {
        write(serial, "$",              1);
        write(serial, buffer.getData(), buffer.len());
        write(serial, "#",              1);
        write(serial, strSum,           2);

        if(read(serial, &cc, 1) <= 0) {
            perror("read");
            fail();
            return;
        };
    } while(cc != '+');
}

void Debugger::handleCommand() {

    switch (buffer.getData()[0]) {
    case '!':   buffer.setReturnCode(OK);   break;
    case '?':   stopReply();                break;
    case 'c':
    case 's':   handleCommand_cs();         break;
    case 'D':   handleCommand_D();          break;
    case 'g':
    case 'G':   handleCommand_gG();         break;
    case 'm':
    case 'M':   handleCommand_mM();         break;
    case 'p':
    case 'P':   handleCommand_pP();         break;
    case 'q':   handleCommand_q();          break;
    case 'v':   handleCommand_v();          break;
    case 'z':
    case 'Z':   handleCommand_zZ();         break;
    default:    buffer.clear();
    }
}

void Debugger::stopReply() {

    {
        FastGlobalIrqLock dLock;
        // While process is running
        while(attached.running) {
            // Set thread to notify
            Thread::IRQglobalIrqUnlockAndWait(dLock);
        }
    }
    // When code is here the attached process must already be stopped, not
    // checking again

    // NOTE: on all exit condition (fault, execve, fault) set attached.process
    // before the wait, to prevent it triggering with reclaimed memory on clear
    switch (attached.reason) {
    case StopReason::NONE: {
        BUF_FORMAT(buffer, "W" "00");
    } break;
    case StopReason::DEBUGEVENT: {
        // TODO: Support multiple debug halting reason
        BUF_FORMAT(buffer, "S" "%02x", SIGTRAP);
    } break;
    case StopReason::EXIT: {
        attached.process = nullptr;
        attached.pid = wait(&attached.ec);
        BUF_FORMAT(buffer, "W" "%02x", attached.code & 0xff);
        attached.clear();
        BreakpointUnit::clear();
    } break;
    case StopReason::FAULT: {
        attached.process = nullptr;
        attached.pid = wait(&attached.ec);
        buffer.clear();
        buffer.appendChar('O');
        buffer.appendBytes("Program fault has occurred: 0x");
        const char retCode[] = {hexToAscii(attached.code >> 4),
                                hexToAscii(attached.code & 0xf),
                                '\0'};
        buffer.appendBytes(retCode);
        attached.clear();
        BreakpointUnit::clear();
        sendPacket();
        BUF_FORMAT(buffer, "X" "%02x", SIGKILL);
    } break;
    case StopReason::EXECVE:
        BreakpointUnit::clear();
        if (features.supported(GDBFeatures::EXEC_EVENTS)) {
            BUF_FORMAT(buffer, "T" "%02x" "exec:", SIGTRAP);
            buffer.appendBytes(Debugger::attached.name);
            Debugger::attached.name = nullptr;
        } else {
            attached.process = nullptr;
            attached.pid = wait(&attached.ec);
            attached.clear();
            BUF_FORMAT(buffer, "S" "%02x", SIGTRAP);
        }
    };
}

static inline void appendBigEndian64(GDBBuffer& buffer, char* valPtr) {
    const auto ptr = reinterpret_cast<unsigned long long*>(valPtr); 
    *ptr = toBigEndian64(*ptr); 
    buffer.appendRegister64(*ptr);
}

static inline void appendBigEndian32(GDBBuffer& buffer, char* valPtr) {
    const auto ptr = reinterpret_cast<unsigned int*>(valPtr); 
    *ptr = toBigEndian32(*ptr); 
    buffer.appendRegister32(*ptr); 
}

static inline void parseBigEndian64(char* readPtr, char* valPtr) {
    const auto ptr = reinterpret_cast<unsigned long long*>(valPtr); 
    *ptr = strtoull(readPtr, nullptr, 16);
    *ptr = toBigEndian64(*ptr);
}

static inline void parseBigEndian32(char* readPtr, char* valPtr) {
    const auto ptr = reinterpret_cast<unsigned int*>(valPtr);
    *ptr = strtoul(readPtr, nullptr, 16);
    *ptr = toBigEndian32(*ptr);
}

void Debugger::handleCommand_gG() {

    const auto read = buffer.getData()[0] == 'g';
    char valPtr[MAX_REGISTER_SIZE_BYTES];

    if (read) {
        buffer.clear();
        for (int i = 0; i < registerFile.entries; i++) {
            const auto size = registerFile.getSize(i);
            if(! registerFile.read(attached.thread, i, valPtr)) {
                #if FPU_REGISTERS == 1
                if (size == 8)
                    buffer.appendString("xxxxxxxxxxxxxxxx", 16);
                else
                    buffer.appendString("xxxxxxxx", 8);
                #endif
                continue;
            }
            #if FPU_REGISTERS == 1
            if (size == 8)
                appendBigEndian64(buffer,valPtr);
            else if (size == 4)
            #endif
                appendBigEndian32(buffer,valPtr);
        }
    } else {
        if (buffer.len() != registerFile.sizeBytes * 2) {
            buffer.setReturnCode(REGISTER_WRITE_FAIL);
            return;
        }

        auto readPtr = buffer.getData();
        for(int i = 0; i < registerFile.entries ; i ++) {
            const auto size = registerFile.getSize(i);
            const auto readSize = size * 2;
            const auto oldChar = readPtr[readSize];
            readPtr[readSize] = '\0';
            #if FPU_REGISTERS == 1
            if (size == 8)
                parseBigEndian64(readPtr, valPtr);
            else if (size == 4)
            #endif
                parseBigEndian32(readPtr, valPtr);

            if(! registerFile.write(attached.thread, i, valPtr)) {
                buffer.setReturnCode(GDBReturnCode::REGISTER_WRITE_FAIL);
                return;
            }
            readPtr[readSize] = oldChar;
            readPtr += readSize;
        }
        buffer.setReturnCode(OK);
    }
}

void Debugger::handleCommand_pP() {

    const auto read = buffer.getData()[0] == 'p';
    char* separator;
    const auto entry = strtoul(buffer.getData() + 1, &separator, 16);
    const auto size = registerFile.getSize(entry);
    char valPtr[MAX_REGISTER_SIZE_BYTES];

    if (read) {
        buffer.clear();
        if(! registerFile.read(attached.thread, entry, valPtr)) {
            for(int i=0; i < (size * 2); i++)
                buffer.appendChar('x');
            return;
        }
        #if FPU_REGISTERS == 1
        if (size == 8)
            appendBigEndian64(buffer, valPtr);
        else if (size == 4)
        #endif
            appendBigEndian32(buffer, valPtr);
    } else {
        const auto readPtr = separator + 1;
        #if FPU_REGISTERS == 1
        if (size == 8)
            parseBigEndian64(readPtr, valPtr);
        else if (size == 4)
        #endif
            parseBigEndian32(readPtr, valPtr);
        buffer.setReturnCode(registerFile.write(attached.thread, entry, valPtr)
                ? OK
                : REGISTER_WRITE_FAIL);
    }
}

void Debugger::handleCommand_mM() {
    const auto read = buffer.getData()[0] == 'm';
    char* separator;
    const auto baseAddress = reinterpret_cast<char*>(strtoul(buffer.getData() + 1, &separator, 16));
    const auto len = strtoul(separator + 1, &separator, 16);

    if (read) {
        if (! attached.process->mpu.withinForReading(baseAddress, len)) {
            buffer.setReturnCode(MEMORY_READ_FAIL);
            return;
        }
        buffer.clear();
        buffer.appendBytes(reinterpret_cast<unsigned int*>(baseAddress), len);
    } else {
        // When running code from RAM: this message is used instead of Z0 to
        // support software breakpoints:
        // The memory area to be written must be readable from the process (both
        // code and data are valid)
        // In addition the area must be within _process_pool, this prevents
        // attempts at modifying flash memory addresses
        // * Cannot simply use
        // if (! attached.process->mpu.withinForWriting(baseAddress, len)) {
        // * as it would fail on an attempt to write inside code section
        layoutMacro(process_pool, start);
        layoutMacro(process_pool, end);
        const bool inProcessMem = attached.process->mpu.withinForReading(baseAddress, len);
        const auto base = reinterpret_cast<unsigned int>(baseAddress);
        const bool inProcessPool =  base        >= _process_pool_start
                                 && base + len  <= _process_pool_end
                                 && base + len  >= base;

        if (! (inProcessMem && inProcessPool)) {
            buffer.setReturnCode(MEMORY_WRITE_FAIL);
            return;
        }
        const char *const messageBase = buffer.getData();
        const unsigned int hexOffset = (separator - messageBase) + 1;
        for(unsigned int  writeIdx =  0    ,   readIdx =  hexOffset
                        ; writeIdx <  len  &&  readIdx <  buffer.len()
                        ; writeIdx ++      ,   readIdx += 2) {
            baseAddress[writeIdx] = (asciiToHex(messageBase[readIdx]) << 4)
                                  |  asciiToHex(messageBase[readIdx + 1])
                                  ;
        }
        buffer.setReturnCode(OK);
    }
}

void Debugger::handleCommand_cs() {
    // Prepare struct to wake up thread
    attached.running = true;
    // NOTE: It's mandatory to set stopreason to NONE as only the first thread
    // which triggers an event can set attached.reason, this is done by checking
    // on the stopreason
    attached.reason = StopReason::NONE;
    attached.thread->debugStatus = (buffer.getData()[0] == 'c')
                                 ? DebugStatus::RUN
                                 : DebugStatus::STEP
                                 ;
    attached.thread->debugWakeup();

    stopReply();
}

void Debugger::handleCommand_D() {
    // Stop debugging process and wake up its only (assuming single thread
    // execution)
    {
        FastGlobalIrqLock dLock;
        attached.process = nullptr;
        attached.thread->IRQdebugWakeup();
    }
    attached.clear();
    BreakpointUnit::clear();
    buffer.setReturnCode(OK);
}

void Debugger::handleCommand_v() {

    VMessage vMessage;
    parsePacket_v(&vMessage);
    switch(vMessage.type) {
    case VMessage::VRUN:    vrun();     break;
    case VMessage::VATTACH: vattach();  break;
    default:
        buffer.clear();
    }
}

void Debugger::handleCommand_q() {

    QMessage qMessage;
    parsePacket_q(&qMessage);

    switch (qMessage.type) {
    case QMessage::SUPPORTED: {
        if (strstr(buffer.getData(), "exec-events+")) features.set(GDBFeatures::EXEC_EVENTS);
        BUF_FORMAT(buffer,
            "PacketSize=%x"
            ";exec-events+"
            ";qXfer:features:read+"
            ";qXfer:memory-map:read+"
            , buffer.size -1);
    } break;
    case QMessage::OFFSETS: {
        const auto textSegment = attached.process->program.getElfBase();
        const auto dataSegment = reinterpret_cast<unsigned int>(
                attached.process->image.getProcessBasePointer());
        BUF_FORMAT(buffer,
            "Text=%x;Data=%x;Bss=%x", textSegment, dataSegment, dataSegment);
    } break;
    case QMessage::FEATURES: {
        const bool more = qMessage.xmlView(targetXMLStringLen);
        if (buffer.size <= qMessage.length + 1) {
            buffer.setReturnCode(MEMORY_READ_FAIL);
            return;
        }
        buffer.clear();
        buffer.appendChar(more ? 'm' : 'l');
        buffer.appendString((char*)&targetXMLString[qMessage.offset], qMessage.length);
    } break;
    case QMessage::MEMORY_MAP: {
        layoutMacro(flash,          origin);
        layoutMacro(flash,          length);
        layoutMacro(flash,          erasesize);
        layoutMacro(process_pool,   start);
        layoutMacro(process_pool,   end);
        const auto _process_pool_length = _process_pool_end - _process_pool_start;
        // TODO: This is not the proper way to communicate memory layout, it
        // will break if the length requested by GDB is smaller than the message
        // provided, gdb requests a length appropriate for the advertised
        // buffersize if this fail I will have to implement this as a separated
        // template string with fixed length (numbers as 0x........), modify it
        // in place at setup and provide chunks as requested
        // NOTE: That the first character 'l' only makes sense if I assume the
        // whole message is sent as a single packet, otherwise I will have to
        // properly implement 'm' and 'l' messages
        // TODO: How to determine blocksize?
        BUF_FORMAT(buffer,
            "l"
            "<memory-map>"
                "<memory type=\"ram\" start=\"0x%x\" length=\"0x%x\"/>"
                "<memory type=\"flash\" start=\"0x%x\" length=\"0x%x\">"
                    "<property name=\"blocksize\">0x%x</property>"
                "</memory>"
            "</memory-map>",
            _process_pool_start, _process_pool_length, _flash_origin, _flash_length, _flash_erasesize);
    } break;
    default:
        buffer.clear();
    }
}

void Debugger::handleCommand_zZ() {
    const bool insert = buffer.getData()[0] == 'Z';
    char* separator;
    const auto baseAddress  = strtoul(buffer.getData() + 3, &separator, 16);
    const auto kind         = strtoul(separator        + 1, &separator, 16);
    if (separator < buffer.getData() + buffer.len()) {
        // Additional bp features: not implemented
        buffer.clear();
        return;
    }

    if(! Debugger::attached.process->mpu.withinForReading(reinterpret_cast<char*>(baseAddress), kind)) {
        buffer.setReturnCode(BREAKPOINT_SET_FAIL);
        return;
    }

    int idx = -1;
    switch(buffer.getData()[1]) {
    case '1': {
        if (kind != 2 && kind != 4) break;
        idx = insert ? BreakpointUnit::addBreakpoint(   baseAddress, kind)
                     : BreakpointUnit::removeBreakpoint(baseAddress, kind);
    } break;
    case '2': {
        idx = insert ? BreakpointUnit::addWatchpoint(   baseAddress, kind, WatchpointType::WRITE)
                     : BreakpointUnit::removeWatchpoint(baseAddress, kind, WatchpointType::WRITE);
    } break;
    case '3': {
        idx = insert ? BreakpointUnit::addWatchpoint(   baseAddress, kind, WatchpointType::READ)
                     : BreakpointUnit::removeWatchpoint(baseAddress, kind, WatchpointType::READ);
    } break;
    case '4': {
        idx = insert ? BreakpointUnit::addWatchpoint(   baseAddress, kind, WatchpointType::ACCESS)
                     : BreakpointUnit::removeWatchpoint(baseAddress, kind, WatchpointType::ACCESS);
    } break;
    default:
        buffer.clear();
        // Default behaviour: not implemented, empty reply
        return;
    }
    if (idx < 0) {
        // If BP/WP type is supported but no slot is available: report fail
        buffer.setReturnCode(BREAKPOINT_FULL);
        return;
    }
    buffer.setReturnCode(OK);
}

void Debugger::vrun() {
    // This code is cursed, it works, I won't touch it further
    // TODO: I should, and then I can inline it in handleV

    char* const str = strchr(buffer.getData(), ';');

    // Starting from first character of the buffer after ; (vRun;2f62696e2f666f6f;626172)
    //                                                           ^
    // If character is a semi ';' terminate string '\0'
    // Transform hex pairs into characters 2f62696e -> "/bin"
    // TODO: make use of the whole buffer
    int argsNum = 0;
    const char* str_end = buffer.getData() + buffer.len();
    char* w_ptr = str;
    for(char* r_ptr = str; r_ptr < str_end - 1 && *r_ptr; w_ptr ++) {
        if (*r_ptr == ';') {
            *w_ptr = '\0';
            argsNum ++;
            r_ptr++;
        } else {
            char cc =  asciiToHex(r_ptr[0]) << 4;
                 cc |= asciiToHex(r_ptr[1]);
            *w_ptr = cc;
            r_ptr += 2;
        }
    }   
    *w_ptr = '\0';
    char** args = new char*[argsNum+1];
    char* ptr = str+1;
    args[argsNum] = nullptr;
    for (int i = 0; i < argsNum; i++) {
        args[i] = ptr;
        ptr += strlen(ptr) + 1;
    }

    // posix_spawn must remain unix compliant, it's interface cannot be
    // modified, this is not the proper way to communicate with process
    // creation, A fork() should be performed and the child process should be
    // set traceable before execve, but since this mechanism is not present in
    // miosix due to lack of a fork syscall, which is way more expensive than a
    // direct process spawn

    attached.ec=posix_spawn(&(attached.pid),args[0],nullptr,nullptr,args, nullptr);
    if (attached.ec != 0) {
        buffer.setReturnCode(GDBReturnCode::SPAWN_FAIL);
        return;
    }

    // If process creation comletes successfully: attached.process is set
    delete[] args;

    // once thread is no longer running (can be as soon as created (skip right
    // away) can continue execution

    stopReply();
}

// TODO: check if logic to determine live threads is correct
void Debugger::vattach() {
    char* const str = strchr(buffer.getData(), ';');
    const auto pid = strtoul(str + 1, nullptr, 16);

    buffer.clear();

    Process* proc = attached.process->debugGetByPid(pid);
    // Provided pid is not valid
    if (proc == nullptr) {
        buffer.setReturnCode(GDBReturnCode::ATTACH_FAIL);
        return;
    }

    if (proc->program.isCopiedInRam() &&
            (!proc->priv)) {
        BUF_FORMAT(buffer,
                "E.Cannot attach to process with code inside RAM unless the debugger spawned it");
        return;
    }

    {
        FastGlobalIrqLock dLock;
        Thread* th;
        for (const auto& thread : proc->threads) {
            if (thread->flags.isDeleted() == false
            && thread->flags.isDeleting() == false) {
                th = thread;
                break;
            }
        }
        // If it's not waiting due to a debug event, wake it up, otherwise keep
        // it in debug wait and continue (preserving attached info)
        if (! th->flags.isWaitingDebug()) {
            attached.process                = proc;
            attached.running                = true;
            attached.pid                    = pid;
            th->debugStatus                 = DebugStatus::PEND;
            th->IRQwakeup();
        }
    }
    stopReply();
}

#define strEq(ptr, str)         (!strcmp(ptr, str))
#define strParEq(ptr, str)      (!strncmp(ptr, str, sizeof(str) - 1))

void Debugger::parsePacket_v(VMessage* vMessage) {
    static const char vrun[]    = "vRun";
    static const char vattach[]    = "vAttach";

    const auto ptr = buffer.getData();

    if (strParEq(ptr, vrun)) {
        vMessage->type = VMessage::VRUN;
    } else if (strParEq(ptr, vattach)) {
        vMessage->type = VMessage::VATTACH;
    }
}

void Debugger::parsePacket_q(QMessage* qMessage) {
    static const char qsupported[]  = "qSupported";
    static const char qoffsets[]    = "qOffsets";
    static const char q_mem_map[]   = "qXfer:memory-map:read:"/*annex not present:*/;
    //                                                      ^ Skip annex parsing 
    static const char q_feats[]     = "qXfer:features:read:target.xml"/*:*/;
    //                                                    ^ Annex: target only

          auto ptr   = buffer.getData();
    const auto limit = buffer.getData() + buffer.len();

    // Simple packets
    if (strParEq(ptr, qsupported))
        { qMessage->type = QMessage::SUPPORTED; return; }
    if (strEq(ptr, qoffsets))
        { qMessage->type = QMessage::OFFSETS; return; }
    
    // Packets with additional data
    if (strParEq(ptr, q_mem_map)) {
        if ((ptr += sizeof(q_mem_map)) >= limit) return;
        qMessage->type = QMessage::MEMORY_MAP;
    } else if (strParEq(ptr, q_feats)) {
        if ((ptr += sizeof(q_feats)) >= limit) return;
        qMessage->type = QMessage::FEATURES;
    }

    char* separator;
    qMessage->offset = strtoul(ptr,             &separator, 16);
    qMessage->length = strtoul(separator + 1,   &separator, 16);
}

unsigned int BreakpointUnit::dirty      = 0xffffffff;
int BreakpointUnit::breakpointsNum      = fpbGetAvailableBreakpoints();
int BreakpointUnit::watchpointsNum      = fpbGetAvailableWatchpoints();
Breakpoint* BreakpointUnit::breakpoints = new Breakpoint[fpbGetAvailableBreakpoints()];
Watchpoint* BreakpointUnit::watchpoints = new Watchpoint[fpbGetAvailableWatchpoints()];

const unsigned int BreakpointUnit::mask         = fpbGetSupportedWatchpointMask(),
                   BreakpointUnit::revision     = fpbGetRevisionVersion(),
                   BreakpointUnit::writeMask    = fpbGetWriteMask();

}

#endif
