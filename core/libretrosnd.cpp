
// ep128emu-core -- libretro core version of the ep128emu emulator
// Copyright (C) 2022 Zoltan Balogh
// https://github.com/zoltanvb/ep128emu-core
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

/* TODO:
these may be used from config instead of hardcoded constant
but, there are some problems (probably signed-unsigned mismatch)
if they are simply replaced
sound.hwPeriods	16
sound.sampleRate	44100
sound.swPeriods	16
*/

#include "ep128emu.hpp"
#include "system.hpp"
#include "libretrosnd.hpp"
#include <vector>

namespace Ep128Emu {

#define LIBRETRO_PERIOD_SIZE 16
  AudioOutput_libretro::AudioOutput_libretro()
    : AudioOutput(),
      writeBufIndex(0),
      readBufIndex(0),
      readSubBufIndex(0)
  {
    // initialize buffers
    int nPeriodsSW_ = 44100;
    int periodSize = LIBRETRO_PERIOD_SIZE;
    buffers.resize(size_t(nPeriodsSW_));
    for (int i = 0; i < nPeriodsSW_; i++)
    {
      buffers[i].audioData.resize(size_t(periodSize) << 1);
      for (int j = 0; j < (periodSize << 1); j++)
        buffers[i].audioData[j] = 0;
    }
  }

  AudioOutput_libretro::~AudioOutput_libretro()
  {
    writeBufIndex = 0;
    readBufIndex = 0;
    readSubBufIndex = 0;
    buffers.clear();
  }

  void AudioOutput_libretro::sendAudioData(const int16_t *buf, size_t nFrames)
  {
    for (size_t i = 0; i < nFrames; i++)
    {
      Buffer& buf_ = buffers[writeBufIndex];
      buf_.audioData[buf_.writePos++] = buf[(i << 1) + 0];
      buf_.audioData[buf_.writePos++] = buf[(i << 1) + 1];
      if (buf_.writePos >= buf_.audioData.size())
      {
        buf_.writePos = 0;
        //buf_.lrLock.notify();
        forwardMutex.lock();
        if (++writeBufIndex >= buffers.size())
          writeBufIndex = 0;
        forwardMutex.unlock();
      }
    }
    // call base class to write sound file
    AudioOutput::sendAudioData(buf, nFrames);
  }

  void AudioOutput_libretro::forwardAudioData(int16_t *buf_out, size_t* nFrames, int expectedFrames)
  {
    //printf("Trying forwardAudioData: %d %d\n",readBufIndex,writeBufIndex);

    int currOutputIndex = -1;
    int expectedLatencyFrames = 800;
    size_t ringBufferSize = buffers.size();
    int partialFrames_pre = LIBRETRO_PERIOD_SIZE - readSubBufIndex;

    forwardMutex.lock();
    size_t writeBufIndex_ = writeBufIndex;
    int partialFrames_post = buffers[writeBufIndex_].writePos;
    forwardMutex.unlock();

    signed int availableBuffers = 0;
    if(readBufIndex<writeBufIndex_)
    {
      availableBuffers = writeBufIndex_ - readBufIndex - 1;
    }
    else if (readBufIndex > writeBufIndex_)
    {
      availableBuffers = writeBufIndex_ + (ringBufferSize - readBufIndex) -1;
    }
    else
    {
      availableBuffers = 0;
      // if no full buffer available, discard the partial frame as well
      partialFrames_post = 0;
      partialFrames_pre = 0;
    }
    // do not care about partial frames in the buffer being written
    int availableFrames = partialFrames_pre + availableBuffers * LIBRETRO_PERIOD_SIZE + partialFrames_post;

    signed int framesToSend = 0;
    // slowly try to pull frames towards the expected amount
    framesToSend = expectedFrames + (availableFrames - expectedFrames - expectedLatencyFrames)/100;
    if (framesToSend > availableFrames) {
      framesToSend = availableFrames;
      //printf("Audio buffer underrun: rd %d wr %d av %d exp %d fts %d\n",readBufIndex,writeBufIndex_, availableFrames, expectedFrames, framesToSend);
    }
    //printf("Trying forwardAudioData: rd %d wr %d av %d exp %d fts %d\n",readBufIndex,writeBufIndex_, availableFrames, expectedFrames, framesToSend);

    for (signed int i=0; i<framesToSend; i++)
    {
      Buffer& buf_ = buffers[readBufIndex];
      currOutputIndex++;
      buf_out[(currOutputIndex << 1) + 0] = buf_.audioData[(readSubBufIndex << 1) + 0];
      buf_out[(currOutputIndex << 1) + 1] = buf_.audioData[(readSubBufIndex << 1) + 1];
      readSubBufIndex++;
      if (readSubBufIndex>=LIBRETRO_PERIOD_SIZE)
      {
        readSubBufIndex = 0;
        readBufIndex++;
        if (readBufIndex >= ringBufferSize) readBufIndex = 0;
      }
    }

    //printf("Returned frames: %d\n",currOutputIndex);
    nFrames[0]=currOutputIndex+1;
  }

  void AudioOutput_libretro::closeDevice()
  {
    // call base class to reset internal state
    AudioOutput::closeDevice();
  }

}       // namespace Ep128Emu

