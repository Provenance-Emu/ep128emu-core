
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2006 Istvan Varga <istvanv@users.sourceforge.net>
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

#include "ep128emu.hpp"
#include "cfg_db.hpp"
#include "emucfg.hpp"

#include <cstdio>

template <typename T>
static void configChangeCallback(void *userData,
                                 const std::string& name, T value)
{
  (void) name;
  (void) value;
  *((bool *) userData) = true;
}

template <typename T>
static void defineConfigurationVariable(
    Ep128Emu::ConfigurationDB& db, const std::string& name,
    T& value, T defaultValue, bool& changeFlag,
    double minVal, double maxVal, double step = 0.0)
{
  value = defaultValue;
  changeFlag = true;
  db.createKey(name, value);
  db[name].setRange(minVal, maxVal, step);
  db[name].setCallback(&(configChangeCallback<T>), &changeFlag, true);
}

static void defineConfigurationVariable(
    Ep128Emu::ConfigurationDB& db, const std::string& name,
    bool& value, bool defaultValue, bool& changeFlag)
{
  value = defaultValue;
  changeFlag = true;
  db.createKey(name, value);
  db[name].setCallback(&(configChangeCallback<bool>), &changeFlag, true);
}

static void defineConfigurationVariable(
    Ep128Emu::ConfigurationDB& db, const std::string& name,
    std::string& value, const std::string& defaultValue, bool& changeFlag)
{
  value = defaultValue;
  changeFlag = true;
  db.createKey(name, value);
  db[name].setStripString(true);
  db[name].setCallback(&(configChangeCallback<const std::string&>),
                       &changeFlag, true);
}

static void defaultErrorCallback(void *userData, const char *msg)
{
  (void) userData;
  throw Ep128Emu::Exception(msg);
}

namespace Ep128Emu {

