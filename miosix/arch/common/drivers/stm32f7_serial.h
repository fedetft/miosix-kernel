/***************************************************************************
 *   Copyright (C) 2010-2018 by Terraneo Federico                          *
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

#pragma once

#include "filesystem/console/console_device.h"
#include "kernel/sync.h"
#include "kernel/queue.h"
#include "interfaces/gpio.h"
#include "board_settings.h"

namespace miosix {

class STM32SerialHW;
class STM32Serial;
class STM32DmaSerial;

/**
 * \internal Common code for DMA and non-DMA implementations of the STM32 serial
 * port class.
 */
class STM32SerialBase
{
public:
    /**
     * Utility factory method for crating an instance of the STM32 serial
     * drivers.
     * 
     * Calls errorHandler(Error::UNEXPECTED) if id is not correct range or if DMA
     * operation is requested and the specified port is not supported.
     * \tparam Tx Output GPIO
     * \tparam Rx Input GPIO
     * \tparam Rts Request to send GPIO (used only for flow control)
     * \tparam Cts Clear to send GPIO (used only for flow control)
     * \param id A number to select which USART. The maximum id depends on the
     *           specific microcontroller, the minimum id is always 1
     * \param speed Serial port baudrate
     * \param flowctrl True to enable flow control
     * \param dma True to enable DMA operation
     */
    template<typename Tx, typename Rx, typename Rts, typename Cts>
    static inline intrusive_ref_ptr<Device> get(
        unsigned int id, unsigned int speed, bool flowctrl, bool dma);
    
    /**
     * \return port id, 1 for USART1, 2 for USART2, ... 
     */
    int getId() const { return portId; }

private:
    /**
     * Constructor, does not initialize the peripheral
     */
    STM32SerialBase(int id, int baudrate, bool flowControl);

    /**
     * Initialize GPIOs, baud rate and flow control
     */
    void commonInit(int id, int baudrate, GpioPin tx, GpioPin rx,
                    GpioPin rts, GpioPin cts);
    
    /**
     * Write to the serial port directly. Interrupts must be already disabled.
     */
    void IRQwrite(const char *str);

    /**
     * Wait until all characters have been written to the serial port.
     * Needs to be callable from interrupts disabled (it is used in IRQwrite)
     */
    void waitSerialTxFifoEmpty();

    /**
     * Read a block of data from rxQueue. Stops when the maximum size is reached
     * or if the line becomes idle.
     */
    ssize_t readFromRxQueue(void *buffer, size_t size);

    /**
     * Update if the line is idle or not from the value of the USART
     * status register.
     */
    inline void rxUpdateIdle(unsigned long sr)
    {
        idle=!!(sr & USART_ISR_IDLE);
    }

    /**
     * Put a value in the rxQueue.
     */
    inline bool rxQueuePut(char c)
    {
        return rxQueue.tryPut(c);
    }

    /**
     * Wakeup a suspended readFromRxQueue() after a state change of rxQueue or
     * line idleness status.
     */
    void rxWakeup();

    /**
     * Common implementation of ioctl() for the STM32 serial
     */
    int ioctl(int cmd, void* arg);

    friend class STM32Serial;
    friend class STM32DmaSerial;

    const STM32SerialHW *port;        ///< Pointer to USART port object
    const bool flowControl;           ///< True if flow control GPIOs enabled
    const unsigned char portId;       ///< 1 for USART1, 2 for USART2, ...

    KernelMutex rxMutex;              ///< Mutex locked during reception
    DynUnsyncQueue<char> rxQueue;     ///< Receiving queue
    static const unsigned int rxQueueMin=16; ///< Minimum queue size
    Thread *rxWaiting=nullptr;        ///< Thread waiting for rx, or 0
    bool idle=true;                   ///< Receiver idle
};

/**
 * Serial port class for stm32 microcontrollers.
 * 
 * Classes of this type are reference counted, must be allocated on the heap
 * and managed through intrusive_ref_ptr<FileBase>
 */
class STM32Serial : public STM32SerialBase, public Device
{
public:
    /**
     * Constructor, initializes the serial port using remapped pins and disables
     * flow control.
     * 
     * Calls errorHandler(Error::UNEXPECTED) if id is not in the correct range.
     * \param id a number to select which USART. The maximum id depends on the
     *           specific microcontroller, the minimum id is always 1.
     * \param baudrate serial port baudrate
     * \param tx tx pin
     * \param rx rx pin
     */
    STM32Serial(int id, int baudrate, GpioPin tx, GpioPin rx);
    
