/* TODO

build for mac
magyar nyelvű leírás is
double free crash at new game load sometimes
save state for speaker and mono states
  new snapshot version

  format, hw, trainer detection from tzx / cdt
  http://k1.spdns.de/Develop/Projects/zasm/Info/TZX%20format.html
  https://www.cpcwiki.eu/index.php?title=Format:CDT_tape_image_file_format&mobileaction=toggle_view_desktop
  tapir format compatibility

doublecheck cpc and zx keyboard maps

gfx:
crash amikor interlaced módban akarok menübe menni, mintha frame dupe-hoz lenne köze
sw fb + interlace = crash
info msg overlay
long info msg with game instructions // inkább a collection részeként
keyboard is stuck after entering menu (like ctrl+f1), upstroke not detected, should be reseted
virtual keyboard

core options v2
check and include libretro common
detailed type detection from content name
cheat support
m3u support (cpc 3 guerra)
cp/m support (EP, CPC)
locale support ep, cpc
rom config for clkoff+hfont

low prio:
ep128cfg support player 2 mapping
info msg for other players
TAPir format support
opengl display support
demo record/play
support for content in zip
EP Mouse support

*/


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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
//#include <math.h>
#include <algorithm>

#include "ep128vm.hpp"
#include "vmthread.hpp"
#include "emucfg.hpp"
#include "fileio.hpp"
#include "libretro.h"
#include "libretro-funcs.hpp"
#include "libretrodisp.hpp"
#include "core.hpp"
#ifdef WIN32
#include <windows.h>
#endif // WIN32

static struct retro_log_callback logging;
static retro_log_printf_t log_cb;
static retro_environment_t environ_cb;

const char *retro_save_directory;
const char *retro_system_directory;
const char *retro_content_directory;
char retro_system_data_directory[512];
char retro_system_bios_directory[512];
char retro_system_save_directory[512];
char retro_content_filepath[512];
uint16_t audioBuffer[EP128EMU_SAMPLE_RATE*1000*2];

std::string contentFileName="";

retro_usec_t curr_frame_time = 0;
retro_usec_t prev_frame_time = 0;
float waitPeriod = 0.001;
bool useSwFb = false;
bool useHalfFrame = false;
bool enableDiskControl = false;
bool needsDiskControl = false;
int borderSize = 0;
bool soundHq = true;
unsigned maxUsers;
bool maxUsersSupported = true;
bool canSkipFrames = false;
bool enhancedRom = false;

Ep128Emu::VMThread              *vmThread    = (Ep128Emu::VMThread *) 0;
Ep128Emu::EmulatorConfiguration *config      = (Ep128Emu::EmulatorConfiguration *) 0;
Ep128Emu::LibretroCore          *core        = (Ep128Emu::LibretroCore *) 0;

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
  (void)level;
  va_list va;
  va_start(va, fmt);
  vfprintf(stderr, fmt, va);
  va_end(va);
}

static void cfgErrorFunc(void *userData, const char *msg)
{
  (void) userData;
  std::fprintf(stderr, "WARNING: %s\n", msg);
}

static void fileNameCallback(void *userData, std::string& fileName)
{
  (void) userData;
  fileName = contentFileName;
}

void set_frame_time_cb(retro_usec_t usec)
{
  if (usec == 0 || usec > 1000000/50)
  {
    curr_frame_time = 1000000/50;
  }
  else
  {
    curr_frame_time = usec;
  }
  prev_frame_time = curr_frame_time;

}


static void update_keyboard_cb(bool down, unsigned keycode,
                               uint32_t character, uint16_t key_modifiers)
{
  if(keycode != RETROK_UNKNOWN && core)
    core->update_keyboard(down,keycode,character,key_modifiers);
}

