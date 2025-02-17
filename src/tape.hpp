
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2010 Istvan Varga <istvanv@users.sourceforge.net>
// http://sourceforge.net/projects/ep128emu/
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

#ifndef EP128EMU_TAPE_HPP
#define EP128EMU_TAPE_HPP

#include "ep128emu.hpp"

#include <vector>
#ifndef EXCLUDE_SOUND_LIBS
#include <sndfile.h>
#endif // EXCLUDE_SOUND_LIBS

namespace Ep128Emu {

  class Tape {
   protected:
    long      sampleRate;       // defaults to 24000
    int       fileBitsPerSample;
    int       requestedBitsPerSample;
    bool      isReadOnly;
    bool      isPlaybackOn;
    bool      isRecordOn;
    bool      isMotorOn;
    size_t    tapeLength;       // tape length (in samples)
    size_t    tapePosition;     // current read/write position (in samples)
    int       inputState;
    int       outputState;
    Tape(int bitsPerSample = 1);
   public:
    virtual ~Tape();
    /*!
     * Get sample rate of tape emulation.
     */
    inline long getSampleRate() const
    {
      return sampleRate;
    }
    /*!
     * Get number of bits (1, 2, 4, or 8) per sample in tape file.
     */
    inline int getSampleSize() const
    {
      return fileBitsPerSample;
    }
    /*!
     * Returns true if the tape file is opened in read-only mode.
     */
    inline bool getIsReadOnly() const
    {
      return isReadOnly;
    }
   protected:
    virtual void runOneSample_();
   public:
    /*!
     * Run tape emulation for a period of 1.0 / getSampleRate() seconds.
     */
    inline void runOneSample()
    {
      if (isPlaybackOn && isMotorOn)
        runOneSample_();
    }
    /*!
     * Turn motor on (newState = true) or off (newState = false).
     */
    virtual void setIsMotorOn(bool newState);
    /*!
     * Returns true if the motor is currently on.
     */
    inline bool getIsMotorOn() const
    {
      return isMotorOn;
    }
    /*!
     * Start playback.
     * (note: the motor should also be turned on to actually play)
     */
    inline void play()
    {
      isPlaybackOn = true;
      isRecordOn = false;
    }
    /*!
     * Start recording; if the tape file is read-only, this is equivalent
     * to calling play().
     * (note: the motor should also be turned on to actually record)
     */
    inline void record()
    {
      isPlaybackOn = true;
      isRecordOn = !isReadOnly;
    }
    /*!
     * Stop playback and recording.
     */
    virtual void stop();
    /*!
     * Set input signal for recording.
     */
    inline void setInputSignal(int newState)
    {
      inputState = newState;
    }
    /*!
     * Get output signal.
     */
    inline int getOutputSignal() const
    {
      return outputState;
    }
    /*!
     * Seek to the specified time (in seconds).
     */
    virtual void seek(double t);
    /*!
     * Returns the current tape position in seconds.
     */
    inline double getPosition() const
    {
      return (double(uint32_t(tapePosition)) / double(sampleRate));
    }
    /*!
     * Returns the current tape length in seconds.
     */
    inline double getLength() const
    {
      return (double(uint32_t(tapeLength)) / double(sampleRate));
    }
    /*!
     * Returns true if the current position is at the end of the tape;
     * should be used only when reading a tape file.
     */
    inline bool getIsEndOfTape() const
    {
      return (tapePosition >= tapeLength);
    }
    /*!
     * Seek forward (if isForward = true) or backward (if isForward = false)
     * to the nearest cue point, or by 't' seconds if no cue point is found.
     */
    virtual void seekToCuePoint(bool isForward = true, double t = 10.0);
    /*!
     * Create a new cue point at the current tape position.
     * Has no effect if the file does not have a cue point table, or it
     * is read-only.
     */
    virtual void addCuePoint();
    /*!
     * Delete the cue point nearest to the current tape position.
     * Has no effect if the file is read-only.
     */
    virtual void deleteNearestCuePoint();
    /*!
     * Delete all cue points. Has no effect if the file is read-only.
     */
    virtual void deleteAllCuePoints();
  };