    /**
     * Constructor, initializes the serial port using remapped pins and enables
     * flow control.
     * 
     * Calls errorHandler(Error::UNEXPECTED) if id is not in the correct range.
     * \param id a number to select which USART. The maximum id depends on the
     *           specific microcontroller, the minimum id is always 1.
     * \param tx tx pin
     * \param rx rx pin
     * \param rts rts pin
     * \param cts cts pin
     */
    STM32Serial(int id, int baudrate, GpioPin tx, GpioPin rx,
                GpioPin rts, GpioPin cts);
    
    /**
     * Read a block of data
     * \param buffer buffer where read data will be stored
     * \param size buffer size
     * \param where where to read from
     * \return number of bytes read or a negative number on failure. Note that
     * it is normal for this function to return less character than the amount
     * asked
     */
    ssize_t readBlock(void *buffer, size_t size, off_t where);
    
    /**
     * Write a block of data
     * \param buffer buffer where take data to write
     * \param size buffer size
     * \param where where to write to
     * \return number of bytes written or a negative number on failure
     */
    ssize_t writeBlock(const void *buffer, size_t size, off_t where);
    
    /**
     * Write a string.
     * An extension to the Device interface that adds a new member function,
     * which is used by the kernel on console devices to write debug information
     * before the kernel is started or in case of serious errors, right before
     * rebooting.
     * \param str the string to write. The string must be NUL terminated.
     */
    void IRQwrite(const char *str);
    
    /**
     * Performs device-specific operations
     * \param cmd specifies the operation to perform
     * \param arg optional argument that some operation require
     * \return the exact return value depends on CMD, -1 is returned on error
     */
    int ioctl(int cmd, void *arg)
    {
        return STM32SerialBase::ioctl(cmd, arg);
    }
    
    /**
     * Destructor
     */
    ~STM32Serial();
    
private:
    /**
     * Code common for all constructors
     */
    void commonInit(int id, int baudrate, GpioPin tx, GpioPin rx,
                    GpioPin rts, GpioPin cts);

    /**
     * \internal the serial port interrupts call this member function.
     * Never call this from user code.
     */
    void IRQhandleInterrupt();

    KernelMutex txMutex;              ///< Mutex locked during transmission
};

/**
 * Serial port class for stm32 microcontrollers which uses DMA for data
 * transfer.
 * 
 * Classes of this type are reference counted, must be allocated on the heap
 * and managed through intrusive_ref_ptr<FileBase>
 */
class STM32DmaSerial : public STM32SerialBase, public Device
{
public:
    /**
     * Constructor, initializes the serial port using remapped pins and disables
     * flow control.
     * 
     * Calls errorHandler(Error::UNEXPECTED) if id is not in the correct range, or if
     * there is no support for DMA operation for this port.
     * \param id a number to select which USART. The maximum id depends on the
     *           specific microcontroller, the minimum id is always 1.
     * \param baudrate serial port baudrate
     * \param tx tx pin
     * \param rx rx pin
     */
    STM32DmaSerial(int id, int baudrate, GpioPin tx, GpioPin rx);
    
    /**
     * Constructor, initializes the serial port using remapped pins and enables
     * flow control.
     * 
     * Calls errorHandler(Error::UNEXPECTED) if id is not in the correct range, or if
     * there is no support for DMA operation for this port.
     * \param id a number to select which USART. The maximum id depends on the
     *           specific microcontroller, the minimum id is always 1.
     * \param tx tx pin
     * \param rx rx pin
     * \param rts rts pin
     * \param cts cts pin
     */
    STM32DmaSerial(int id, int baudrate, GpioPin tx, GpioPin rx,
                GpioPin rts, GpioPin cts);
    
    /**
     * Read a block of data
     * \param buffer buffer where read data will be stored
     * \param size buffer size
     * \param where where to read from
     * \return number of bytes read or a negative number on failure. Note that
     * it is normal for this function to return less character than the amount
     * asked
     */
    ssize_t readBlock(void *buffer, size_t size, off_t where);
    
    /**
     * Write a block of data
     * \param buffer buffer where take data to write
     * \param size buffer size
     * \param where where to write to
     * \return number of bytes written or a negative number on failure
     */
    ssize_t writeBlock(const void *buffer, size_t size, off_t where);
    