static void check_variables(void)
{
  struct retro_variable var =
  {
    .key = "ep128emu_wait",
  };
  if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
  {
    waitPeriod = 0.001f * std::atoi(var.value);
  }

  var.key = "ep128emu_swfb";
  if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
  {
    useSwFb = std::atoi(var.value) == 1 ? true : false;
  }

  var.key = "ep128emu_sdhq";
  if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
  {
    bool soundHq_;
    soundHq_ = std::atoi(var.value) == 1 ? true : false;
    if (soundHq != soundHq_)
    {
      soundHq = soundHq_;
      if(core)
      {
        core->config->sound.highQuality = soundHq;
        core->config->soundSettingsChanged = true;
        core->config->applySettings();
      }
    }
  }

  var.key = "ep128emu_useh";
  if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
  {
    useHalfFrame = std::atoi(var.value) == 1 ? true : false;
  }

  var.key = "ep128emu_brds";
  if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
  {
    borderSize = std::atoi(var.value);
    if(core)
      core->borderSize = borderSize*2;
  }

  var.key = "ep128emu_romv";
  if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
  {
    if(var.value[0] == 'E') { enhancedRom = true;}
    else { enhancedRom = false;}
  }

  std::string zoomKey;
  var.key = "ep128emu_zoom";
  if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
  {
    zoomKey = var.value;
    Ep128Emu::stringToLowerCase(zoomKey);
  }

  std::string infoKey;
  var.key = "ep128emu_info";
  if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
  {
    infoKey = var.value;
    Ep128Emu::stringToLowerCase(infoKey);
  }

  std::string autofireKey;
  var.key = "ep128emu_afbt";
  if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
  {
    autofireKey = var.value;
    Ep128Emu::stringToLowerCase(autofireKey);
  }

  int autofireSpeed = -1;
  var.key = "ep128emu_afsp";
  if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
  {
    autofireSpeed = std::atoi(var.value);
  }

  // If function is not supported, use all users (and don't interrogate again)
  if(maxUsersSupported && !environ_cb(RETRO_ENVIRONMENT_GET_INPUT_MAX_USERS,&maxUsers)) {
    maxUsers = EP128EMU_MAX_USERS;
    maxUsersSupported = false;
    log_cb(RETRO_LOG_INFO, "GET_INPUT_MAX_USERS not supported, using fixed %d\n", EP128EMU_MAX_USERS);
  }

  if(core)
    core->initialize_joystick_map(zoomKey,infoKey,autofireKey, autofireSpeed,
    Ep128Emu::joystick_type.at("DEFAULT"), Ep128Emu::joystick_type.at("DEFAULT"), Ep128Emu::joystick_type.at("DEFAULT"),
    Ep128Emu::joystick_type.at("DEFAULT"), Ep128Emu::joystick_type.at("DEFAULT"), Ep128Emu::joystick_type.at("DEFAULT"));

  if(vmThread) vmThread->resetKeyboard();
}

void retro_init(void)
{
  struct retro_log_callback log;

  // Init log
  if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
    log_cb = log.log;
  else
    log_cb = fallback_log;

  // actually it is the advanced disk control...
  unsigned dci;
  if (environ_cb(RETRO_ENVIRONMENT_GET_DISK_CONTROL_INTERFACE_VERSION, &dci))
    enableDiskControl = true;
  else
    enableDiskControl = false;

  const char *system_dir = NULL;
  if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) && system_dir)
  {
    // if defined, use the system directory
    retro_system_directory=system_dir;
  }

  const char *content_dir = NULL;
  if (environ_cb(RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY, &content_dir) && content_dir)
  {
    // if defined, use the system directory
    retro_content_directory=content_dir;
  }

  const char *save_dir = NULL;
  if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir) && save_dir)
  {
    // If save directory is defined use it, otherwise use system directory
    retro_save_directory = *save_dir ? save_dir : retro_system_directory;
  }
  else
  {
    // make retro_save_directory the same in case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY is not implemented by the frontend
    retro_save_directory=retro_system_directory;
  }

  if(system_dir == NULL)
  {
    strcpy(retro_system_bios_directory, ".");
  }
  else
  {
    strncpy(
      retro_system_bios_directory,
      retro_system_directory,
      sizeof(retro_system_bios_directory) - 1
    );
  }
  strncpy(
    retro_system_save_directory,
    retro_save_directory,
    sizeof(retro_system_save_directory) - 1
  );

  log_cb(RETRO_LOG_INFO, "Retro ROM DIRECTORY %s\n", retro_system_bios_directory);
  log_cb(RETRO_LOG_DEBUG, "Retro SAVE_DIRECTORY %s\n", retro_system_save_directory);
  log_cb(RETRO_LOG_DEBUG, "Retro CONTENT_DIRECTORY %s\n", retro_content_directory);

   static const struct retro_controller_info ports[EP128EMU_MAX_USERS+1] = {
      { Ep128Emu::controller_description, 11  }, // port 1
      { Ep128Emu::controller_description, 11  }, // port 2
      { Ep128Emu::controller_description, 11  }, // port 3
      { Ep128Emu::controller_description, 11  }, // port 4
      { Ep128Emu::controller_description, 11  }, // port 5
      { Ep128Emu::controller_description, 11  }, // port 6
      { NULL, 0 }
   };

   environ_cb( RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports );

  struct retro_keyboard_callback kcb = { update_keyboard_cb };
  environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &kcb);
  struct retro_frame_time_callback ftcb = { set_frame_time_cb };
  environ_cb(RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK, &ftcb);

  environ_cb(RETRO_ENVIRONMENT_GET_CAN_DUPE,&canSkipFrames);
  //environ_cb(RETRO_ENVIRONMENT_GET_OVERSCAN,&showan);
  // safe mode
  canSkipFrames = false;
  check_variables();
