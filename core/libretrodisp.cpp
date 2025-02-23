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

#include "ep128emu.hpp"
#include "system.hpp"
#include "libretrodisp.hpp"

namespace Ep128Emu
{

// --------------------------------------------------------------------------
// The colormap does the static conversion from the ep128emu output (Nick format)
// to the pixel format that can be used by the framebuffer (RGB565 or XRGB8888)
// Floating point conversion wouldn't be strictly necessary, it is inherited
// from the ep128emu feature of tunable display parameters.
LibretroDisplay::Colormap::Colormap()
{
  palette16 = new uint16_t[256];
  palette32 = new uint32_t[256];
  DisplayParameters dp;
  setParams(dp);
}

LibretroDisplay::Colormap::~Colormap()
{
  delete[] palette16;
  delete[] palette32;
}

void LibretroDisplay::Colormap::setParams(const DisplayParameters& dp)
{
  float   rTbl[256];
  float   gTbl[256];
  float   bTbl[256];
  for (size_t i = 0; i < 256; i++)
  {
    float   r = float(uint8_t(i)) / 255.0f;
    float   g = float(uint8_t(i)) / 255.0f;
    float   b = float(uint8_t(i)) / 255.0f;
    if (dp.indexToRGBFunc)
      dp.indexToRGBFunc(uint8_t(i), r, g, b);
    rTbl[i] = r;
    gTbl[i] = g;
    bTbl[i] = b;
  }
  for (size_t i = 0; i < 256; i++)
  {
    palette16[i] = pixelConvRGB565(rTbl[i], gTbl[i], bTbl[i]);
    palette32[i] = pixelConvXRGB8888(rTbl[i], gTbl[i], bTbl[i]);
  }
}

uint32_t LibretroDisplay::Colormap::pixelConvXRGB8888(double r, double g, double b)
{
  unsigned int  ri, gi, bi;
  ri = (r > 0.0 ? (r < 1.0 ? (unsigned int) (r * 255.0 + 0.5) : 255U) : 0U);
  gi = (g > 0.0 ? (g < 1.0 ? (unsigned int) (g * 255.0 + 0.5) : 255U) : 0U);
  bi = (b > 0.0 ? (b < 1.0 ? (unsigned int) (b * 255.0 + 0.5) : 255U) : 0U);
  return ((uint32_t(ri) << 16) + (uint32_t(gi) << 8) + uint32_t(bi));
}

uint16_t LibretroDisplay::Colormap::pixelConvRGB565(double r, double g, double b)
{
  unsigned int  ri, gi, bi;
  ri = (r > 0.0 ? (r < 1.0 ? (unsigned int) (r * 255.0 + 0.5) : 255U) : 0U);
  gi = (g > 0.0 ? (g < 1.0 ? (unsigned int) (g * 255.0 + 0.5) : 255U) : 0U);
  bi = (b > 0.0 ? (b < 1.0 ? (unsigned int) (b * 255.0 + 0.5) : 255U) : 0U);
  return (((uint16_t(ri) & 0xF8) << 8) + ((uint16_t(gi) & 0xFC) << 3) + ((uint16_t(bi) >> 3) & 0xF8));
}



void LibretroDisplay::decodeLine(unsigned char *outBuf,
                                 const unsigned char *inBuf, size_t nBytes)
{
  const unsigned char *bufp = inBuf;
  unsigned char *endp = outBuf + 768;
  do
  {
    switch (bufp[0])
    {
    case 0x00:                        // blank
      do
      {
        outBuf[15] = outBuf[14] =
        outBuf[13] = outBuf[12] =
        outBuf[11] = outBuf[10] =
        outBuf[ 9] = outBuf[ 8] =
        outBuf[ 7] = outBuf[ 6] =
        outBuf[ 5] = outBuf[ 4] =
        outBuf[ 3] = outBuf[ 2] =
        outBuf[ 1] = outBuf[ 0] = 0x00;
        outBuf = outBuf + 16;
        bufp = bufp + 1;
        if (outBuf >= endp)
          break;
      }
      while (bufp[0] == 0x00);
      break;
    case 0x01:                        // 1 pixel, 256 colors
      do
      {
        outBuf[15] = outBuf[14] =
        outBuf[13] = outBuf[12] =
        outBuf[11] = outBuf[10] =
        outBuf[ 9] = outBuf[ 8] =
        outBuf[ 7] = outBuf[ 6] =
        outBuf[ 5] = outBuf[ 4] =
        outBuf[ 3] = outBuf[ 2] =
        outBuf[ 1] = outBuf[ 0] = bufp[1];
        outBuf = outBuf + 16;
        bufp = bufp + 2;
        if (outBuf >= endp)
          break;
      }
      while (bufp[0] == 0x01);
      break;
    case 0x02:                        // 2 pixels, 256 colors
      do
      {
        outBuf[ 7] = outBuf[ 6] =
        outBuf[ 5] = outBuf[ 4] =
        outBuf[ 3] = outBuf[ 2] =
        outBuf[ 1] = outBuf[ 0] = bufp[1];
        outBuf[15] = outBuf[14] =
        outBuf[13] = outBuf[12] =
        outBuf[11] = outBuf[10] =
        outBuf[ 9] = outBuf[ 8] = bufp[2];
        outBuf = outBuf + 16;
        bufp = bufp + 3;
        if (outBuf >= endp)
          break;
      }
      while (bufp[0] == 0x02);
      break;
    case 0x03:                        // 8 pixels, 2 colors
      do
      {
        unsigned char c0 = bufp[1];
        unsigned char c1 = bufp[2];
        unsigned char b = bufp[3];
        outBuf[ 1] = outBuf[ 0] = ((b & 128) ? c1 : c0);
        outBuf[ 3] = outBuf[ 2] = ((b &  64) ? c1 : c0);
        outBuf[ 5] = outBuf[ 4] = ((b &  32) ? c1 : c0);
        outBuf[ 7] = outBuf[ 6] = ((b &  16) ? c1 : c0);
        outBuf[ 9] = outBuf[ 8] = ((b &   8) ? c1 : c0);
        outBuf[11] = outBuf[10] = ((b &   4) ? c1 : c0);
        outBuf[13] = outBuf[12] = ((b &   2) ? c1 : c0);
        outBuf[15] = outBuf[14] = ((b &   1) ? c1 : c0);
        outBuf = outBuf + 16;
        bufp = bufp + 4;
        if (outBuf >= endp)
          break;
      }
      while (bufp[0] == 0x03);
      break;
    case 0x04:                        // 4 pixels, 256 colors
      do
      {
        outBuf[ 3] = outBuf[ 2] =
        outBuf[ 1] = outBuf[ 0] = bufp[1];
        outBuf[ 7] = outBuf[ 6] =
        outBuf[ 5] = outBuf[ 4] = bufp[2];
        outBuf[11] = outBuf[10] =
        outBuf[ 9] = outBuf[ 8] = bufp[3];
        outBuf[15] = outBuf[14] =
        outBuf[13] = outBuf[12] = bufp[4];
        outBuf = outBuf + 16;
        bufp = bufp + 5;
        if (outBuf >= endp)
          break;
      }
      while (bufp[0] == 0x04);
      break;
    case 0x06:                        // 16 (2*8) pixels, 2*2 colors
      do
      {
        unsigned char c0 = bufp[1];
        unsigned char c1 = bufp[2];
        unsigned char b = bufp[3];
        outBuf[ 0] = ((b & 128) ? c1 : c0);
        outBuf[ 1] = ((b &  64) ? c1 : c0);
        outBuf[ 2] = ((b &  32) ? c1 : c0);
        outBuf[ 3] = ((b &  16) ? c1 : c0);
        outBuf[ 4] = ((b &   8) ? c1 : c0);
        outBuf[ 5] = ((b &   4) ? c1 : c0);
        outBuf[ 6] = ((b &   2) ? c1 : c0);
        outBuf[ 7] = ((b &   1) ? c1 : c0);
        c0 = bufp[4];
        c1 = bufp[5];
        b = bufp[6];
        outBuf[ 8] = ((b & 128) ? c1 : c0);
        outBuf[ 9] = ((b &  64) ? c1 : c0);
        outBuf[10] = ((b &  32) ? c1 : c0);
        outBuf[11] = ((b &  16) ? c1 : c0);
        outBuf[12] = ((b &   8) ? c1 : c0);
        outBuf[13] = ((b &   4) ? c1 : c0);
        outBuf[14] = ((b &   2) ? c1 : c0);
        outBuf[15] = ((b &   1) ? c1 : c0);
        outBuf = outBuf + 16;
        bufp = bufp + 7;
        if (outBuf >= endp)
          break;
      }
      while (bufp[0] == 0x06);
      break;
    case 0x08:                        // 8 pixels, 256 colors
      do
      {
        outBuf[ 1] = outBuf[ 0] = bufp[1];
        outBuf[ 3] = outBuf[ 2] = bufp[2];
        outBuf[ 5] = outBuf[ 4] = bufp[3];
        outBuf[ 7] = outBuf[ 6] = bufp[4];
        outBuf[ 9] = outBuf[ 8] = bufp[5];
        outBuf[11] = outBuf[10] = bufp[6];
        outBuf[13] = outBuf[12] = bufp[7];
        outBuf[15] = outBuf[14] = bufp[8];
        outBuf = outBuf + 16;
        bufp = bufp + 9;
        if (outBuf >= endp)
          break;
      }
      while (bufp[0] == 0x08);
      break;
    default:                          // invalid flag byte
      do
      {
        *(outBuf++) = 0x00;
      }
      while (outBuf < endp);
      break;
    }
  }
  while (outBuf < endp);

  (void) nBytes;
#if 0
  if (size_t(bufp - inBuf) != nBytes)
    throw std::exception();
#endif
}

// --------------------------------------------------------------------------

void LibretroDisplay::Message_LineData::copyLine(const uint8_t *buf,
    size_t nBytes)
{
  nBytes_ = (unsigned int) nBytes;
  if (nBytes_ & 3U)
    buf_[nBytes_ / 4U] = 0U;
  if (nBytes_)
    std::memcpy(&(buf_[0]), buf, nBytes_);
}

LibretroDisplay::Message_LineData&
LibretroDisplay::Message_LineData::operator=(const Message_LineData& r)
{
  std::memcpy(&nBytes_, &(r.nBytes_),
              size_t((const char *) &(r.buf_[((r.nBytes_ + 7U) & (~7U)) / 4U])
                     - (const char *) &(r.nBytes_)));
  return (*this);
}

void LibretroDisplay::deleteMessage(Message *m)
{
  messageQueueMutex.lock();
  m->nxt = freeMessageStack;
  freeMessageStack = m;
  messageQueueMutex.unlock();
}

void LibretroDisplay::queueMessage(Message *m)
{
  messageQueueMutex.lock();
  if (exitFlag)
  {
    messageQueueMutex.unlock();
    std::free(m);
    return;
  }
  m->nxt = (Message *) 0;
  if (lastMessage)
    lastMessage->nxt = m;
  else
    messageQueue = m;
  lastMessage = m;
//  bool    isFrameDone = (m->msgType == Message::MsgType_FrameDone);
  messageQueueMutex.unlock();
  /*if (EP128EMU_UNLIKELY(isFrameDone))
  {
    if (!videoResampleEnabled)
    {
      threadLock.wait(1);
    }
  }*/
}


void LibretroDisplay::setDisplayParameters(const DisplayParameters& dp)
{
  // No other display parameters are supported.
  displayParameters.indexToRGBFunc = dp.indexToRGBFunc;
  colormap.setParams(dp);

}

const VideoDisplay::DisplayParameters&
LibretroDisplay::getDisplayParameters() const
{
  return displayParameters;
}

void LibretroDisplay::drawLine(const uint8_t *buf, size_t nBytes)
{

  if (curLine >= 0 && curLine < (EP128EMU_LIBRETRO_SCREEN_HEIGHT + 2))
  {
    Message_LineData  *m = allocateMessage<Message_LineData>();
    m->lineNum = curLine;
    m->copyLine(buf, nBytes);
    queueMessage(m);
  }
  if (vsyncCnt != 0)
  {
    curLine += 2;
    if (vsyncCnt >= (EP128EMU_VSYNC_MIN_LINES + 2 - EP128EMU_VSYNC_OFFSET) &&
        (vsyncState || vsyncCnt >= (EP128EMU_VSYNC_MAX_LINES
                                    + 2 - EP128EMU_VSYNC_OFFSET)))
    {
      vsyncCnt = 2 - EP128EMU_VSYNC_OFFSET;
    }
    vsyncCnt++;
  }
  else
  {
    curLine = (oddFrame ? -1 : 0);
    vsyncCnt++;
    frameDone();
    frameCount++;
  }
}

void LibretroDisplay::vsyncStateChange(bool newState, unsigned int currentSlot_)
{
  vsyncState = newState;
  if (newState &&
      vsyncCnt >= (EP128EMU_VSYNC_MIN_LINES + 2 - EP128EMU_VSYNC_OFFSET))
  {
    vsyncCnt = 2 - EP128EMU_VSYNC_OFFSET;
    // Odd frames are detected by vSync activated in the middle of the line (half-line sync)
    // This will trigger 2nd "field" in interlaced mode.
    oddFrame = (currentSlot_ >= 20U && currentSlot_ < 48U);
    if (oddFrame)
    {
      interlacedFrameCount = 3;
    }
    else interlacedFrameCount = interlacedFrameCount > 0 ? interlacedFrameCount - 1 : 0;
  }
}
// --------------------------------------------------------------------------

LibretroDisplay::LibretroDisplay(int xx, int yy, int ww, int hh,
                                 const char *lbl, bool useHalfFrame_)
  :     colormap(),
        messageQueue((Message *) 0),
        lastMessage((Message *) 0),
        freeMessageStack((Message *) 0),
        messageQueueMutex(),
        lineBuffers((Message_LineData **) 0),
        curLine(0),
        vsyncCnt(0),
        skippingFrame(false),
        useHalfFrame(useHalfFrame_),
        framesPendingFlag(false),
        vsyncState(false),
        oddFrame(false),
        lineBuf((unsigned char *) 0),
        threadLock1(false),
        threadLock2(true),
        exitFlag(false),
        displayParameters(),
        savedDisplayParameters(),
        redrawFlag(false),
        prvFrameWasOdd(false),
        lastLineNum(-2),
#ifdef EP128EMU_USE_XRGB8888
        frame_buf1((uint32_t *) 0),
#else
        frame_buf1((uint16_t *) 0),
#endif // EP128EMU_USE_XRGB8888
        interlacedFrameCount(0),
        frameCount(0),
        contentTopEdge(0),
        contentLeftEdge(0),
        contentBottomEdge(EP128EMU_LIBRETRO_SCREEN_HEIGHT-1),
        contentRightEdge(EP128EMU_LIBRETRO_SCREEN_WIDTH-1),
        scanBorders(false),
        bordersScanned(false)
{
  try
  {
    lineBuffers = new Message_LineData*[EP128EMU_LIBRETRO_SCREEN_HEIGHT + 2];
    for (size_t n = 0; n < (EP128EMU_LIBRETRO_SCREEN_HEIGHT + 2); n++)
      lineBuffers[n] = (Message_LineData *) 0;
  }
  catch (...)
  {
    if (lineBuffers)
      delete[] lineBuffers;
    throw;
  }
  try
  {
    linesChanged = new bool[289];
    for (size_t n = 0; n < 289; n++)
      linesChanged[n] = false;
  }
  catch (...)
  {
    if (linesChanged)
      delete[] linesChanged;
    throw;
  }
  resetViewport();

#ifdef EP128EMU_USE_XRGB8888
  frameSize = ww * hh * sizeof(uint32_t);
  frame_buf1 = (uint32_t*) calloc(ww * hh, sizeof(uint32_t));
  frame_buf3 = (uint32_t*) calloc(ww * hh, sizeof(uint32_t));
#else
  frameSize = ww * hh * sizeof(uint16_t);
  frame_buf1 = (uint16_t*) calloc(ww * hh, sizeof(uint16_t));
  frame_buf3 = (uint16_t*) calloc(ww * hh, sizeof(uint16_t));
#endif // EP128EMU_USE_XRGB8888
  frame_bufActive = frame_buf1;
  frame_bufSpare = frame_buf3;
  lineBuf = (unsigned char*) calloc(ww, sizeof(unsigned char));
  this->start();
}

// Enable display processing. If sync is required, do not return until all input is processed.
void LibretroDisplay::wakeDisplay(bool syncRequired)
{
  threadLock1.notify();
  if (syncRequired)
  {
    threadLock2.wait(10);
  }
}

void LibretroDisplay::resetViewport()
{
  setViewport(0,0,EP128EMU_LIBRETRO_SCREEN_WIDTH-1,EP128EMU_LIBRETRO_SCREEN_HEIGHT-1);
}

bool LibretroDisplay::setViewport(int x1, int y1, int x2, int y2)
{
  if (x1>=0 && x1<x2)
  {
  }
  else return false;
  if (y1>=0 && y1<y2)
  {
  }
  else return false;
  if (x2<EP128EMU_LIBRETRO_SCREEN_WIDTH)
  {
  }
  else return false;
  if (y2<EP128EMU_LIBRETRO_SCREEN_HEIGHT)
  {
  }
  else return false;

  viewPortX1 = x1;
  viewPortY1 = y1;
  viewPortX2 = x2;
  viewPortY2 = y2;
  return true;
}

bool LibretroDisplay::isViewportDefault()
{
  if(
    viewPortX1 == 0 &&
    viewPortY1 == 0 &&
    viewPortX2 == EP128EMU_LIBRETRO_SCREEN_WIDTH-1 &&
    viewPortY2 == EP128EMU_LIBRETRO_SCREEN_HEIGHT-1
  ) return true;
  else return false;
}

// Main display routine implementing Thread::run.
void LibretroDisplay::run()
{
  bool frameDone;
  while (true)
  {
    if (exitFlag) break;
    frameDone = false;
    threadLock1.wait(10);
    do
    {
      frameDone = checkEvents();
      if (frameDone)
      {
        draw(frame_bufActive, scanBorders);
        scanBorders = false;
      }
    }
    while (frameDone);
    threadLock2.notify();
  }
}

void LibretroDisplay::frameDone()
{
  Message *m = allocateMessage<Message_FrameDone>();
  queueMessage(m);
}

bool LibretroDisplay::checkEvents()
{
  redrawFlag = false;
  while (true)
  {
    messageQueueMutex.lock();
    Message *m = messageQueue;
    if (m)
    {
      messageQueue = m->nxt;
      if (messageQueue)
      {
        if (!messageQueue->nxt)
          lastMessage = messageQueue;
      }
      else
        lastMessage = (Message *) 0;
    }
    messageQueueMutex.unlock();
    if (!m)
      break;

    if (EP128EMU_EXPECT(m->msgType == Message::MsgType_LineData))
    {
      Message_LineData  *msg;
      msg = static_cast<Message_LineData *>(m);
      int     lineNum = msg->lineNum;
      if (lineNum >= 0 && lineNum < 578)
      {
        lastLineNum = lineNum;
        // check if this line has changed
        if (lineBuffers[lineNum])
        {
          if (*(lineBuffers[lineNum]) == *msg)
          {
            deleteMessage(m);
            continue;
          }
        }
        linesChanged[lineNum >> 1] = true;
        if (lineBuffers[lineNum])
          deleteMessage(lineBuffers[lineNum]);
        lineBuffers[lineNum] = msg;
        continue;
      }
    }
    else if (m->msgType == Message::MsgType_FrameDone)
    {
      redrawFlag = true;
      deleteMessage(m);
      break;
    }
    deleteMessage(m);
  }
  return redrawFlag;
}

LibretroDisplay::~LibretroDisplay()
{
  exitFlag = true;
  threadLock1.notify();
  this->join();

  free(frame_buf1);
  free(frame_buf3);
  frame_buf1 = NULL;
  frame_buf3 = NULL;
  frame_bufActive = NULL;
  frame_bufReady = NULL;
  frame_bufSpare = NULL;
  free(lineBuf);
  lineBuf = NULL;
  messageQueueMutex.lock();
  while (freeMessageStack)
  {
    Message *m = freeMessageStack;
    freeMessageStack = m->nxt;
    std::free(m);
  }
  while (messageQueue)
  {
    Message *m = messageQueue;
    messageQueue = m->nxt;
    std::free(m);
  }
  lastMessage = (Message *) 0;
  messageQueueMutex.unlock();
  for (size_t n = 0; n < (EP128EMU_LIBRETRO_SCREEN_HEIGHT + 2); n++)
  {
    Message *m = lineBuffers[n];
    if (m)
    {
      lineBuffers[n] = (Message_LineData *) 0;
      std::free(m);
    }
  }
  delete[] lineBuffers;
}

void LibretroDisplay::limitFrameRate(bool isEnabled)
{
  (void) isEnabled;
}

void LibretroDisplay::draw(void * fb, bool scanForBorder)
{
  int borderColor = 0;
  int firstNonzeroLine   = EP128EMU_LIBRETRO_SCREEN_HEIGHT;
  int firstNonzeroCol    = EP128EMU_LIBRETRO_SCREEN_WIDTH;
  int firstNonborderLine = EP128EMU_LIBRETRO_SCREEN_HEIGHT;
  int firstNonborderCol  = EP128EMU_LIBRETRO_SCREEN_WIDTH;
  int lastNonzeroLine    = 0;
  int lastNonzeroCol     = 0;
  int lastNonborderLine  = 0;
  int lastNonborderCol   = 0;

  // render to own framebuf always if there's a chance of resolution change
  if ((scanForBorder || bordersScanned) && !interlacedFrameCount)
  {
    frame_bufActive = frame_buf1;
  }
  for (int yc = 0; yc < EP128EMU_LIBRETRO_SCREEN_HEIGHT; yc++)
  {
    // Skip odd lines if interlace is not used.
    if (!interlacedFrameCount && (yc & 1)) continue;
    // Skip any display if not within viewport (inclusive).
    if (yc < viewPortY1 || yc > viewPortY2) continue;
    if (lineBuffers[yc])
    {
      // decode video data
      const unsigned char *bufp = (unsigned char *) 0;
      size_t  nBytes = 0;
      bool nonzero = false;
      bool nonborder = false;
      lineBuffers[yc]->getLineData(bufp, nBytes);
      decodeLine(lineBuf,bufp,nBytes);

      for(int i=0; i<EP128EMU_LIBRETRO_SCREEN_WIDTH; i++)
      {
        // Skip any display if not within viewport (inclusive).
        if (i<viewPortX1 || i>viewPortX2) continue;
        unsigned char pixelValue = lineBuf[i];

#ifdef EP128EMU_USE_XRGB8888
        uint32_t pixelResult = colormap(pixelValue);
#else
        uint16_t pixelResult = colormap(pixelValue);
#endif // EP128EMU_USE_XRGB8888

        if (scanForBorder && pixelResult > 0)
        {
          if (!nonzero)
          {
            nonzero=true;
            if(i<firstNonzeroCol)firstNonzeroCol = i;
            // Detect border color only in the first 5 rows (arbitrary decision)
            if (yc<5 && borderColor == 0) borderColor = pixelResult;
          }
          if (!nonborder && borderColor>0 && pixelResult != borderColor)
          {
            nonborder=true;
            if (i<firstNonborderCol)firstNonborderCol = i;
          }
          if (i>lastNonzeroCol)
          {
            lastNonzeroCol = i;
          }
          if (i>lastNonborderCol && pixelResult != borderColor)
          {
            lastNonborderCol = i;
          }
        }
        int currWidth = viewPortX2 - viewPortX1 + 1;
        int currLine = yc - viewPortY1;
        int currPos = i - viewPortX1;
        // Fake interlace: use previous frame's alternate lines.
        // Fake as there's no fading or other effect to actually emulate interlace artifacts
        if (interlacedFrameCount)
        {
          frame_bufActive[currLine * currWidth + currPos] = pixelResult;
          frame_bufSpare[currLine * currWidth + currPos] = pixelResult;
          if (yc < viewPortY2-1)
            frame_bufActive[(currLine+1) * currWidth + currPos] = frame_bufSpare[(currLine+1) * currWidth + currPos];
        }
        else
        {
          if (useHalfFrame)
          {
            // use different addressing to achieve packed frame even in this case
            frame_bufActive[currLine/2 * currWidth + currPos] = pixelResult;
          }
          else
          {
            // doublescan if half frame usage is disabled
            frame_bufActive[currLine * currWidth + currPos] = pixelResult;
            frame_bufActive[(currLine+1) * currWidth + currPos] = pixelResult;
          }
        }
      }
      if(scanForBorder && nonzero)
      {
        if(yc < firstNonzeroLine) firstNonzeroLine = yc;
        if(yc > lastNonzeroLine) lastNonzeroLine = yc;
      }
      // known problem: if there is border, then bottom edge will not be the obvious screen edge of black and border
      // but the last row where any other content is present
      // also if game uses 1-color dithered border, that will also be cut off
      if(scanForBorder && nonborder)
      {
        if(yc < firstNonborderLine) firstNonborderLine = yc;
        if(yc > lastNonborderLine) lastNonborderLine = yc;
      }
    }
  }
  if (scanForBorder)
  {
    if (borderColor > 0)
    {
      contentTopEdge    = firstNonborderLine;
      contentBottomEdge =  lastNonborderLine;
      contentLeftEdge   = firstNonborderCol;
      contentRightEdge  =  lastNonborderCol;
    }
    else
    {
      contentTopEdge    = firstNonzeroLine;
      contentBottomEdge =  lastNonzeroLine;
      contentLeftEdge   = firstNonzeroCol;
      contentRightEdge  =  lastNonzeroCol;
    }
    bordersScanned = true;
  }
}

}       // namespace Ep128Emu

