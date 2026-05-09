#pragma once

#include "debugger_interface.h"

namespace miosix {

class RegisterFile {
public:

    /**
     * @brief Copy content of register i into ref
     *
     * Destination must be big enough to store the register, register size can
     * be queried with registerSize
     *
     * @param i number of register to read
     * @param ref to store value
     * @return true if valid read, false otherwise
     */
    bool read(int i, char* ref);

    /**
     * @brief Writes the value pinted by ref into register i
     *
     * The number of bytes written depends on register size
     *
     * @param i number of register to write
     * @param value value to write
     * @return true if valid write, false otherwise
     */
    bool write(int i, char* ref);

    /**
     * @brief Return the size of register i in bytes
     *
     * @param i Index of register in register file
     */
    int getSize(int i);

    // Number of entries in the register file
    const static int entries            = REGISTER_FILE_ENTRIES;
    
    // Total size of the register file in bytes
    const static int sizeBytes          = REGISTER_FILE_SIZE_BYTES;

    // Maximum size of a register in bytes
    const static int maxRegSizeBytes    = MAX_REGISTER_SIZE_BYTES;
};

typedef enum {
    OK = 0,
    SPAWN_FAIL,
    MEMORY_WRITE_FAIL,
    MEMORY_READ_FAIL,
    REGISTER_READ_FAIL,
    REGISTER_WRITE_FAIL,
    BREAKPOINT_SET_FAIL,
    BREAKPOINT_FULL,
    BREAKPOINT_TYPE_NOT_SUPPORTED,
} GDBReturnCode;

class GDBBuffer {
public:

    GDBBuffer() { clear(); }

    /**
     * @brief Computes chesksum of the buffer's content
     */
    inline const unsigned char checksum() const {
        unsigned char sum = 0;
        for (unsigned int i = 0; i < head; i++)
            sum += (unsigned char)data[i];
        return sum;
    };

    inline void clear() { head = 0; data[head] = '\0'; }

    /**
     * @brief Moves the head of buffer to "len" and null-terminates the string
     *
     * @param len 
     * @return the new position of head on success, -1 on fail
     */
    int setLen(unsigned int len) {
        if (size <= len) return -1;
        head = len;
        data[head] = '\0';
        return len;
    }

    /**
     * @brief Appends character to the buffer followed by '\0'
     *
     * @param character 
     * @returns the nubmer of charactesrs written on success, 0 on fail
     */
    int appendChar(char character);

    /**
     * @brief Appends a null string to the buffer
     *
     * @param str 
     * @param len 
     * @returns the nubmer of charactesrs written on success, 0 on fail
     */
    int appendString(char* str, unsigned int len);

    /**
     * @brief Set the output buffer as one of the GDBReturnCode messages
     *
     * @param code 
     * @returns the nubmer of charactesrs written on success, 0 on fail
     */
    int setReturnCode(GDBReturnCode code);

    /**
     * @brief Appends the value in the output buffer, null terminated
     *
     * @param value 
     * @returns the nubmer of charactesrs written on success, 0 on fail
     */
    int appendRegister32(unsigned int value);

    /**
     * @brief Appends the value in the output buffer, null terminated
     *
     * @param value 
     * @returns the nubmer of charactesrs written on success, 0 on fail
     */
    int appendRegister64(unsigned long long value);

    /**
     * @brief Clears the buffer and append `bytes` bytes to the output buffer
     *
     * Prints the bytes from low to high addresses, each byte is represented as
     * two hex characters, padding with 0 if the byte is shorter than two characters.
     *
     * @param addr beginning address of bytes in memory
     * @param bytes number of bytes to append
     * @returns the length of the appended string, 0 on fail
     */
    int setBytes(unsigned int* addr, unsigned int bytes);

    int setComment(char comment[]);

    /**
     * @brief Returns the number of bytes still available
     *
     * @return 
     */
    const unsigned int available() {return size - head; }

    /**
     * @brief Returns the content of the buffer
     *
     * @return 
     */
    inline char* getData() { return data; }

    /**
     * @brief Returns the length of buffer's content
     *
     * @return 
     */
    inline const unsigned int len() const {return head; }
    