#ifdef WIN32
  timeBeginPeriod(1U);
#endif
  log_cb(RETRO_LOG_DEBUG, "Creating core...\n");
  core = new Ep128Emu::LibretroCore(log_cb, Ep128Emu::VM_config.at("EP128_DISK"), Ep128Emu::LOCALE_UK, canSkipFrames, retro_system_bios_directory, retro_system_save_directory,"","",useHalfFrame, enhancedRom);
  config = core->config;
  config->setErrorCallback(&cfgErrorFunc, (void *) 0);
  vmThread = core->vmThread;
  check_variables();
  log_cb(RETRO_LOG_DEBUG, "Starting core...\n");
  core->start();
  core->change_resolution(core->currWidth,core->currHeight,environ_cb);
}

void retro_deinit(void)
{
  if (core)
  {
    delete core;
    core = (Ep128Emu::LibretroCore *) 0;
    vmThread = (Ep128Emu::VMThread *) 0;
    config = (Ep128Emu::EmulatorConfiguration *) 0;
  }
}

void retro_get_system_info(struct retro_system_info *info)
{
  memset(info, 0, sizeof(*info));
  info->library_name     = "ep128emu";
  info->library_version  = "v1.0.0";
  info->need_fullpath    = true;
  info->valid_extensions = "img|dsk|tap|dtf|com|trn|128|bas|cas|cdt|tzx|.";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
  float aspect = 4.0f / 3.0f;
  aspect = 4.0f / (3.0f / (float) (core->isHalfFrame ? EP128EMU_LIBRETRO_SCREEN_HEIGHT/2/(float)core->currHeight : EP128EMU_LIBRETRO_SCREEN_HEIGHT/(float)core->currHeight));
  //aspect = 4.0f / (3.0f / (float) (EP128EMU_LIBRETRO_SCREEN_HEIGHT/(float)core->currHeight));
  info->timing = (struct retro_system_timing)
  {
    .fps = 50.0,
    .sample_rate = EP128EMU_SAMPLE_RATE_FLOAT,
  };

  info->geometry = (struct retro_game_geometry)
  {
    .base_width   = (unsigned int) core->currWidth,
    .base_height  = (unsigned int) core->currHeight,
    .max_width    = EP128EMU_LIBRETRO_SCREEN_WIDTH,
    .max_height   = EP128EMU_LIBRETRO_SCREEN_HEIGHT,
    .aspect_ratio = aspect,
  };
}