  class Tape_Ep128Emu : public Tape {
   private:
    std::FILE *f;               // tape image file
    uint8_t   *buf;             // 4096 bytes
    uint32_t  *fileHeader;      // table with a length of 1024, contains:
                                //      0, 1: magic number (0x0275CD72,
                                //            0x1C445126)
                                //         2: number of bits per sample in
                                //            file (1, 2, 4, or 8)
                                //         3: sample rate in Hz (10000 to
                                //            120000)
                                // 4 to 1022: the sample positions where cue
                                //            points are defined, in sorted
                                //            order, padded with 0xFFFFFFFF
                                //            values
                                //      1023: must be 0xFFFFFFFF
    size_t    cuePointCnt;
    bool      isBufferDirty;    // true if 'buf' has been changed,
                                // and not written to file yet
    bool      usingNewFormat;
    // ----------------
    void seek_(size_t pos_);
    bool findCuePoint_(size_t& ndx_, size_t pos_);
    void packSamples_();
    bool writeBuffer_();
    bool readBuffer_();
    void unpackSamples_();
    bool writeHeader_();
    void flushBuffer_();
   public:
    /*!
     * Open tape file 'fileName'. If the file does not exist yet, it may be
     * created (depending on the 'mode' parameter) with the specified sample
     * rate and bits per sample. Otherwise, 'sampleRate_' is ignored, and
     * samples are converted according to 'bitsPerSample'.
     * 'mode' can be one of the following values:
     *   0: open tape file read-write if possible, or create a new file if
     *      it does not exist
     *   1: open tape file read-write if possible, and fail if it does not
     *      exist
     *   2: open an existing tape file read-only
     *   3: create new tape file in read-write mode; if the file already
     *      exists, it is truncated and a new header is written
     */
    Tape_Ep128Emu(const char *fileName, int mode = 0,
                  long sampleRate_ = 24000L, int bitsPerSample = 1);
    virtual ~Tape_Ep128Emu();
    /*!
     * Run tape emulation for a period of 1.0 / getSampleRate() seconds.
     */
   protected:
    virtual void runOneSample_();
   public:
    /*!
     * Turn motor on (newState = true) or off (newState = false).
     */
    virtual void setIsMotorOn(bool newState);
    /*!
     * Stop playback and recording.
     */
    virtual void stop();
    /*!
     * Seek to the specified time (in seconds).
     */
    virtual void seek(double t);
    /*!
     * Seek forward (if isForward = true) or backward (if isForward = false)
     * to the nearest cue point, or by 't' seconds if no cue point is found.
     */
    virtual void seekToCuePoint(bool isForward = true, double t = 10.0);
    /*!
     * Create a new cue point at the current tape position.
     * Has no effect if the file does not have a cue point table, or it
     * is read-only.
     */
    virtual void addCuePoint();
    /*!
     * Delete the cue point nearest to the current tape position.
     * Has no effect if the file is read-only.
     */
    virtual void deleteNearestCuePoint();
    /*!
     * Delete all cue points. Has no effect if the file is read-only.
     */
    virtual void deleteAllCuePoints();
  };

  class Tape_WAV : public Tape {
   private:
    std::FILE *f;               // wave file
    uint8_t   *buf;             // 4096 bytes
    uint32_t  *fileHeader;      // 44 byte long header:
/*
0         4   ChunkID          Contains the letters "RIFF" in ASCII form
4         4   ChunkSize        ignored here
8         4   Format           Contains the letters "WAVE"
12        4   Subchunk1ID      Contains the letters "fmt "
16        4   Subchunk1Size    16 for PCM.
20        2   AudioFormat      PCM = 1 (i.e. Linear quantization)
22        2   NumChannels      Mono = 1, Stereo = 2, etc.
24        4   SampleRate       8000, 44100, etc.
28        4   ByteRate         ignored here
32        2   BlockAlign       ignored here
34        2   BitsPerSample    8 bits = 8, 16 bits = 16, etc.
36        4   Subchunk2ID      Contains the letters "data"
40        4   Subchunk2Size    ignored here
44        *   Data             The actual sound data.
*/
    bool      usingNewFormat;
    // ----------------
    bool readBuffer_();
    void unpackSamples_();
   public:
    /*!
     * Open tape file 'fileName'.
     */
    Tape_WAV(const char *fileName, int bitsPerSample = 1);
    virtual ~Tape_WAV();
    /*!
     * Run tape emulation for a period of 1.0 / getSampleRate() seconds.
     */
   protected:
    virtual void runOneSample_();
   public:
    /*!
     * Turn motor on (newState = true) or off (newState = false).
     */
    virtual void setIsMotorOn(bool newState);
    /*!
     * Stop playback and recording.
     */
    virtual void stop();
    /*!
     * Seek to the specified time (in seconds).
     */
    virtual void seek(double t);
    /*!
     * Not supported for this format.
     */
    virtual void seekToCuePoint(bool isForward = true, double t = 10.0);
    /*!
     * Not supported for this format.
     */
    virtual void addCuePoint();
    /*!
     * Not supported for this format.
     */
    virtual void deleteNearestCuePoint();
    /*!
     * Not supported for this format.
     */
    virtual void deleteAllCuePoints();
  };