  EmulatorConfiguration::EmulatorConfiguration(VirtualMachine& vm__,
                                               VideoDisplay& videoDisplay_,
                                               AudioOutput& audioOutput_)
    : vm_(vm__),
      videoDisplay(videoDisplay_),
      audioOutput(audioOutput_),
      errorCallback(&defaultErrorCallback),
      errorCallbackUserData((void *) 0)
  {
    defineConfigurationVariable(*this, "vm.cpuClockFrequency",
                                vm.cpuClockFrequency, 4000000U,
                                vmConfigurationChanged, 2000000.0, 250000000.0);
    defineConfigurationVariable(*this, "vm.videoClockFrequency",
                                vm.videoClockFrequency, 890625U,
                                vmConfigurationChanged, 178125.0, 1781250.0);
    defineConfigurationVariable(*this, "vm.soundClockFrequency",
                                vm.soundClockFrequency, 500000U,
                                vmConfigurationChanged, 500000.0, 500000.0);
    defineConfigurationVariable(*this, "vm.videoMemoryLatency",
                                vm.videoMemoryLatency, 62U,
                                vmConfigurationChanged, 0.0, 250.0);
    defineConfigurationVariable(*this, "vm.enableMemoryTimingEmulation",
                                vm.enableMemoryTimingEmulation, true,
                                vmConfigurationChanged);
    // ----------------
    defineConfigurationVariable(*this, "memory.ram.size",
                                memory.ram.size, 128,
                                memoryConfigurationChanged, 64.0, 3712.0, 16.0);
    for (size_t i = 0; i < 64; i++) {
      if (i >= 8 && (i & 15) >= 4)
        continue;
      char  tmpBuf[24];
      char  *s = &(tmpBuf[0]);
      std::sprintf(s, "memory.rom.%02X.file", (unsigned int) i);
      defineConfigurationVariable(*this, std::string(s),
                                  memory.rom[i].file, std::string(""),
                                  memoryConfigurationChanged);
      std::sprintf(s, "memory.rom.%02X.offset", (unsigned int) i);
      defineConfigurationVariable(*this, std::string(s),
                                  memory.rom[i].offset, int(0),
                                  memoryConfigurationChanged, 0.0, 16760832.0);
    }
    // ----------------
    defineConfigurationVariable(*this, "display.enabled",
                                display.enabled, true,
                                displaySettingsChanged);
    defineConfigurationVariable(*this, "display.quality",
                                display.quality, int(2),
                                displaySettingsChanged, 0.0, 3.0);
    defineConfigurationVariable(*this, "display.brightness",
                                display.brightness, 0.0,
                                displaySettingsChanged, -0.5, 0.5);
    defineConfigurationVariable(*this, "display.contrast",
                                display.contrast, 1.0,
                                displaySettingsChanged, 0.5, 2.0);
    defineConfigurationVariable(*this, "display.gamma",
                                display.gamma, 1.0,
                                displaySettingsChanged, 0.25, 4.0);
    defineConfigurationVariable(*this, "display.saturation",
                                display.saturation, 1.0,
                                displaySettingsChanged, 0.0, 2.0);
    defineConfigurationVariable(*this, "display.red.brightness",
                                display.red.brightness, 0.0,
                                displaySettingsChanged, -0.5, 0.5);
    defineConfigurationVariable(*this, "display.red.contrast",
                                display.red.contrast, 1.0,
                                displaySettingsChanged, 0.5, 2.0);
    defineConfigurationVariable(*this, "display.red.gamma",
                                display.red.gamma, 1.0,
                                displaySettingsChanged, 0.25, 4.0);
    defineConfigurationVariable(*this, "display.green.brightness",
                                display.green.brightness, 0.0,
                                displaySettingsChanged, -0.5, 0.5);
    defineConfigurationVariable(*this, "display.green.contrast",
                                display.green.contrast, 1.0,
                                displaySettingsChanged, 0.5, 2.0);
    defineConfigurationVariable(*this, "display.green.gamma",
                                display.green.gamma, 1.0,
                                displaySettingsChanged, 0.25, 4.0);
    defineConfigurationVariable(*this, "display.blue.brightness",
                                display.blue.brightness, 0.0,
                                displaySettingsChanged, -0.5, 0.5);
    defineConfigurationVariable(*this, "display.blue.contrast",
                                display.blue.contrast, 1.0,
                                displaySettingsChanged, 0.5, 2.0);
    defineConfigurationVariable(*this, "display.blue.gamma",
                                display.blue.gamma, 1.0,
                                displaySettingsChanged, 0.25, 4.0);
    defineConfigurationVariable(*this, "display.effects.param1",
                                display.effects.param1, 0.35,
                                displaySettingsChanged, 0.0, 0.5);
    defineConfigurationVariable(*this, "display.effects.param2",
                                display.effects.param2, 0.7,
                                displaySettingsChanged, 0.0, 1.0);
    defineConfigurationVariable(*this, "display.effects.param3",
                                display.effects.param3, 0.3,
                                displaySettingsChanged, 0.0, 1.0);
    defineConfigurationVariable(*this, "display.width",
                                display.width, 704,
                                displaySettingsChanged, 352.0, 1408.0);
    defineConfigurationVariable(*this, "display.height",
                                display.height, 576,
                                displaySettingsChanged, 288.0, 1152.0);
    defineConfigurationVariable(*this, "display.pixelAspectRatio",
                                display.pixelAspectRatio, 1.0,
                                displaySettingsChanged, 0.5, 2.0);
    // ----------------
    defineConfigurationVariable(*this, "sound.enabled",
                                sound.enabled, true,
                                soundSettingsChanged);
    defineConfigurationVariable(*this, "sound.highQuality",
                                sound.highQuality, false,
                                soundSettingsChanged);
    defineConfigurationVariable(*this, "sound.device",
                                sound.device, int(0),
                                soundSettingsChanged, 0.0, 1000.0);
    defineConfigurationVariable(*this, "sound.sampleRate",
                                sound.sampleRate, 48000.0,
                                soundSettingsChanged, 11025.0, 192000.0);
    defineConfigurationVariable(*this, "sound.latency",
                                sound.latency, 0.1,
                                soundSettingsChanged, 0.005, 0.5);
    defineConfigurationVariable(*this, "sound.hwPeriods",
                                sound.hwPeriods, int(4),
                                soundSettingsChanged, 2.0, 16.0);
    defineConfigurationVariable(*this, "sound.swPeriods",
                                sound.swPeriods, int(3),
                                soundSettingsChanged, 2.0, 16.0);
    defineConfigurationVariable(*this, "sound.file",
                                sound.file, std::string(""),
                                soundSettingsChanged);
    defineConfigurationVariable(*this, "sound.volume",
                                sound.volume, 0.7071,
                                soundSettingsChanged, 0.01, 1.0);
    defineConfigurationVariable(*this, "sound.dcBlockFilter1Freq",
                                sound.dcBlockFilter1Freq, 10.0,
                                soundSettingsChanged, 1.0, 1000.0);
    defineConfigurationVariable(*this, "sound.dcBlockFilter2Freq",
                                sound.dcBlockFilter2Freq, 10.0,
                                soundSettingsChanged, 1.0, 1000.0);
    // ----------------
    for (int i = 0; i < 128; i++) {
      char  tmpBuf[16];
      char  *s = &(tmpBuf[0]);
      std::sprintf(s, "keyboard.%02X.x", (unsigned int) i);
      for (int j = 0; j < 2; j++) {
        s[12] = '0' + char(j);
        defineConfigurationVariable(*this, std::string(s),
                                    keyboard[i][j], int(-1),
                                    keyboardMapChanged, -1.0, 65535.0);
      }
    }
    // ----------------
    for (int i = 0; i < 4; i++) {
      FloppyDriveSettings *floppy_ = (FloppyDriveSettings *) 0;
      bool                *floppyChanged_ = (bool *) 0;
      switch (i) {
      case 0:
        floppy_ = &(floppy.a);
        floppyChanged_ = &floppyAChanged;
        break;
      case 1:
        floppy_ = &(floppy.b);
        floppyChanged_ = &floppyBChanged;
        break;
      case 2:
        floppy_ = &(floppy.c);
        floppyChanged_ = &floppyCChanged;
        break;
      case 3:
        floppy_ = &(floppy.d);
        floppyChanged_ = &floppyDChanged;
        break;
      }
      char  tmpBuf[32];
      char  *s = &(tmpBuf[0]);
      std::sprintf(s, "floppy.%c.imageFile", int('a') + i);
      defineConfigurationVariable(*this, std::string(s),
                                  floppy_->imageFile, std::string(""),
                                  *floppyChanged_);
      std::sprintf(s, "floppy.%c.tracks", int('a') + i);
      defineConfigurationVariable(*this, std::string(s),
                                  floppy_->tracks, int(-1),
                                  *floppyChanged_, -1.0, 240.0);
      std::sprintf(s, "floppy.%c.sides", int('a') + i);
      defineConfigurationVariable(*this, std::string(s),
                                  floppy_->sides, int(2),
                                  *floppyChanged_, -1.0, 2.0);
      std::sprintf(s, "floppy.%c.sectorsPerTrack", int('a') + i);
      defineConfigurationVariable(*this, std::string(s),
                                  floppy_->sectorsPerTrack, int(9),
                                  *floppyChanged_, -1.0, 240.0);
    }
    // ----------------
    defineConfigurationVariable(*this, "tape.imageFile",
                                tape.imageFile, std::string(""),
                                tapeSettingsChanged);
    defineConfigurationVariable(*this, "tape.fastMode",
                                tape.fastMode, false,
                                fastTapeModeChanged);
    // ----------------
    defineConfigurationVariable(*this, "fileio.workingDirectory",
                                fileio.workingDirectory, std::string("."),
                                fileioSettingsChanged);
    // ----------------
    defineConfigurationVariable(*this, "debug.bpPriorityThreshold",
                                debug.bpPriorityThreshold, int(0),
                                debugSettingsChanged, 0.0, 4.0);
  }