void retro_set_environment(retro_environment_t cb)
{
  environ_cb = cb;

  bool no_content = true;
  cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);

  if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
    log_cb = logging.log;
  else
    log_cb = fallback_log;

  environ_cb = cb;

  static const struct retro_variable vars[] =
  {
    { "ep128emu_wait", "Main thread wait (ms); 0|1|5|10" },
    { "ep128emu_sdhq", "High sound quality; 1|0" },
    { "ep128emu_swfb", "Use accelerated SW framebuffer; 0|1" },
    { "ep128emu_useh", "Enable resolution changes (requires restart); 1|0" },
    { "ep128emu_brds", "Border lines to keep when zooming in; 0|2|4|8|10|20" },
    { "ep128emu_romv", "System ROM version (EP only); Original|Enhanced" },
    { "ep128emu_zoom", "User 1 Zoom button; R3|Start|Select|X|Y|A|B|L|R|L2|R2|L3" },
    { "ep128emu_info", "User 1 Info button; L3|R3|Start|Select|X|Y|A|B|L|R|L2|R2" },
    { "ep128emu_afbt", "User 1 Autofire for button; None|X|Y|A|B|L|R|L2|R2|L3|R3|Start|Select" },
    { "ep128emu_afsp", "User 1 Autofire repeat delay; 1|2|4|8|16" },
    { NULL, NULL },
  };
  cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void*)vars);
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
  audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
  audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
  input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
  input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
  video_cb = cb;
}

void retro_reset(void)
{
  if(vmThread) vmThread->reset(true);
}

static void update_input(void)
{
  input_poll_cb();
  core->update_input(input_state_cb, environ_cb, maxUsers);
}

static void render(void)
{
  core->render(video_cb, environ_cb);
}

/*
static void audio_callback(void)
{
    audio_cb(0, 0);
}
*/
static void audio_callback_batch(void)
{
  size_t nFrames=0;
  int exp = curr_frame_time*EP128EMU_SAMPLE_RATE/1000000;
  //printf("expected : %d curr_frame_time: %d\n",exp,curr_frame_time);

  core->audioOutput->forwardAudioData((int16_t*)audioBuffer,&nFrames,exp);
  //printf("sending frames: %d frame_time: %d\n",nFrames,curr_frame_time);
  audio_batch_cb((int16_t*)audioBuffer, nFrames);
}

void retro_run(void)
{

  bool updated = false;
  if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
    check_variables();

  void *buf = NULL;
  if (useSwFb)
  {
    struct retro_framebuffer fb = {0};
    fb.width = core->currWidth;
    fb.height = core->currHeight;
    fb.access_flags = RETRO_MEMORY_ACCESS_WRITE;
#ifdef EP128EMU_USE_XRGB8888
    if (environ_cb(RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER, &fb) && fb.format == RETRO_PIXEL_FORMAT_XRGB8888)
#else
    if (environ_cb(RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER, &fb) && fb.format == RETRO_PIXEL_FORMAT_RGB565)
#endif // EP128EMU_USE_XRGB8888
    {
      buf = fb.data;
    }
  }
  update_input();
  core->run_for(curr_frame_time,waitPeriod,buf);
  audio_callback_batch();
  core->sync_display();
  render();
}

bool header_match(const char* buf1, const unsigned char* buf2, size_t length)
{
  for (size_t i = 0; i < length; i++)
  {
    if ((unsigned char)buf1[i] != buf2[i])
    {
      return false;
    }
  }
  return true;
}

