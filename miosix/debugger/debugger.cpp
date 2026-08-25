#include "debugger.h"
#include "interfaces-impl/debug_registers.h"
#include "miosix_settings.h"

#ifdef PROCESS_DEBUGGER

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
#include "interfaces/debugger.h"

#ifdef LOGGING
    #define dbg(...) fprintf(stdout, "[DBG]: " __VA_ARGS__)
#else//LOGGING
    #define dbg(...) do {} while (0)
#endif//LOGGING

namespace miosix {

AttachedProcessInfo Debugger::attached;
bool Debugger::failed = false;
Thread* Debugger::thread = nullptr;
int Debugger::needJoin = 0;

// TODO: might improve, these are a bit hacky
// Shorthand combining sniprintf on gdbbufer and setLen for the same
#define BUF_FORMAT(x, ...) x.setLen(sniprintf(x.getData(), x.size, __VA_ARGS__))

// Shorthand to import extern definition for process_pool_start and process_pool_end
#define IMPORT_SYMBOL(m,p)\
    extern char __##m##_##p asm("_"#m"_"#p); \
    const auto _##m##_##p = reinterpret_cast<const unsigned int>(&__##m##_##p)

// printf can format in hexadecimal, but some messages require reverted endianess or may refer to wide memory
// area (memory read usually takes up to 64 bytes at time

// Accessory functions only used here: convert ascii 0-9,a-f to int
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

// GDBBuffer:
// - functions "setXX" reset the buffer content
// - functions "appendXX" append content to the end of already existing buffer
//                        require buffer.clear() to write from beginning

// Append a single character, preserves null-termination
int GDBBuffer::appendChar(char character) {
    if (available() <= 1) return 0;
    data[head++] = character;
    data[head] = '\0';
    return 1;
}

// Append multiple characters, preserves null-termination
int GDBBuffer::appendString(const char* str, unsigned int len) {
    if (available() <= static_cast<int>(len)) return 0;
    memcpy(&data[head], str, len);
    head += len;
    data[head] = '\0';
    return len;
}

// Set return code
int GDBBuffer::setReturnCode(GDBReturnCode code) {
    const auto len = (code == GDBReturnCode::OK)
        ? sniprintf(data, size, "OK")
        : sniprintf(data, size, "E%02x", code)
        ;
    // This check is to keep implementation equal to all other, it is never
    // taken: buffer is always longer than 3 characters
    // if (size <= len) return 0;
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
    if (available() <= static_cast<int>(len)) return 0;
    for(unsigned int i = 0; i < size; i++) {
        const char byte = reinterpret_cast<const char*>(addr)[i];
        data[head++] = hexToAscii(byte >>   4);
        data[head++] = hexToAscii(byte &  0xf);
    }
    data[head] = '\0';
    return len;
}

// At any moment in loop: if "failed" is set skip any additional operation and
// return to caller (not using exceptions)
void Debugger::listen(int serial) {
    failed = false;
    this->serial = serial;

    dbg("listening on serial #%d\n", serial);
    dbg("hardware breakpoints: %d\n", fpbGetAvailableBreakpoints());
    dbg("hardware watchpoints: %d\n", fpbGetAvailableWatchpoints());

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

    // Set debugger thread when handling messages
    thread = Thread::getCurrentThread();

    // From now on, any debug event (SWBKPT, HWBKPT, Watchpiont, step, pend)
    // will trigger DebugMonitor rather than HardFault
    debugMonitorEnable();

    dbg("size: %d\n", buffer.size);
    while (!failed) {
        recvPacket();
        handleCommand();
        processCleanup();
        sendPacket();
    }

    // If debugger reaches here it means an unrecoverable fail happened
    // (read/write perror), behave like a detach: disable all debug hardware,
    // resume any halted thread
    handleCommand_D();
    // Unset debugger thread
    thread = nullptr;

    dbg("failed\n");
}

void Debugger::listen(char serialName[]) {
    dbg("open %s\n", serialName);
    const auto serial = open(serialName, O_RDWR | O_NOCTTY);

    if(serial < 0) {
        perror("open");
        fail();
        return;
    }

    listen(serial);

    dbg("close %s\n", serialName);
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

    // FSM:
    // - ack on 0xf0 and 0x03 (reset and Ctrl-C)
    // - discard any character until '$' is found (beginning of frame)
    // - read until '#'
    // - read checksumbytes and compare with message checksum, send ack and return/retry
    while(!failed) {
        if(read(serial, &cc, 1) < 0) {
            perror("read");
            fail();
            return;
        }

        // Handle special characters
        // if(cc == ((char) 0x03)) {
        //     write(serial, "+", 1); continue;
        // }
        // if(cc == ((char) 0xf0)) {
        //     write(serial, "+", 1); continue;
        // }

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
    // Compute checksum for message, send and retry until positive ack is received
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

        if(read(serial, &cc, 1) < 0) {
            perror("read");
            fail();
            return;
        };
    } while(cc != '+');
}

void Debugger::handleCommand() {

    switch (buffer.getData()[0]) {
    case '!':   buffer.setReturnCode(OK);   break;
                // When this function is executing, either the process doesn't
                // exists or it's in debugstate
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

    // Default reply if no program is running
    BUF_FORMAT(buffer, "W" "00");
    if (attached.noProgram()) return;

    // When code is here the attached process must already be stopped, not
    // checking again

    // NOTE: attached.IRQclear should be guarded by a GlobalIrqLock, but this
    // whole code secion executes as the attached process already stopped, any
    // debug event triggered in this section is not associated with debugged
    // process (escalates to HardFault rather than being handled by
    // DebugMonitor)

    switch (attached.event.reason) {
        // Should never happen
    case StopReason::NONE: break;
    case StopReason::DEBUGEVENT: {
        // Can report different stop reasons with 'T' packet
        // if reading from DFSR, but need to also report watchpoint/breakpoint
        // address
        BUF_FORMAT(buffer, "S" "%02x", SIGTRAP);
    } break;
    case StopReason::EXECVE: {
        BreakpointUnit::clear();
        if (features.supported(GDBFeatures::EXEC_EVENTS)) {
            // Client supports exec events, report them
            BUF_FORMAT(buffer, "T" "%02x" "exec:", SIGTRAP);
            buffer.appendBytes(Debugger::attached.name);
            buffer.appendChar(';');
            Debugger::attached.name = nullptr;
        } else {
            // Else just treat it as exit (source code has changed)
            BUF_FORMAT(buffer, "W" "00");
        }
    } break;
    case StopReason::EXIT: {
        // Report exit code
        BreakpointUnit::clear();
        BUF_FORMAT(buffer, "W" "%02x", attached.event.code & 0xff);
        {
            FastGlobalIrqLock dLock;
            attached.IRQclear();
        }
    } break;
    case StopReason::FAULT: {
        BreakpointUnit::clear();
        // Report fault code (additional packet 'O', contains FaultID)
        buffer.clear();
        buffer.appendChar('O');
        buffer.appendBytes("Program fault has occurred: 0x");
        const auto c = attached.event.code;
        const char retCode[] = {hexToAscii(c >> 4),
                                hexToAscii(c & 0xf),
                                '\0'};
        buffer.appendBytes(retCode);
        sendPacket();
        {
            FastGlobalIrqLock dLock;
            attached.IRQclear();
        }
        BUF_FORMAT(buffer, "X" "%02x", SIGKILL);
    } break;
    };
}

void Debugger::processCleanup() {
    int n = 0;
    {
        // I want to swap condition and lock and use volatile, but ++ is
        // deprecated for volatile
        FastGlobalIrqLock dLock;
        // Check There is at least one process to join
        if (needJoin > 0) {
            // needJoin is volatile, make sure to read it again once lock is taken
            // to get the exact value, then set it to 0
            n = needJoin;
            needJoin = 0;
        }
    }
    // Wait for n processes (spawned by debugger), ignore pid and status
    while (n-->0) wait(NULL);
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

    if (attached.noProgram()) {
        buffer.setReturnCode(FAIL);
        return;
    }

    const auto read = buffer.getData()[0] == 'g';
    char valPtr[MAX_REGISTER_SIZE_BYTES];

    if (read) {
        buffer.clear();
        for (int i = 0; i < RegisterFile::entries; i++) {
            const auto size = RegisterFile::getSize(i);
            if(RegisterFile::read(attached.thread, i, valPtr)) {
                #if __FPU_PRESENT == 1
                    // if FPU not implemented, skip conditions
                if (size == 8)
                    appendBigEndian64(buffer,valPtr);
                else if (size == 4)
                #endif
                    appendBigEndian32(buffer,valPtr);
            } else {
                // Write x-es if unaccessible
                for(int i=0; i < (size * 2); i++)
                    buffer.appendChar('x');
            }
        }
    } else {
        // - 1 since I have to take into account initial G
        if (buffer.len() - 1 != RegisterFile::sizeBytes * 2) {
            // G packet is sorter than expected
            buffer.setReturnCode(REGISTER_WRITE_FAIL);
            return;
        }

        // + 1 since I have to take into account intial G
        auto readPtr = buffer.getData() + 1;
        for(int i = 0; i < RegisterFile::entries ; i ++) {
            // In place (input buffer):
            // - get size of register i
            // - save character after last one of register and substitute with
            //   '\0' (for strtoul)
            // - write register
            // - restore character
            // - move to next register
            const auto size = RegisterFile::getSize(i);
            const auto readSize = size * 2;
            const auto oldChar = readPtr[readSize];
            readPtr[readSize] = '\0';
            // Guardds to skip check on architecture with no
            // floating point support
            #if __FPU_PRESENT == 1
            if (size == 8)
                parseBigEndian64(readPtr, valPtr);
            else if (size == 4)
            #endif
                parseBigEndian32(readPtr, valPtr);

            if(! RegisterFile::write(attached.thread, i, valPtr)) {
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

    if (attached.noProgram()) {
        buffer.setReturnCode(FAIL);
        return;
    }

    const auto read = buffer.getData()[0] == 'p';
    char* separator;
    const auto entry = strtoul(buffer.getData() + 1, &separator, 16);
    const auto size = RegisterFile::getSize(entry);
    char valPtr[MAX_REGISTER_SIZE_BYTES];

    if (read) {
        buffer.clear();
        if(! RegisterFile::read(attached.thread, entry, valPtr)) {
            for(int i=0; i < (size * 2); i++)
                buffer.appendChar('x');
            return;
        }
        // These guards are only to skip check on architecture with no
        // floating point support
        #if __FPU_PRESENT == 1
        if (size == 8)
            appendBigEndian64(buffer, valPtr);
        else if (size == 4)
        #endif
            appendBigEndian32(buffer, valPtr);
    } else {
        const auto readPtr = separator + 1;
        #if __FPU_PRESENT == 1
        if (size == 8)
            parseBigEndian64(readPtr, valPtr);
        else if (size == 4)
        #endif
            parseBigEndian32(readPtr, valPtr);
        buffer.setReturnCode(RegisterFile::write(attached.thread, entry, valPtr)
                ? OK
                : REGISTER_WRITE_FAIL);
    }
}

void Debugger::handleCommand_mM() {
    const auto read = buffer.getData()[0] == 'm';
    char* separator;
    const auto baseAddress = reinterpret_cast<char*>(strtoul(buffer.getData() + 1, &separator, 16));
    const auto len = strtoul(separator + 1, &separator, 16);

    // Again, this should not happen with a proper client
    if (attached.noProgram()) {
        buffer.setReturnCode(FAIL);
        return;
    }

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
        IMPORT_SYMBOL(process_pool, start);
        IMPORT_SYMBOL(process_pool, end);
        const bool inProcessMem = attached.process->mpu.withinForReading(baseAddress, len);
        const auto base = reinterpret_cast<unsigned int>(baseAddress);
        // - Could avoid third check (overflow protection) as it's implicit
        //   from inProcessMem (this check is included in mpu methods)
        // - Could use an inFlash variable and refuse if writing is within flash memory
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

    if (attached.noProgram()) {
        // No process is currently attached, attempting to resume execution,
        // return an error
        // While this should never happen with a proper client, it might lock
        // the server if not handled properly
        buffer.setReturnCode(RUN_FAIL);
        return;
    }

    auto t = attached.thread;

    {
        FastGlobalIrqLock dLock;
        // NOTE: It's mandatory to set stopreason to NONE as only the first thread
        // which triggers an event can set attached.reason, this is done by checking
        // on the stopreason

        attached.debugState = false;
        attached.event.reason = StopReason::NONE;
        // Wakeup thread
        t->debugStatus = (buffer.getData()[0] == 'c')
                                     ? DebugStatus::RUN
                                     : DebugStatus::STEP
                                     ;
        t->IRQdebugWakeup();
    }

    waitAttached();
    stopReply();
}

void Debugger::handleCommand_D() {
    // Cannot detach if there is no attached process
    if (attached.noProgram()) {
        buffer.setReturnCode(ATTACH_FAIL);
        return;
    }

    // Do not make shared: some breakpoints may have been inserted

    // Stop debugging process and wake up its only (assuming single thread
    // execution)
    {
        FastGlobalIrqLock dLock;
        attached.debugState = true;
        attached.thread->IRQdebugWakeup();
        attached.IRQclear();
    }
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
        if (attached.noProgram()) {
            buffer.setReturnCode(FAIL);
            return;
        }
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
        IMPORT_SYMBOL(process_pool,   start);
        IMPORT_SYMBOL(process_pool,   end);
        const auto _process_pool_length = _process_pool_end - _process_pool_start;
        // NOTE:  Flagging whole sectoin as flash (gdb is not intended to access
        // this memory area anyway)
        // NOTE: End of flash: Any address above 0x1fffffff is not
        // communicated as being part of flash, since rev.1 hw breakpoints
        // cannot address it anyway (mask is 0x1ffffff4) rev.2 bp can address
        // outside of this space, in which case  it's possible to tell gdb to
        // use hardware breakpoints, the server will prevent insertion attempts
        // outside of addressable space
        // TODO: This is not the proper way to communicate memory layout,
        // The protocol doensn't specify behavior if length provided is greater
        // than requested, GDB should request a length appropriate for the
        // advertised buffersize, if this fail I will have to implement this as
        // a separated template string with fixed length (numbers as 0x........)
        // modify it in place at setup and provide chunks as requested
        // NOTE: First character 'l' only makes sense if I assume the
        // whole message is sent as a single packet, otherwise I will have to
        // properly implement 'm' and 'l' messages
        //
        // Advertise memory as
        // +-----+ 0xffffffff
        // | ROM |
        // +-----+ _process_pool_end
        // | RAM | // && mpu.withinForRead() => Can insert swbreaks
        // +-----+ _process_pool_start
        // | ROM |
        // +-----+ 0x00000000
        //
        BUF_FORMAT(buffer,
            // 'l' Assumming requested length is greater than string length
            "l"
            "<memory-map>"
                "<memory type=\"rom\" start=\"0x00000000\" length=\"0x%x\"/>"
                "<memory type=\"ram\" start=\"0x%x\" length=\"0x%x\"/>"
                "<memory type=\"rom\" start=\"0x%x\" length=\"0x%x\"/>"
            "</memory-map>",
                                    _process_pool_start,
            _process_pool_start,    _process_pool_length,
            _process_pool_end,      0xffffffff - _process_pool_end);
        dbg("msg: %s\n", buffer.getData());
    } break;
    default:
        buffer.clear();
    }
}

void Debugger::handleCommand_zZ() {
                        // or  ... & 0x20 -> uppercase
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
    // This code is pretty bad, it works, I won't touch it further

    char* const str = strchr(buffer.getData(), ';');
    if (str == NULL) {
        buffer.setReturnCode(FAIL);
        return;
    }

    // Starting from first character of the buffer after ; (vRun;2f62696e2f666f6f;626172)
    //                                                           ^
    // Decode to                                           (vRun\0/bin/foo\0bar\0)

    // Decode
    // If character is a semi ';' terminate string '\0'
    // Transform hex pairs into characters 2f62696e -> "/bin"
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

    // Create argv
    char** args = new char*[argsNum+1];
    char* ptr = str+1;
    args[argsNum] = nullptr;
    for (int i = 0; i < argsNum; i++) {
        args[i] = ptr;
        ptr += strlen(ptr) + 1;
    }

    char** a = args;
    while(*a) {
        printf("\"%s\" ", *a);
        a++;
    }
    printf("\n");

    // posix_spawn cannot be modified:
    // In posix_spawn implementation: check if Debugger::thread == currentThread
    int pid = 0,ec = 0;
    ec = posix_spawn(&pid,args[0],nullptr,nullptr,args, nullptr);
    if (ec != 0) {
        printf("ec: %d\n", ec);
        buffer.setReturnCode(GDBReturnCode::SPAWN_FAIL);
        return;
    }

    // If process creation completes successfully: attached.process is set
    delete[] args;

    // Process::create sets pending event for thread of created process
    waitAttached();
    stopReply();
}

void Debugger::vattach() {

    if (attached.process != nullptr) {
        // Single process mode: you should detach from a previous process before
        // attaching to a new one
        buffer.setReturnCode(GDBReturnCode::ATTACH_FAIL);
        return;
    }

    char* const str = strchr(buffer.getData(), ';');
    const auto pid = strtoul(str + 1, nullptr, 16);

    buffer.clear();

    // Fancy quasi-static method (cannot make it fully static, needs to access ProcessTable)
    Process* proc = attached.process->debugGetByPid(pid);
    // Provided pid is not valid
    if (proc == nullptr) {
        buffer.setReturnCode(GDBReturnCode::ATTACH_FAIL);
        return;
    }

    bool priv = proc->isPrivate();

    if (proc->program.isCopiedInRam() &&
            !proc->tryMakePrivate()) {
        // gdb may attempt software breakpoints, but process code is shared, forbid
        BUF_FORMAT(buffer,
                "E.Code section is shared and cannot use hardware breakpoints");
        return;
    }

    {
        FastGlobalIrqLock dLock;

        Thread* th = nullptr;

        // TODO: check if logic to determine live threads is correct
        for (const auto& thread : proc->threads) {
            if (thread->flags.isDeleted() == false
            && thread->flags.isDeleting() == false) {
                th = thread;
                break;
            }
        }

        if(th == nullptr) {
            FastGlobalIrqUnlock u(dLock);
            // Fail if no thread is alive in the selected process
            buffer.setReturnCode(GDBReturnCode::ATTACH_FAIL);
            // If previously was private, keep it private
            // This is the only way an entry can be assigned shared again, to prevent
            // sharing modified code on debugger detach, can only revert to its
            // original value if attach fails
            if(!priv) proc->makeShared();
            return;
        }

        attached.process                    = proc;
        attached.debugState                 = proc->IRQdebugState();

        if (attached.debugState)
        {
            // Thread stopped before the attach, there is no way to know the reason
            // Report generic debugevent
            attached.event.reason = StopReason::DEBUGEVENT;
            attached.thread = th;
        }
        else
        {
            // Process not in debugstate, make thread pending
            attached.event.reason           = StopReason::NONE;
            th->debugStatus                 = DebugStatus::PEND;
            th->IRQwakeup();
        }
    }

    waitAttached();
    stopReply();
}

// TODO: bad code
// Fancy comparators for message parsing:
// Negated stringcompare
#define strEq(ptr, str)         (!strcmp(ptr, str))
// Negated stringcompare up to length of str (excluded \0)
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