  EmulatorConfiguration::~EmulatorConfiguration()
  {
    keyboardMap.clear();
  }

  void EmulatorConfiguration::applySettings()
  {
    if (vmConfigurationChanged) {
      // assume none of these will throw exceptions
      vm_.setCPUFrequency(vm.cpuClockFrequency);
      vm_.setVideoFrequency(vm.videoClockFrequency);
#if 0
      vm_.setSoundClockFrequency(vm.soundClockFrequency);
#endif
      vm_.setVideoMemoryLatency(vm.videoMemoryLatency);
      vm_.setEnableMemoryTimingEmulation(vm.enableMemoryTimingEmulation);
      vmConfigurationChanged = false;
    }
    if (memoryConfigurationChanged) {
      vm_.resetMemoryConfiguration(memory.ram.size);
      for (size_t i = 0; i < 64; i++) {
        if (i >= 8 && (i & 15) >= 4)
          continue;
        try {
          vm_.loadROMSegment(uint8_t(i),
                             memory.rom[i].file.c_str(),
                             size_t(memory.rom[i].offset));
        }
        catch (Exception& e) {
          memory.rom[i].file = "";
          memory.rom[i].offset = 0;
          vm_.loadROMSegment(uint8_t(i), "", 0);
          errorCallback(errorCallbackUserData, e.what());
        }
      }
      memoryConfigurationChanged = false;
    }
    if (displaySettingsChanged) {
      // assume that changing the display settings will not fail
      vm_.setEnableDisplay(display.enabled);
      VideoDisplay::DisplayParameters dp(videoDisplay.getDisplayParameters());
      dp.displayQuality = display.quality;
      dp.brightness = display.brightness;
      dp.contrast = display.contrast;
      dp.gamma = display.gamma;
      dp.saturation = display.saturation;
      dp.redBrightness = display.red.brightness;
      dp.redContrast = display.red.contrast;
      dp.redGamma = display.red.gamma;
      dp.greenBrightness = display.green.brightness;
      dp.greenContrast = display.green.contrast;
      dp.greenGamma = display.green.gamma;
      dp.blueBrightness = display.blue.brightness;
      dp.blueContrast = display.blue.contrast;
      dp.blueGamma = display.blue.gamma;
      dp.blendScale1 = display.effects.param1;
      dp.blendScale2 = display.effects.param2;
      dp.blendScale3 = display.effects.param3;
      dp.pixelAspectRatio = display.pixelAspectRatio;
      videoDisplay.setDisplayParameters(dp);
      // NOTE: resolution changes are not handled here
      displaySettingsChanged = false;
    }
    if (soundSettingsChanged) {
      vm_.setEnableAudioOutput(sound.enabled);
      if (!sound.enabled) {
        // close device if sound is disabled
        audioOutput.closeDevice();
      }
      else {
        try {
          audioOutput.setParameters(sound.device, float(sound.sampleRate),
                                    float(sound.latency), sound.hwPeriods,
                                    sound.swPeriods);
        }
        catch (Exception& e) {
          audioOutput.closeDevice();
          errorCallback(errorCallbackUserData, e.what());
        }
      }
      vm_.setAudioOutputHighQuality(sound.highQuality);
      vm_.setAudioOutputVolume(float(sound.volume));
      vm_.setAudioOutputFilters(float(sound.dcBlockFilter1Freq),
                                float(sound.dcBlockFilter2Freq));
      try {
        audioOutput.setOutputFile(sound.file);
      }
      catch (Exception& e) {
        sound.file = "";
        audioOutput.setOutputFile(sound.file);
        errorCallback(errorCallbackUserData, e.what());
      }
      soundSettingsChanged = false;
    }
    if (keyboardMapChanged) {
      keyboardMap.clear();
      for (int i = 0; i < 128; i++) {
        for (int j = 0; j < 128; j++) {
          if (keyboard[i][j] >= 0)
            keyboardMap[keyboard[i][j]] = i;
        }
      }
      keyboardMapChanged = false;
    }
    if (floppyAChanged) {
      try {
        vm_.setDiskImageFile(0, floppy.a.imageFile,
                             floppy.a.tracks, floppy.a.sides,
                             floppy.a.sectorsPerTrack);
      }
      catch (Exception& e) {
        floppy.a.imageFile = "";
        vm_.setDiskImageFile(0, floppy.a.imageFile,
                             floppy.a.tracks, floppy.a.sides,
                             floppy.a.sectorsPerTrack);
        errorCallback(errorCallbackUserData, e.what());
      }
      floppyAChanged = false;
    }
    if (floppyBChanged) {
      try {
        vm_.setDiskImageFile(1, floppy.b.imageFile,
                             floppy.b.tracks, floppy.b.sides,
                             floppy.b.sectorsPerTrack);
      }
      catch (Exception& e) {
        floppy.b.imageFile = "";
        vm_.setDiskImageFile(1, floppy.b.imageFile,
                             floppy.b.tracks, floppy.b.sides,
                             floppy.b.sectorsPerTrack);
        errorCallback(errorCallbackUserData, e.what());
      }
      floppyBChanged = false;
    }
    if (floppyCChanged) {
      try {
        vm_.setDiskImageFile(2, floppy.c.imageFile,
                             floppy.c.tracks, floppy.c.sides,
                             floppy.c.sectorsPerTrack);
      }
      catch (Exception& e) {
        floppy.c.imageFile = "";
        vm_.setDiskImageFile(2, floppy.c.imageFile,
                             floppy.c.tracks, floppy.c.sides,
                             floppy.c.sectorsPerTrack);
        errorCallback(errorCallbackUserData, e.what());
      }
      floppyCChanged = false;
    }
    if (floppyDChanged) {
      try {
        vm_.setDiskImageFile(3, floppy.d.imageFile,
                             floppy.d.tracks, floppy.d.sides,
                             floppy.d.sectorsPerTrack);
      }
      catch (Exception& e) {
        floppy.d.imageFile = "";
        vm_.setDiskImageFile(3, floppy.d.imageFile,
                             floppy.d.tracks, floppy.d.sides,
                             floppy.d.sectorsPerTrack);
        errorCallback(errorCallbackUserData, e.what());
      }
      floppyDChanged = false;
    }
    if (tapeSettingsChanged) {
      try {
        vm_.setTapeFileName(tape.imageFile);
      }
      catch (Exception& e) {
        tape.imageFile = "";
        vm_.setTapeFileName(tape.imageFile);
        errorCallback(errorCallbackUserData, e.what());
      }
      tapeSettingsChanged = false;
    }
    if (fastTapeModeChanged) {
      vm_.setEnableFastTapeMode(tape.fastMode);
      fastTapeModeChanged = false;
    }
    if (fileioSettingsChanged) {
      try {
        vm_.setWorkingDirectory(fileio.workingDirectory);
      }
      catch (Exception& e) {
        fileio.workingDirectory = ".";
        try {
          vm_.setWorkingDirectory(fileio.workingDirectory);
        }
        catch (Exception&) {
        }
        errorCallback(errorCallbackUserData, e.what());
      }
      fileioSettingsChanged = false;
    }
    if (debugSettingsChanged) {
      vm_.setBreakPointPriorityThreshold(debug.bpPriorityThreshold);
      debugSettingsChanged = false;
    }
  }

  int EmulatorConfiguration::convertKeyCode(int keyCode)
  {
    std::map< int, int >::iterator  i;
    i = keyboardMap.find(keyCode);
    if (i == keyboardMap.end())
      return -1;
    return ((*i).second & 127);
  }

  void EmulatorConfiguration::setErrorCallback(void (*func)(void *,
                                                            const char *),
                                               void *userData_)
  {
    if (func)
      errorCallback = func;
    else
      errorCallback = &defaultErrorCallback;
    errorCallbackUserData = userData_;
  }

}       // namespace Ep128Emu