    // Must have enough space to fit Register filein a single message
    // (size of registerfile) * 2 (hex encoding) + 1 (null terminator) 
    static const unsigned int size = RegisterFile::sizeBytes * 2 + 1;

private:
    char         data[size];
    unsigned int head;

    static_assert(size > RegisterFile::sizeBytes * 2,
        "StubBuffers must be large enough to store all registers with hex encoding (2 bytes per register)");
};

typedef enum {
    // TODO: consider starting @ exit
    NONE,                   // No process is running yet, for extended mode
    DEBUGEVENT,             // Process stopped due to debug event,
    EXIT,                   // Process exited normally, code is return value
    FAULT,                  // Thread terminated, code is the fault reason
    EXECVE,                 // Process called execve: preserved in
                            // DebugMon_Handler
} StopReason;

/**
 * @class AttachedProcessInfo
 * @brief Contains information about the process currently being debugged
 *
 * Used in communication between Process & Thread class, DebugMon_Handler and
 * Debugger class
 *
 */
class AttachedProcessInfo {
public:

    char*           name    = nullptr;
    pid_t           pid     = 0;
    int             ec;
    unsigned int    tid     = 0;
    unsigned int    code    = 0;
    StopReason      reason  = NONE;

    // FIXME: Move to Thread.h
    //  - private
    //  - friend class Debugger
    DebugStatus status   =  DebugStatus::STOP;

    void clear() {
        name        = nullptr;
        pid         = 0;
        tid         = 0;
        code        = 0;
        reason      = NONE;
        // FIXME: for all threads in process
        status      = DebugStatus::STOP;
    }
};

/**
 * @class VMessage
 * @brief Tag for 'v' messages
 *
 */
class VMessage {
public:
    enum Type {
        VRUN,
        NONE,
    } type = NONE;
    char* args;
};

/**
 * @class QMessage
 * @brief Tag for 'q' messages
 *
 */
struct QMessage {
public:
    enum Type {
        SUPPORTED,
        OFFSETS,
        MEMORY_MAP,
        FEATURES,
        NONE,
    } type = NONE;
    unsigned int offset, length;

    /**
     * @brief Restricts qMessage's offset and length to a valid view of str
     *
     * @param size 
     * @return true if more data is available, false otherwise
     */
    inline bool xmlView(unsigned int size) {
        if (offset > size) {
            length = 0; return false;
        }
        if (offset + length > size) {
            length = size - offset; return false;
        }
        return true;
    }
};

/**
 * @class GDBFeatures
 * @brief Flags for supported GDB features
 *
 */
class GDBFeatures {
public:
    typedef enum : unsigned int {
        EXEC_EVENTS,
    } Name;

    /**
     * @brief Set featurea as supported
     *
     * @param feat 
     */
    void set(Name feat) { features |= (1 << feat); }

    /**
     * @brief Check if feature is supported
     *
     * @param feat 
     * @return 
     */
    bool supported(Name feat) { return features & (1 << feat); }

private:
    unsigned int features = 0;
};

/**
 * @class Debugger
 * @brief Handles communication with GDB
 *
 */
class Debugger {
public:

    void listen(char* serialName);

    static inline void fail() { failed = true; }

private:

    // FileNo forcommunication with GDB
    int serial = -1;
    
    // To start and stop process
    GDBBuffer buffer;
    GDBFeatures features;

    // Share information between Debugger and other modules
    static AttachedProcessInfo attached;
    // TODO: make fully static
    static RegisterFile registerFile;

    // FIXME: BreakpointUnit in separated class, make all static, IRQsyncLocal
    // takes thread as arg
    static BreakpointUnit fpb;

    static bool failed;

    inline void extendedMode() { buffer.setReturnCode(OK); }

    void recvPacket();
    void sendPacket();

    // Handle specific commands
    void handleCommand();
    void handleCommand_gG();
    void handleCommand_mM();
    void handleCommand_cs();
    void handleCommand_pP();
    void handleCommand_v();
    void handleCommand_q();
    void handleCommand_zZ();

    void stopReply();

    void vrun();
    void parsePacket_v(VMessage* vMessage);
    void parsePacket_q(QMessage* qMessage);
    
};

}