  class Tape_EPTE : public Tape {
   private:
    std::FILE *f;
    size_t    bytesRemaining;
    bool      endOfTape;
    uint8_t   shiftReg;
    uint8_t   bitsRemaining;
    uint8_t   halfPeriodSamples;
    size_t    samplesRemaining;
    size_t    leaderSampleCnt;
    size_t    chunkBytesRemaining;
    size_t    chunkCnt;
    uint32_t  chunkArray[128]; // for EPTE/TAPir chunk pointers stored in header
   public:
    /*!
     * Open EPTE format tape file 'fileName' read-only.
     */
    Tape_EPTE(const char *fileName, int bitsPerSample = 1);
    virtual ~Tape_EPTE();
    /*!
     * Run tape emulation for a period of 1.0 / getSampleRate() seconds.
     */
   protected:
    virtual void runOneSample_();
   public:
    /*!
     * Turn motor on (newState = true) or off (newState = false).
     */
    virtual void setIsMotorOn(bool newState);
    /*!
     * Stop playback and recording.
     */
    virtual void stop();
    /*!
     * Seek to the specified time (in seconds).
     */
    virtual void seek(double t);
    /*!
     * Seek forward (if isForward = true) or backward (if isForward = false)
     * to the nearest cue point, or by 't' seconds if no cue point is found.
     */
    virtual void seekToCuePoint(bool isForward = true, double t = 10.0);
    /*!
     * Create a new cue point at the current tape position.
     * Has no effect if the file does not have a cue point table, or it
     * is read-only.
     */
    virtual void addCuePoint();
    /*!
     * Delete the cue point nearest to the current tape position.
     * Has no effect if the file is read-only.
     */
    virtual void deleteNearestCuePoint();
    /*!
     * Delete all cue points. Has no effect if the file is read-only.
     */
    virtual void deleteAllCuePoints();
  };