bool retro_load_game(const struct retro_game_info *info)
{

#ifdef EP128EMU_USE_XRGB8888
  enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
#else
  enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
#endif // EP128EMU_USE_XRGB8888
  if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
  {
#ifdef EP128EMU_USE_XRGB8888
    log_cb(RETRO_LOG_ERROR, "XRGB8888 is not supported.\n");
#else
    log_cb(RETRO_LOG_ERROR, "RGB565 is not supported.\n");
#endif // EP128EMU_USE_XRGB8888
    return false;
  }

  check_variables();
  if(info != nullptr)
  {
    if (core)
    {
      delete core;
      core = (Ep128Emu::LibretroCore *) 0;
      vmThread = (Ep128Emu::VMThread *) 0;
      config = (Ep128Emu::EmulatorConfiguration *) 0;
    }
    log_cb(RETRO_LOG_INFO, "Loading game: %s \n",info->path);
    std::string filename(info->path);
    std::string contentExt;
    std::string contentPath;
    std::string contentFile;
    std::string contentBasename;
    std::string configFile;
    std::string configFileExt(".ep128cfg");

    size_t idx = filename.rfind('.');
    if(idx != std::string::npos)
    {
      contentExt = filename.substr(idx+1);
      configFile = filename.substr(0,idx);
      configFile += configFileExt;
      Ep128Emu::stringToLowerCase(contentExt);
    }
    else
    {
      contentExt=""; // No extension found
    }
    log_cb(RETRO_LOG_DEBUG, "Content extension: %s \n",contentExt.c_str());
    Ep128Emu::splitPath(filename,contentPath,contentFile);
    contentBasename = contentFile;
    Ep128Emu::stringToLowerCase(contentBasename);

    int contentLocale = Ep128Emu::LOCALE_UK;
    for(int i=1;i<Ep128Emu::LOCALE_AMOUNT;i++) {
      idx = filename.rfind(Ep128Emu::locale_identifiers[i]);
      if(idx != std::string::npos) {
        contentLocale = i;
        log_cb(RETRO_LOG_INFO, "Locale detected: %s \n",Ep128Emu::locale_identifiers[i].c_str());
        break;
      }
    }

    if(Ep128Emu::does_file_exist(configFile.c_str()))
    {
      log_cb(RETRO_LOG_INFO, "Content specific configuration file: %s \n",configFile.c_str());
    }
    else
    {
      configFile = "";
      log_cb(RETRO_LOG_DEBUG, "No content specific config file exists\n");
    }

    std::string diskExt = "img";
    std::string tapeExt = "tap";
    std::string fileExtDtf = "dtf";
    std::string fileExtTvc = "cas";
    std::string diskExtTvc = "dsk";
    //std::string tapeExtSnd = "notwav";
    //std::string tapeExtZx = "tzx";
    std::string fileExtZx = "tap";
    std::string tapeExtCpc = "cdt";

    std::FILE *imageFile;
    const size_t nBytes = 64;
    uint8_t tmpBuf[nBytes];
    uint8_t tmpBufOffset128[nBytes];
    static const char zeroBytes[nBytes] = "\0";

    imageFile = Ep128Emu::fileOpen(info->path, "rb");
    std::fseek(imageFile, 0L, SEEK_SET);
    if(std::fread(&(tmpBuf[0]), sizeof(uint8_t), nBytes, imageFile) != nBytes)
    {
      throw Ep128Emu::Exception("error reading game content file");
    };
    // TODO: handle seek / read failures
    std::fseek(imageFile, 128L, SEEK_SET);
    if(std::fread(&(tmpBufOffset128[0]), sizeof(uint8_t), nBytes, imageFile) != nBytes)
    {
      log_cb(RETRO_LOG_DEBUG, "Game content file too short for full header analysis\n");
    };
    std::fclose(imageFile);

    static const char *cpcDskFileHeader = "MV - CPCEMU";
    static const char *cpcExtFileHeader = "EXTENDED CPC DSK File";
    static const char *ep128emuTapFileHeader = "\x02\x75\xcd\x72\x1c\x44\x51\x26";
    static const char *epteFileMagic = "ENTERPRISE 128K TAPE FILE       ";
    static const char *tzxFileMagic = "ZXTape!\032\001";
    static const char *tvcDskFileHeader = "\xeb\xfe\x90";
    static const char *epDskFileHeader1 = "\xeb\x3c\x90";
    static const char *epDskFileHeader2 = "\xeb\x4c\x90";
    static const char *epComFileHeader = "\x00\x05";
    static const char *epComFileHeader2 = "\x00\x06";
    static const char *epBasFileHeader = "\x00\x04";
    // static const char *zxTapFileHeader = "\x13\x00\x00\x00";
    // Startup sequence may contain:
    // - chars on the keyboard (a-z, 0-9, few symbols like :
    // - 0xff as wait character
    // - 0xfe as "
    // - 0xfd as F1 (START)
    const char* startupSequence = "";
    bool tapeContent = false;
    bool diskContent = false;
    bool fileContent = false;
    int detectedMachineDetailedType = Ep128Emu::VM_config.at("VM_CONFIG_UNKNOWN");

    // start with longer magic strings - less chance of mis-detection
    if(header_match(cpcDskFileHeader,tmpBuf,11) || header_match(cpcExtFileHeader,tmpBuf,21))
    {
      detectedMachineDetailedType = Ep128Emu::VM_config.at("CPC_DISK");
      diskContent=true;
      startupSequence ="cat\r\xff\xff\xff\xff\xff\xffrun\xfe";
    }
    else if(header_match(tzxFileMagic,tmpBuf,9))
    {
      // if tzx format is called cdt, it is for CPC
      if (contentExt == tapeExtCpc)
      {
        detectedMachineDetailedType = Ep128Emu::VM_config.at("CPC_TAPE");
        tapeContent = true;
        startupSequence ="run\xfe\r\r";
      }
      else
      {
        detectedMachineDetailedType = Ep128Emu::VM_config.at("ZX128_TAPE");
        tapeContent = true;
        startupSequence ="\r";
      }
    }
    else if(header_match(epteFileMagic,tmpBufOffset128,32) || header_match(ep128emuTapFileHeader,tmpBuf,8) /*|| contentExt == tapeExtSnd*/)
    {
      detectedMachineDetailedType = Ep128Emu::VM_config.at("EP128_TAPE");
      tapeContent=true;
      startupSequence =" \xff\xff\xfd";
    }
    else if (contentExt == fileExtTvc && header_match(zeroBytes,&(tmpBuf[5]),nBytes-6))
    {
      detectedMachineDetailedType = Ep128Emu::VM_config.at("TVC64_FILE");
      fileContent=true;
      startupSequence =" \xffload\r";
    }
    // EP and TVC disks may have similar extensions
    else if (contentExt == diskExt || contentExt == diskExtTvc)
    {
      if (header_match(tvcDskFileHeader,tmpBuf,3))
      {
        detectedMachineDetailedType = Ep128Emu::VM_config.at("TVC64_DISK");
        diskContent=true;
        // ext 2 - dir - esc - load"
        startupSequence =" ext 2\r dir\r \x1bload\xfe";
      }
      else if (header_match(epDskFileHeader1,tmpBuf,3) || header_match(epDskFileHeader2,tmpBuf,3))
      {
        detectedMachineDetailedType = Ep128Emu::VM_config.at("EP128_DISK");
        diskContent=true;
      }
      else {
        log_cb(RETRO_LOG_ERROR, "Content format not recognized!\n");
        return false;
      }
    }
    else if (contentExt == fileExtZx /*&& header_match(zxTapFileHeader,tmpBuf,4)*/)
    {
      detectedMachineDetailedType = Ep128Emu::VM_config.at("ZX128_FILE");
      fileContent=true;
      startupSequence ="\r";
    }
    else if (contentExt == fileExtDtf) {
      detectedMachineDetailedType = Ep128Emu::VM_config.at("EP128_FILE_DTF");
      fileContent=true;
      startupSequence =" \xff\xff\xff\xff\xff:dl ";
    }
    // last resort: EP file, first 2 bytes
    else if (header_match(epComFileHeader,tmpBuf,2) || header_match(epComFileHeader2,tmpBuf,2) || header_match(epBasFileHeader,tmpBuf,2))
    {
      detectedMachineDetailedType = Ep128Emu::VM_config.at("EP128_FILE");
      fileContent=true;
      startupSequence =" \xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xfd";
    }
    else
    {
      log_cb(RETRO_LOG_ERROR, "Content format not recognized!\n");
      return false;
    }
    try
    {
      log_cb(RETRO_LOG_DEBUG, "Creating core\n");
      check_variables();
      core = new Ep128Emu::LibretroCore(log_cb, detectedMachineDetailedType, contentLocale, canSkipFrames,
                                        retro_system_bios_directory, retro_system_save_directory,
                                        startupSequence,configFile.c_str(),useHalfFrame, enhancedRom);
      log_cb(RETRO_LOG_DEBUG, "Core created\n");
      config = core->config;
      check_variables();
      if (diskContent)
      {
        config->floppy.a.imageFile = info->path;
        config->floppyAChanged = true;
      }
      if (tapeContent)
      {
        config->tape.imageFile = info->path;
        config->tapeFileChanged = true;
      }
      if (fileContent)
      {
        config->fileio.workingDirectory = contentPath;
        contentFileName=contentPath+contentFile;
        core->vm->setFileNameCallback(&fileNameCallback, NULL);
        config->fileioSettingsChanged = true;
        config->vm.enableFileIO=true;
        config->vmConfigurationChanged = true;
        if( detectedMachineDetailedType == Ep128Emu::VM_config.at("EP128_FILE_DTF") ) {
          core->startSequence += contentBasename+"\r";
        }
      }
      config->applySettings();

      if (tapeContent)
      {
        // ZX tape will be started at the end of the startup sequence
        if (core->machineType == Ep128Emu::MACHINE_ZX || config->tape.forceMotorOn)
        {
        }
        // for other machines, remote control will take care of actual tape control, just start it
        else
        {
          core->vm->tapePlay();
        }
      }
    }
    catch (...)
    {
      log_cb(RETRO_LOG_ERROR, "Exception in load_game\n");
      throw;
    }

    config->setErrorCallback(&cfgErrorFunc, (void *) 0);
    vmThread = core->vmThread;
    log_cb(RETRO_LOG_DEBUG, "Starting core\n");
    core->start();
  }

  return true;
}

