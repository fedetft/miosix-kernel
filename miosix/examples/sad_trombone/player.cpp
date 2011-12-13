/***************************************************************************
 *   Copyright (C) 2011 by Terraneo Federico                               *
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

#include <algorithm>
#include "miosix/kernel/scheduler/scheduler.h"
#include "miosix/kernel/buffer_queue.h"
#include "adpcm.h"
#include "player.h"

using namespace miosix;

typedef Gpio<GPIOA_BASE,4> dacPin; //DAC1 out on the stm32f100rb is PA4

static const int bufferSize=256; //Buffer RAM is 4*bufferSize bytes
static Thread *waiting;
static BufferQueue<unsigned short,bufferSize> bq;
static bool enobuf=true;

/**
 * Configure the DMA to do another transfer
 */
static void IRQdmaRefill()
{
	const unsigned short *buffer;
	unsigned int size;
	if(bq.IRQgetReadableBuffer(buffer,size)==false)
	{
		enobuf=true;
		return;
	}
	DMA1_Channel3->CCR=0;
	DMA1_Channel3->CPAR=reinterpret_cast<unsigned int>(&DAC->DHR12L1);
	DMA1_Channel3->CMAR=reinterpret_cast<unsigned int>(buffer);
	DMA1_Channel3->CNDTR=size;
	DMA1_Channel3->CCR=DMA_CCR3_MSIZE_0 | //Read 16 bit from memory
					   DMA_CCR3_PSIZE_0 | //Write 16 bit to peripheral
						  DMA_CCR3_MINC | //Increment memory pointer
						   DMA_CCR3_DIR | //Transfer memory to peripheral
						  DMA_CCR3_TEIE | //Interrupt on error
						  DMA_CCR3_TCIE | //Interrupt on transfer complete
						    DMA_CCR3_EN;  //Start DMA    
}

/**
 * Helper function used to start a DMA transfer from non-interrupt code
 */
static void dmaRefill()
{
	FastInterruptDisableLock dLock;
	IRQdmaRefill();
}

/**
 * DMA end of transfer interrupt
 */
void __attribute__((naked)) DMA1_Channel3_IRQHandler()
{
	saveContext();
	asm volatile("bl _Z17DACdmaHandlerImplv");
	restoreContext();
}

/**
 * DMA end of transfer interrupt actual implementation
 */
void __attribute__((used)) DACdmaHandlerImpl()
{
	DMA1->IFCR=DMA_IFCR_CGIF3;
	bq.IRQbufferEmptied();
	IRQdmaRefill();
	waiting->IRQwakeup();
	if(waiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
		Scheduler::IRQfindNextThread();
}

/**
 * This function allows to atomically check if a variable equals a specific
 * value and if not, put the thread in wait state until said condition is
 * satisfied. To wake the thread, another thread or an interrupt routine
 * should first set the variable to the desired value, and then call
 * wakeup() (or IRQwakeup()) on the sleeping thread.
 * \param T type of the variable to test
 * \param variable the variable to test
 * \param value the value that will cause this function to return.
 */
template<typename T>
static void atomicTestAndWaitUntil(volatile T& variable, T value)
{
	FastInterruptDisableLock dLock;
	while(variable!=value)
	{
		Thread::IRQgetCurrentThread()->IRQwait();
		{
			FastInterruptEnableLock eLock(dLock);
			Thread::yield();
		}
	}
}

/**
 * Helper function that waits until a buffer is available for writing
 * \return a writable buffer from bq
 */
static unsigned short *getWritableBuffer()
{
	FastInterruptDisableLock dLock;
	unsigned short *result;
	while(bq.IRQgetWritableBuffer(result)==false)
	{
		waiting->IRQwait();
		{
			FastInterruptEnableLock eLock(dLock);
			Thread::yield();
		}
	}
	return result;
}

/**
 * Helper function called after a buffer is filled
 */
static void bufferFilled()
{
	FastInterruptDisableLock dLock;
	bq.IRQbufferFilled(bq.bufferMaxSize);
}

//
// class Sound
//

Sound::~Sound() {}

//
// class ADPCMSound
//

bool ADPCMSound::fillBuffer(unsigned short *buffer, int size)
{
	int remaining=soundSize-index;
	int length=std::min(size,remaining*2);
	for(int i=0;i<length;i+=2)
	{
		unsigned char in=soundData[index++];
		buffer[i+0]=ADPCM_Decode(in & 0xf)+0x8000;
		buffer[i+1]=ADPCM_Decode(in>>4)+0x8000;
	}
	if(index<soundSize) return false;
	for(int i=length;i<size;i++) buffer[i]=0x8000;
	return true;
}

void ADPCMSound::rewind() { index=0; }

//
// class Player
//

Player& Player::instance()
{
	static Player singleton;
	return singleton;
}

void Player::play(Sound& sound)
{
	Lock<Mutex> l(mutex);

	//Configure GPIOs
	dacPin::mode(Mode::INPUT_ANALOG);

	{
		FastInterruptDisableLock dLock;
		//Enable peripherals clock gating, other threads might be concurretly
		//using these registers, so modify them in a critical section
		RCC->AHBENR |= RCC_AHBENR_DMA1EN;
		RCC->APB1ENR |= RCC_APB1ENR_DACEN | RCC_APB1ENR_TIM6EN;
	}

	//Configure DAC
	DAC->CR=DAC_CR_DMAEN1 | DAC_CR_TEN1 | DAC_CR_EN1;

	//Configure DMA
	NVIC_SetPriority(DMA1_Channel3_IRQn,2);//High priority for DMA
	NVIC_EnableIRQ(DMA1_Channel3_IRQn);

	//Configure TIM6
	DBGMCU->CR |= DBGMCU_CR_DBG_TIM6_STOP; //TIM6 stops while debugging
	TIM6->CR1=0; //Upcounter, not started, no special options
	TIM6->CR2=TIM_CR2_MMS_1; //Update evant is output as TRGO, used by the DAC
	TIM6->PSC=0;
	TIM6->ARR=(24000000/44100)-1;
	TIM6->CR1|=TIM_CR1_CEN;

	//Antibump, rise slowly to 0x8000
	unsigned short *buffer=getWritableBuffer();
	for(int i=0;i<bufferSize;i++) buffer[i]=(0x8000/(2*bufferSize))*i;
	bufferFilled();
	buffer=getWritableBuffer();
	for(int i=0;i<bufferSize;i++) buffer[i]=(0x8000/(2*bufferSize))*i+0x4000;
	bufferFilled();

	//Start playing
	sound.rewind();
	waiting=Thread::getCurrentThread();
	for(;;)
	{
		if(enobuf)
		{
			enobuf=false;
			dmaRefill();
		}
		if(sound.fillBuffer(getWritableBuffer(),bufferSize)) break;
		bufferFilled();
	}
	atomicTestAndWaitUntil(enobuf,true); //Play last buffer

	//Shutdown
	NVIC_DisableIRQ(DMA1_Channel3_IRQn);
	TIM6->CR1=0;
	DMA1_Channel3->CCR=0;
	DAC->CR=0;
	dacPin::mode(Mode::INPUT_PULL_UP_DOWN);
	dacPin::pulldown();
}

bool Player::isPlaying() const
{
	if(mutex.tryLock()==false) return true;
	mutex.unlock();
	return false;
}

Player::Player() {}