    /**
     * Write a string.
     * An extension to the Device interface that adds a new member function,
     * which is used by the kernel on console devices to write debug information
     * before the kernel is started or in case of serious errors, right before
     * rebooting.
     * \param str the string to write. The string must be NUL terminated.
     */
    void IRQwrite(const char *str);
    
    /**
     * Performs device-specific operations
     * \param cmd specifies the operation to perform
     * \param arg optional argument that some operation require
     * \return the exact return value depends on CMD, -1 is returned on error
     */
    int ioctl(int cmd, void *arg)
    {
        return STM32SerialBase::ioctl(cmd, arg);
    }
    
    /**
     * Destructor
     */
    ~STM32DmaSerial();
    
private:
    /**
     * Code common for all constructors
     */
    void commonInit(int id, int baudrate, GpioPin tx, GpioPin rx,
                    GpioPin rts, GpioPin cts);

    /**
     * Start a write to the serial port using DMA. The write is asynchronous
     * (when the function returns, the transfer is still in progress)
     * \param buffer buffer to write
     * \param size size of buffer to write
     */
    void startDmaWrite(const char *buffer, size_t size);

    /**
     * Wait until a pending DMA TX completes, if any
     */
    void waitDmaWriteEnd();
    
    /**
     * Start asynchronously reading from the serial port using DMA into
     * rxBuffer.
     */
    void IRQstartDmaRead();

    /**
     * The DMA write stream interrupt calls this member function.
     * Never call this from user code.
     */
    void IRQhandleDmaTxInterrupt();
    
    /**
     * Stop DMA read into rxBuffer
     * \return the number of characters in rxBuffer
     */
    int IRQstopDmaRead();
    
    /**
     * Moves the entire content of rxBuffer into rxQueue for reading out by
     * the application thread.
     */
    void IRQflushDmaReadBuffer();

    /**
     * The DMA read stream interrupt calls this member function.
     * Never call this from user code.
     */
    void IRQhandleDmaRxInterrupt();

    /**
     * The serial port interrupt calls this member function.
     * Never call this from user code.
     */
    void IRQhandleInterrupt();

    /**
     * The STM3F3, STM32F4 and STM32L4 have an ugly quirk of having 64KB RAM area
     * called CCM that can only be accessed by the processor and not be the DMA.
     * \param x pointer to check
     * \return true if the pointer is inside the CCM, and thus it isn't possible
     * to use it for DMA transfers
     */
    static bool isInCCMarea(const void *x)
    {
        unsigned int ptr=reinterpret_cast<const unsigned int>(x);
        return (ptr>=0x10000000) && (ptr<(0x10000000+64*1024));
    }

    KernelMutex txMutex;              ///< Mutex locked during transmission

    Thread *txWaiting=nullptr;        ///< Thread waiting for tx, or 0
    static const unsigned int txBufferSize=16; ///< Size of DMA tx buffer
    /// Tx buffer, for tx speedup. This buffer must not end up in the CCM of the
    /// STM32F4, as it is used to perform DMA operations. This is guaranteed by
    /// the fact that this class must be allocated on the heap as it derives
    /// from Device, and the Miosix linker scripts never puts the heap in CCM
    char txBuffer[txBufferSize];
    /// This buffer emulates the behaviour of a 16550. It is filled using DMA
    /// and an interrupt is fired as soon as it is half full
    char rxBuffer[rxQueueMin];
    bool dmaTxInProgress=false;        ///< True if a DMA tx is in progress
};

template<typename Tx, typename Rx, typename Rts, typename Cts>
intrusive_ref_ptr<Device> STM32SerialBase::get(
    unsigned int id, unsigned int speed, bool flowctrl, bool dma)
{
    if(!flowctrl&&!dma)
        return intrusive_ref_ptr<Device>(new STM32Serial(id,speed,
            Tx::getPin(),Rx::getPin()));
    else if(!flowctrl&&dma)
        return intrusive_ref_ptr<Device>(new STM32DmaSerial(id,speed,
            Tx::getPin(),Rx::getPin()));
    else if(flowctrl&&!dma)
        return intrusive_ref_ptr<Device>(new STM32Serial(id,speed,
            Tx::getPin(),Rx::getPin(),Rts::getPin(),Cts::getPin()));
    else //if(flowctrl&&dma)
        return intrusive_ref_ptr<Device>(new STM32DmaSerial(id,speed,
            Tx::getPin(),Rx::getPin(),Rts::getPin(),Cts::getPin()));
}

} //namespace miosix