void retro_unload_game(void)
{
  try
  {
    config->floppy.a.imageFile = "";
    config->floppyAChanged = true;
    config->applySettings();
  }
  catch (...)
  {
    log_cb(RETRO_LOG_ERROR, "Exception in unload_game\n");
    throw;
  }

}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
  if (type != 0x200)
    return false;
  if (num != 2)
    return false;
  return retro_load_game(NULL);
}

size_t retro_serialize_size(void)
{
  return EP128EMU_SNAPSHOT_SIZE;
}

bool retro_serialize(void *data_, size_t size)
{
  if (size < retro_serialize_size())
    return false;

  memset( data_, 0x00,size);

  Ep128Emu::File  f;
  core->vm->saveState(f);
  f.writeMem(data_, size);

  return true;
}

bool retro_unserialize(const void *data_, size_t size)
{
  if (size < retro_serialize_size())
    return false;

  unsigned char *buf= (unsigned char*)data_;

  // workaround: find last non-zero byte - which will be crc32 of the end-of-file chunk type, 6A 50 08 5E, so essentially the end of content
  size_t lastNonZeroByte = size-1;
  for (size_t i=size-1; i>0; i--)
  {
    if(buf[i] != 0)
    {
      lastNonZeroByte = i;
      break;
    }
  }
  Ep128Emu::File  f((unsigned char *)data_,lastNonZeroByte+1);
  core->vm->registerChunkTypes(f);
  f.processAllChunks();
  core->config->applySettings();
  core->startSequenceIndex = core->startSequence.length();
  if(vmThread) vmThread->resetKeyboard();

  // todo: restore filenamecallback if file is used?
  return true;
}

