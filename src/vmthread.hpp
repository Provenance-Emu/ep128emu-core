
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2016 Istvan Varga <istvanv@users.sourceforge.net>
// https://github.com/istvan-v/ep128emu/
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

#ifndef EP128EMU_VMTHREAD_HPP
#define EP128EMU_VMTHREAD_HPP

#include "ep128emu.hpp"
#include "system.hpp"
#include "vm.hpp"

namespace Ep128Emu {

  class VMThread : private Thread {
   public:
    VirtualMachine& vm;
    struct VMThreadStatus : public VirtualMachine::VMStatus {
      // 'threadStatus' is zero if the emulation thread is running,
      // and non-zero if it has terminated (negative if the termination
      // occured due to an error).
      int       threadStatus;
      float     speedPercentage;
      bool      isPaused;
      // --------
      VMThreadStatus(VMThread& vmThread_);
    };
   private:
    class Message;
    Mutex           mutex_;
    unsigned long   lockCnt;
    ThreadLock      threadLock1;
    ThreadLock      threadLock2;
    Timer           speedTimer;
    Message         *messageQueue;
    Message         *lastMessage;
    Message         *freeMessageStack;
    int             messageCnt;
    bool            exitFlag;
    bool            joinFlag;
    bool            errorFlag;
    bool            pauseFlag;
    float           timesliceLength;
    float           avgTimesliceLength;
    double          prvTime;
    double          nxtTime;
    volatile size_t allowedRuntime;
    VirtualMachine::VMStatus  vmStatus;
    void            *userData;
    void            (*errorCallback)(void *userData_, const char *msg);
    void            (*processCallback)(void *userData_);
    bool            keyboardState[128];
   public:
    VMThread(VirtualMachine& vm_, void *userData_ = (void *) 0);
    virtual ~VMThread();
    /*!
     * Block the execution of the emulation thread, so that the main thread
     * can safely access the virtual machine object. Returns zero on success,
     * a positive value if the emulation thread did not respond after 't'
     * milliseconds, and a negative value if the thread has terminated for
     * some reason.
     */
    int lock(size_t t);
    /*!
     * Allow the emulation thread to resume execution after a previous
     * successful call to lock().
     */
    void unlock();
    /*!
     * Run emulation (or just wait if paused) for a short period of time,
     * and update status information. This can be called by the main thread
     * after lock() in a loop for single-threaded emulation.
     * Returns false after quit() was called or a fatal error occured.
     */
    bool process();
    /*!
     * Allow execution for a short period of time (one frame of frontend)
     */
    void allowRunFor(size_t microseconds);
    /*!
     * True if VM has already consumed the execution time set up in allowRunFor.
     */
    bool isReady(void);
    /*!
     * Pause emulation if 'n' is true, or continue if 'n' is false.
     * NOTE: the initial state is pause=true.
     */
    void pause(bool n);
    /*!
     * Terminate emulation thread. If 'waitFlag_' is true, it will also
     * wait until the thread has actually stopped, and status information
     * is updated.
     */
    void quit(bool waitFlag_ = false);
    /*!
     * Set pointer to be passed to callback functions.
     */
    void setUserData(void *userData_);
    /*!
     * Set function to be called to print an error message on non-fatal
     * exceptions.
     */
    void setErrorCallback(void (*func)(void *userData_, const char *msg_));
    /*!
     * Reset emulated machine.
     */
    void reset(bool isColdReset_ = false);
    /*!
     * Set amplitude scale for audio output.
     */
    void setAudioOutputVolume(double ampScale_);
    /*!
     * Set state of key 'keyCode_' (0 to 127).
     */
    void setKeyboardState(uint8_t keyCode_, bool isPressed_);
    /*!
     * Send mouse event to the emulated machine. 'dX' and 'dY' are the
     * horizontal and vertical motion of the pointer relative to the position
     * at the time of the previous call, positive values move to the left and
     * up, respectively.
     * Each bit of 'buttonState' corresponds to the current state of a mouse
     * button (1 = pressed):
     *   b0 = left button
     *   b1 = right button
     *   b2 = middle button
     *   b3..b7 = buttons 4 to 8
     * 'mouseWheelEvents' can be the sum of any of the following:
     *   1: mouse wheel up
     *   2: mouse wheel down
     *   4: mouse wheel left
     *   8: mouse wheel right
     */
    void setMouseState(int8_t dX, int8_t dY,
                       uint8_t buttonState, uint8_t mouseWheelEvents);
    /*!
     * Set state of all keys and mouse buttons to released.
     */
    void resetKeyboard();
    /*!
     * Start tape playback.
     */
    void tapePlay();
    /*!
     * Start tape recording.
     */
    void tapeRecord();
    /*!
     * Stop tape playback or recording.
     */
    void tapeStop();
    /*!
     * Set tape position to the specified time (in seconds).
     */
    void tapeSeek(double seekTime_);
    /*!
     * Seek forward (if isForward = true) or backward (if isForward = false)
     * to the nearest cue point, or by 't' seconds if no cue point is found.
     */
    void tapeSeekToCuePoint(bool isForward = true, double t = 10.0);
    /*!
     * Stop playing or recording demo.
     */
    void stopDemo();
    /*!
     * Set function to be called by the emulation thread at an interval of
     * a few milliseconds. This function may throw an std::exception, in which
     * case the error callback will be called to display the error message.
     */
    void setProcessCallback(void (*func)(void *userData_));
    /*!
     * Set maximum emulation speed as a percentage (100 = normal speed).
     * A zero or negative value means no limit.
     */
    void setSpeedPercentage(int speedPercentage_);
  // --------------------------------------------------------------------------
   private:
    virtual void run();
    void cleanup();
    class Message {
     protected:
      VMThread& vmThread;
     public:
      Message   *nextMessage;
      Message(VMThread& vmThread_)
        : vmThread(vmThread_),
          nextMessage((Message *) 0)
      {
      }
      virtual ~Message();
      virtual void process() = 0;
    };
    class Message_Reset : public Message {
     private:
      bool    isColdReset;
     public:
      Message_Reset(VMThread& vmThread_, bool isColdReset_ = false)
        : Message(vmThread_),
          isColdReset(isColdReset_)
      {
      }
      virtual ~Message_Reset();
      virtual void process();
    };
    class Message_SetVolume : public Message {
     private:
      double  ampScale;
     public:
      Message_SetVolume(VMThread& vmThread_, double ampScale_)
        : Message(vmThread_),
          ampScale(ampScale_)
      {
      }
      virtual ~Message_SetVolume();
      virtual void process();
    };
    class Message_KeyboardEvent : public Message {
     private:
      uint8_t keyCode;
      bool    isPressed;
     public:
      Message_KeyboardEvent(VMThread& vmThread_,
                            uint8_t keyCode_, bool isPressed_)
        : Message(vmThread_),
          keyCode(keyCode_),
          isPressed(isPressed_)
      {
      }
      virtual ~Message_KeyboardEvent();
      virtual void process();
    };
    class Message_MouseEvent : public Message {
     private:
      uint32_t  mouseData;
     public:
      Message_MouseEvent(VMThread& vmThread_, uint32_t mouseData_)
        : Message(vmThread_),
          mouseData(mouseData_)
      {
      }
      virtual ~Message_MouseEvent();
      virtual void process();
      static inline uint32_t packMouseEvent(int8_t dX, int8_t dY,
                                            uint8_t buttonState,
                                            uint8_t mouseWheelEvents)
      {
        uint32_t  mouseData_ = 0U;
        ((unsigned char *) &mouseData_)[0] = uint8_t(dX);
        ((unsigned char *) &mouseData_)[1] = uint8_t(dY);
        ((unsigned char *) &mouseData_)[2] = buttonState;
        ((unsigned char *) &mouseData_)[3] = mouseWheelEvents;
        return mouseData_;
      }
      inline void unpackMouseEvent(int8_t& dX, int8_t& dY, uint8_t& buttonState,
                                   uint8_t& mouseWheelEvents) const
      {
        dX = int8_t(uint8_t(((unsigned char *) &mouseData)[0]));
        dY = int8_t(uint8_t(((unsigned char *) &mouseData)[1]));
        buttonState = ((unsigned char *) &mouseData)[2];
        mouseWheelEvents = ((unsigned char *) &mouseData)[3];
      }
    };
    class Message_ResetKeyboard : public Message {
     public:
      Message_ResetKeyboard(VMThread& vmThread_)
        : Message(vmThread_)
      {
      }
      virtual ~Message_ResetKeyboard();
      virtual void process();
    };
    class Message_TapePlay : public Message {
     public:
      Message_TapePlay(VMThread& vmThread_)
        : Message(vmThread_)
      {
      }
      virtual ~Message_TapePlay();
      virtual void process();
    };
    class Message_TapeRecord : public Message {
     public:
      Message_TapeRecord(VMThread& vmThread_)
        : Message(vmThread_)
      {
      }
      virtual ~Message_TapeRecord();
      virtual void process();
    };
    class Message_TapeStop : public Message {
     public:
      Message_TapeStop(VMThread& vmThread_)
        : Message(vmThread_)
      {
      }
      virtual ~Message_TapeStop();
      virtual void process();
    };
    class Message_TapeSeek : public Message {
     private:
      double  seekTime;
      int8_t  seekDirection;
     public:
      Message_TapeSeek(VMThread& vmThread_,
                       int seekDirection_, double seekTime_)
        : Message(vmThread_),
          seekTime(seekTime_),
          seekDirection(int8_t(seekDirection_))
      {
      }
      virtual ~Message_TapeSeek();
      virtual void process();
    };
    class Message_StopDemo : public Message {
     public:
      Message_StopDemo(VMThread& vmThread_)
        : Message(vmThread_)
      {
      }
      virtual ~Message_StopDemo();
      virtual void process();
    };
    class Message_Dummy : public Message {
     private:
      // should have enough space for all the other message types
      double  dummy1;
      double  dummy2;
     public:
      Message_Dummy(VMThread& vmThread_)
        : Message(vmThread_),
          dummy1(0.0),
          dummy2(0.0)
      {
      }
      virtual ~Message_Dummy();
      virtual void process();
    };
    // ----------------
    Message *allocateMessage_();
    void queueMessage(Message *m);
    template <typename T>
    T *allocateMessage()
    {
      Message *m = allocateMessage_();
      if (m)
        return new((void *) m) T(*this);
      return ((T *) 0);
    }
    template <typename T, typename T1>
    T *allocateMessage(T1 arg1)
    {
      Message *m = allocateMessage_();
      if (m)
        return new((void *) m) T(*this, arg1);
      return ((T *) 0);
    }
    template <typename T, typename T1, typename T2>
    T *allocateMessage(T1 arg1, T2 arg2)
    {
      Message *m = allocateMessage_();
      if (m)
        return new((void *) m) T(*this, arg1, arg2);
      return ((T *) 0);
    }
  };

}       // namespace Ep128Emu

#endif  // EP128EMU_VMTHREAD_HPP

