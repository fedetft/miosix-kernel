/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012, 2013, 2014 by Terraneo Federico       *
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

#ifndef SERIAL_STM32_H
#define	SERIAL_STM32_H

#include "filesystem/console/console_device.h"
#include "kernel/sync.h"
#include "kernel/queue.h"
#include "board_settings.h"

#if defined(_ARCH_CORTEXM3_STM32) && defined(__ENABLE_XRAM)
//Quirk: concurrent access to the FSMC from both core and DMA is broken in
//the stm32f1, so disable DMA mode if XRAM is enabled.
#undef SERIAL_1_DMA
#undef SERIAL_2_DMA
#undef SERIAL_3_DMA
#endif

#if defined(SERIAL_1_DMA) || defined(SERIAL_2_DMA) || defined(SERIAL_3_DMA)
#define SERIAL_DMA
#endif

namespace miosix {

/**
 * Serial port class for stm32 microcontrollers.
 * Only supports USART1, USART2 and USART3
 * Additionally, USARTx can use DMA if SERIAL_x_DMA is defined in
 * board_settings.h, while the other serial use polling for transmission,
 * and interrupt for reception.
 * 
 * Classes of this type are reference counted, must be allocated on the heap
 * and managed through intrusive_ref_ptr<FileBase>
 */
class STM32Serial : public Device
{
public:
    enum FlowCtrl
    {
        NOFLOWCTRL, ///< No hardware flow control
        RTSCTS      ///< RTS/CTS hardware flow control
    };
    
    /**
     * Constructor, initializes the serial port.
     * Calls errorHandler(UNEXPECTED) if id is not in the correct range, or when
     * attempting to construct multiple objects with the same id. That is,
     * it is possible to instantiate only one instance of this class for each
     * hardware USART.
     * \param id a number 1 to 3 to select which USART
     * \param baudrate serial port baudrate
     * \param flowControl to enable hardware flow control on this port
     */
    STM32Serial(int id, int baudrate, FlowCtrl flowControl=NOFLOWCTRL);
    
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
     * Can ONLY be called when the kernel is not yet started, paused or within
     * an interrupt. This default implementation ignores writes.
     * \param str the string to write. The string must be NUL terminated.
     */
    void IRQwrite(const char *str);
    
    /**
     * Performs device-specific operations
     * \param cmd specifies the operation to perform
     * \param arg optional argument that some operation require
     * \return the exact return value depends on CMD, -1 is returned on error
     */
    int ioctl(int cmd, void *arg);
    
    /**
     * \internal the serial port interrupts call this member function.
     * Never call this from user code.
     */
    void IRQhandleInterrupt();
    
    #ifdef SERIAL_DMA
    /**
     * \internal the serial port DMA tx interrupts call this member function.
     * Never call this from user code.
     */
    void IRQhandleDMAtx();
    
    /**
     * \internal the serial port DMA rx interrupts call this member function.
     * Never call this from user code.
     */
    void IRQhandleDMArx();
    #endif //SERIAL_DMA
    
    /**
     * \return port id, 1 for USART1, 2 for USART2, ... 
     */
    int getId() const { return portId; }
    
    /**
     * Destructor
     */
    ~STM32Serial();
    
private:
    #ifdef SERIAL_DMA
    /**
     * Wait until a pending DMA TX completes, if any
     */
    void waitDmaTxCompletion();
    
    /**
     * Write to the serial port using DMA. When the function returns, the DMA
     * transfer is still in progress.
     * \param buffer buffer to write
     * \param size size of buffer to write
     */
    void writeDma(const char *buffer, size_t size);
    
    /**
     * Read from DMA buffer and write data to queue
     */
    void IRQreadDma();
    
    /**
     * Start DMA read
     */
    void IRQdmaReadStart();
    
    /**
     * Stop DMA read
     * \return the number of characters in rxBuffer
     */
    int IRQdmaReadStop();
    #endif //SERIAL_DMA
    
    /**
     * Wait until all characters have been written to the serial port
     */
    void waitSerialTxFifoEmpty()
    {
        while((port->SR & USART_SR_TC)==0) ;
    }

    FastMutex txMutex;                ///< Mutex locked during transmission
    FastMutex rxMutex;                ///< Mutex locked during reception
    
    DynUnsyncQueue<char> rxQueue;     ///< Receiving queue
    static const unsigned int rxQueueMin=16; ///< Minimum queue size
    Thread *rxWaiting;                ///< Thread waiting for rx, or 0
    
    USART_TypeDef *port;              ///< Pointer to USART peripheral
    #ifdef SERIAL_DMA
    #ifdef _ARCH_CORTEXM3_STM32
    DMA_Channel_TypeDef *dmaTx;       ///< Pointer to DMA TX peripheral
    DMA_Channel_TypeDef *dmaRx;       ///< Pointer to DMA RX peripheral
    #else //_ARCH_CORTEXM3_STM32
    DMA_Stream_TypeDef *dmaTx;        ///< Pointer to DMA TX peripheral
    DMA_Stream_TypeDef *dmaRx;        ///< Pointer to DMA RX peripheral
    #endif //_ARCH_CORTEXM3_STM32
    Thread *txWaiting;                ///< Thread waiting for tx, or 0
    static const unsigned int txBufferSize=16; ///< Size of tx buffer, for tx speedup
    /// Tx buffer, for tx speedup. This buffer must not end up in the CCM of the
    /// STM32F4, as it is used to perform DMA operations. This is guaranteed by
    /// the fact that this class must be allocated on the heap as it derives
    /// from Device, and the Miosix linker scripts never put the heap in CCM
    char txBuffer[txBufferSize];
    /// This buffer emulates the behaviour of a 16550. It is filled using DMA
    /// and an interrupt is fired as soon as it is half full
    char rxBuffer[rxQueueMin];
    bool dmaTxInProgress;             ///< True if a DMA tx is in progress
    #endif //SERIAL_DMA
    bool idle;                        ///< Receiver idle
    const bool flowControl;           ///< True if flow control GPIOs enabled
    const unsigned char portId;       ///< 1 for USART1, 2 for USART2, ...
};

} //namespace miosix

#endif //SERIAL_STM32_H