void *retro_get_memory_data(unsigned id)
{
  (void)id;
  return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
  (void)id;
  return 0;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
  (void)index;
  (void)enabled;
  (void)code;
}

unsigned retro_api_version(void)
{
  return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
  //log_cb(RETRO_LOG_INFO, "Plugging device %u into port %u.\n", device, port);
  std::map< unsigned, std::string>::const_iterator  iter_joytype;
  iter_joytype = Ep128Emu::joystick_type_retrodev.find(device);
  if (port < EP128EMU_MAX_USERS && iter_joytype != Ep128Emu::joystick_type_retrodev.end())
  {
    int userMap[EP128EMU_MAX_USERS] = {
      Ep128Emu::joystick_type.at("DEFAULT"), Ep128Emu::joystick_type.at("DEFAULT"),
      Ep128Emu::joystick_type.at("DEFAULT"), Ep128Emu::joystick_type.at("DEFAULT"),
      Ep128Emu::joystick_type.at("DEFAULT"), Ep128Emu::joystick_type.at("DEFAULT")};

    unsigned mappedDev = Ep128Emu::joystick_type.at((*iter_joytype).second);
    log_cb(RETRO_LOG_INFO, "Mapped device %s for user %u \n", (*iter_joytype).second.c_str(), port);

    userMap[port] = mappedDev;
    if(core)
      core->initialize_joystick_map(std::string(""),std::string(""),std::string(""),-1,userMap[0],userMap[1],userMap[2],userMap[3],userMap[4],userMap[5]);
  }
}

unsigned retro_get_region(void)
{
  return RETRO_REGION_PAL;
}
