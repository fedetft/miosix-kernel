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

#include "miosix.h"

#ifndef PLAYER_H
#define PLAYER_H

/**
 * Interface class from where all sound classes derive
 */
class Sound
{
public:
	/**
	 * Fill a buffer with audio samples
	 * \param buffer a buffer where audio samples (16bit unsigned, 44100Hz)
	 * are to be stored. If there is not enough data to fill the entire buffer
	 * the remaining part must be filled with 0
	 * \param length buffer length, must be divisible by two
	 * \return true if this is the last valif buffer (eof encountered)
	 */
	virtual bool fillMonoBuffer(unsigned short *buffer, int length)=0;
    
    /**
	 * Fill a stereo buffer with audio samples
	 * \param buffer a buffer where audio samples (16bit unsigned, 44100Hz)
	 * are to be stored. If there is not enough data to fill the entire buffer.
     * The buffer format is alternating left-right samples, so buffer[0] is left
     * buffer[1] is right, buffer[2] is again left...
	 * the remaining part must be filled with 0
	 * \param length buffer length, must be divisible by four
	 * \return true if this is the last valif buffer (eof encountered)
	 */
	virtual bool fillStereoBuffer(unsigned short *buffer, int length)=0;

	/**
	 * Rewind the internal sound pointer so that succesive calls to
	 * fillBuffer() start brom the beginning of the sound.
	 */
	virtual void rewind()=0;

	/**
	 * Destructor
	 */
	virtual ~Sound();
};

/**
 * Class to play a buffer contatinig ADPCM compressed audio
 */
class ADPCMSound : public Sound
{
public:
	/**
	 * Constructor
	 * \param data ADPCM encoded data. Ownership of the buffer remains
	 * of the caller, which is responsible to make sure it remains valid
	 * for the entire lifetime of this class. This is not a problem in
	 * the expected use case of the buffer being const and static
	 * \param size size of data
	 */
	ADPCMSound(const unsigned char *data, int size)
			: soundData(data), soundSize(size), index(0) {}

	/**
	 * Fill a buffer with audio samples
	 * \param buffer a buffer where audio samples (16bit unsigned, 44100Hz)
	 * are to be stored. If there is not enough data to fill the entire buffer
	 * the remaining part must be filled with 0
	 * \param length buffer length, must be divisible by two
	 * \return true if this is the last valif buffer (eof encountered)
	 */
	virtual bool fillMonoBuffer(unsigned short *buffer, int size);
    
    /**
	 * Fill a stereo buffer with audio samples
	 * \param buffer a buffer where audio samples (16bit unsigned, 44100Hz)
	 * are to be stored. If there is not enough data to fill the entire buffer.
     * The buffer format is alternating left-right samples, so buffer[0] is left
     * buffer[1] is right, buffer[2] is again left...
	 * the remaining part must be filled with 0
	 * \param length buffer length, must be divisible by four
	 * \return true if this is the last valif buffer (eof encountered)
	 */
	virtual bool fillStereoBuffer(unsigned short *buffer, int length);

	/**
	 * Rewind the internal sound pointer so that succesive calls to
	 * fillBuffer() start brom the beginning of the sound.
	 */
	virtual void rewind();

private:
	ADPCMSound(const ADPCMSound&);
	ADPCMSound& operator= (const ADPCMSound&);

	const unsigned char *soundData;
	int soundSize;
	int index;
};

/**
 * Class to play an audio file on the STM32's DAC
 */
class Player
{
public:
	/**
	 * \return an instance of the player (singleton)
	 */
	static Player& instance();

	/**
	 * Play an audio file, returning after the file has coompleted playing
	 * \param sound sound file to play
	 */
	void play(Sound& sound);

	/**
	 * \return true if the resource is busy
	 */
	bool isPlaying() const;
private:
	Player();
	Player(const Player&);
	Player& operator= (const Player&);

	mutable miosix::Mutex mutex;
};

#endif //PLAYER_H