  class Tape_TZX : public Tape {
   private:
    std::FILE *f;
    uint8_t   currentBlockType;
    uint8_t   currentMode;
    bool      endOfTape;
    uint8_t   shiftReg;
    uint32_t  pulseTimer;
    uint32_t  pulseLength;
    uint32_t  pulseCnt;
    uint16_t  pilotPulseLength;
    uint16_t  syncPulseLength1;
    uint16_t  syncPulseLength2;
    uint16_t  bit0PulseLength;
    uint16_t  bit1PulseLength;
    uint8_t   bit0PulseCnt;
    uint8_t   bit1PulseCnt;
    uint16_t  pilotPulseCnt;
    uint8_t   lastByteBits;
    uint8_t   pulseSequencePulsesLeft;
    uint32_t  pauseLength;
    uint32_t  clockFrequency;
    uint32_t  dataBlockBytesLeft;
    uint32_t  directRecordingSampleRate;
    uint32_t  directRecordingTimer;
    uint32_t  loopFilePos;
    size_t    loopStartTime;
    uint16_t  loopRepeatCnt;
    bool      isTAPFile;
   public:
    /*!
     * Open TZX or Spectrum TAP format tape file 'fileName' read-only.
     */
    Tape_TZX(const char *fileName, int bitsPerSample = 1);
    virtual ~Tape_TZX();
    /*!
     * Run tape emulation for a period of 1.0 / getSampleRate() seconds.
     */
   protected:
    void tapeReset();
    bool readByte(uint8_t& n);
    bool readUInt16(uint16_t& n);
    bool readUInt24(uint32_t& n);
    bool readUInt32(uint32_t& n);
    uint32_t convertPulseLength(uint32_t n, uint32_t clockFreq_ = 0U);
    void setPauseMode(uint32_t pauseLength_ = 0U);
    void readNextTZXBlock();
    void directRecordingNextBit();
    void dataBlockNextBit();
    virtual void runOneSample_();
   public:
    /*!
     * Turn motor on (newState = true) or off (newState = false).
     */
    virtual void setIsMotorOn(bool newState);
    /*!
     * Stop playback and recording.
     */
    virtual void stop();
    /*!
     * Seek to the specified time (in seconds).
     */
    virtual void seek(double t);
    /*!
     * Seek forward (if isForward = true) or backward (if isForward = false)
     * to the nearest cue point, or by 't' seconds if no cue point is found.
     */
    virtual void seekToCuePoint(bool isForward = true, double t = 10.0);
    /*!
     * Create a new cue point at the current tape position.
     * Has no effect if the file does not have a cue point table, or it
     * is read-only.
     */
    virtual void addCuePoint();
    /*!
     * Delete the cue point nearest to the current tape position.
     * Has no effect if the file is read-only.
     */
    virtual void deleteNearestCuePoint();
    /*!
     * Delete all cue points. Has no effect if the file is read-only.
     */
    virtual void deleteAllCuePoints();
  };

#ifndef EXCLUDE_SOUND_LIBS
  class Tape_SoundFile : public Tape {
   public:
    class TapeFilter {
     private:
      std::vector<float> irFFT;
      std::vector<float> fftBuf;
      std::vector<float> outBuf;
      size_t  sampleCnt;
     public:
      TapeFilter(size_t irSamples = 1024);
      virtual ~TapeFilter();
      void setFilterParameters(float sampleRate, float minFreq, float maxFreq);
      float processSample(float inputSignal);
      static void fft(float *buf, size_t n, bool isInverse);
    };
   private:
    SNDFILE     *sf;            // tape image file
    std::vector<short>  buf;    // 1024 interleaved sample frames
    int         nChannels;
    int         requestedChannel;
    bool        enableFIRFilter;
    bool        isBufferDirty;  // true if 'buf' has been changed,
                                // and not written to file yet
    TapeFilter  firFilter;
    // ----------------
    void seek_(size_t pos_);
    bool writeBuffer_();
    void flushBuffer_();
   public:
    /*!
     * Open tape file 'fileName'.
     * 'mode' can be one of the following values:
     *   0, 1: open tape file read-write if possible, and fail if it does not
     *         exist
     *   2:    open an existing tape file read-only
     */
    Tape_SoundFile(const char *fileName, int mode = 0, int bitsPerSample = 1);
    virtual ~Tape_SoundFile();
    /*!
     * Run tape emulation for a period of 1.0 / getSampleRate() seconds.
     */
   protected:
    virtual void runOneSample_();
   public:
    /*!
     * Turn motor on (newState = true) or off (newState = false).
     */
    virtual void setIsMotorOn(bool newState);
    /*!
     * Stop playback and recording.
     */
    virtual void stop();
    /*!
     * Seek to the specified time (in seconds).
     */
    virtual void seek(double t);
    /*!
     * Seek forward (if isForward = true) or backward (if isForward = false)
     * to the nearest cue point, or by 't' seconds if no cue point is found.
     */
    virtual void seekToCuePoint(bool isForward = true, double t = 10.0);
    /*!
     * Create a new cue point at the current tape position.
     * Has no effect if the file does not have a cue point table, or it
     * is read-only.
     */
    virtual void addCuePoint();
    /*!
     * Delete the cue point nearest to the current tape position.
     * Has no effect if the file is read-only.
     */
    virtual void deleteNearestCuePoint();
    /*!
     * Delete all cue points. Has no effect if the file is read-only.
     */
    virtual void deleteAllCuePoints();
    /*!
     * Set parameters for sound file reading.
     */
    void setParameters(int requestedChannel_, bool enableFIRFilter_,
                       float filterMinFreq_, float filterMaxFreq_);
  };
#endif // EXCLUDE_SOUND_LIBS

  /*!
   * Open tape file 'fileName'. If the file does not exist yet, it may be
   * created (depending on the 'mode' parameter) with the specified sample
   * rate and bits per sample. Otherwise, 'sampleRate_' is ignored, and
   * samples are converted according to 'bitsPerSample'.
   * 'mode' can be one of the following values:
   *   0: open tape file read-write if possible, or create a new file if
   *      it does not exist
   *   1: open tape file read-write if possible, and fail if it does not
   *      exist
   *   2: open an existing tape file read-only
   *   3: create new tape file in read-write mode; if the file already
   *      exists, it is truncated and a new header is written
   */
  Tape *openTapeFile(const char *fileName, int mode = 0,
                     long sampleRate_ = 24000L, int bitsPerSample = 1);

}       // namespace Ep128Emu

#endif  // EP128EMU_TAPE_HPP

