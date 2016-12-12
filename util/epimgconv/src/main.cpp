
// epimgconv: Enterprise 128 image converter utility
// Copyright (C) 2008-2016 Istvan Varga <istvanv@users.sourceforge.net>
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
//
// The Enterprise 128 program files generated by this utility are not covered
// by the GNU General Public License, and can be used, modified, and
// distributed without any restrictions.

#include "epimgconv.hpp"
#include "imageconv.hpp"
#include "pixel2.hpp"
#include "pixel4.hpp"
#include "pixel16_1.hpp"
#include "pixel16_2.hpp"
#include "pixel256.hpp"
#include "attr16.hpp"
#include "tvc_2.hpp"
#include "tvc_4.hpp"
#include "tvc_16.hpp"
#include "imgwrite.hpp"

#include <string>
#include <vector>
#include <map>

#include <FL/Fl.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Shared_Image.H>

static int convertColorValue(const char *s, bool isFixBias = false)
{
  if (s == (char *) 0 || s[0] == '\0')
    throw Ep128Emu::Exception("missing color value argument");
  bool    negativeFlag = false;
  bool    decimalFormat = true;
  if (s[0] == '#') {
    decimalFormat = false;
    s++;
  }
  else {
    if (s[0] == '+') {
      s++;
    }
    else if (s[0] == '-') {
      negativeFlag = true;
      s++;
    }
  }
  if (s[0] == '\0')
    throw Ep128Emu::Exception("invalid color number format");
  int     n = 0;
  while (true) {
    char    c = *(s++);
    if (c == '\0')
      break;
    if (!(c >= '0' && c <= '9'))
      throw Ep128Emu::Exception("invalid color number format");
    if (n >= 100)
      throw Ep128Emu::Exception("color value is out of range");
    n = (n * 10) + int(c - '0');
  }
  if (negativeFlag)
    n = (-n);
  if (!decimalFormat) {
    int     r = n / 100;
    int     g = (n / 10) % 10;
    int     b = n % 10;
    if (isFixBias) {
      if (r > 3 || g > 3 || b > 1)
        throw Ep128Emu::Exception("color value is out of range");
      Ep128ImgConv::convertRGBToEPColor(n, r, g, b);
      n = n >> 3;
    }
    else {
      if (r > 7 || g > 7 || b > 3)
        throw Ep128Emu::Exception("color value is out of range");
      Ep128ImgConv::convertRGBToEPColor(n, r, g, b);
    }
  }
  else {
    if (n < -1 || n > (isFixBias ? 31 : 255))
      throw Ep128Emu::Exception("color value is out of range");
  }
  return n;
}

int main(int argc, char **argv)
{
  bool    printUsageFlag = false;
  bool    helpFlag = false;
  bool    endOfOptions = false;
  bool    noCompress = false;
  int     outputFormat = 0;
  try {
    Fl::lock();
    fl_register_images();
    Ep128ImgConv::ImageConvConfig config;
    config.conversionType = 2;
    std::string infileName = "";
    std::string outfileName = "";
    for (int i = 1; i < argc; i++) {
      const char  *s = argv[i];
      if (s == (char *) 0 || s[0] == '\0')
        continue;
      if (endOfOptions || s[0] != '-') {
        if (infileName.length() < 1)
          infileName = s;
        else if (outfileName.length() < 1)
          outfileName = s;
        else
          throw Ep128Emu::Exception("too many filename arguments");
        continue;
      }
      if (std::strcmp(s, "--") == 0) {
        endOfOptions = true;
      }
      else if (std::strcmp(s, "-mode") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-mode'");
        config.conversionType = int(std::atoi(argv[i]));
      }
      else if (std::strcmp(s, "-size") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing arguments for '-size'");
        config.width = int(std::atoi(argv[i]));
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-size'");
        config.height = int(std::atoi(argv[i]));
      }
      else if (std::strcmp(s, "-palres") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-palres'");
        config.paletteResolution = int(std::atoi(argv[i]));
      }
      else if (std::strcmp(s, "-border") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-border'");
        config.borderColor = convertColorValue(argv[i]);
      }
      else if (std::strcmp(s, "-quality") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-quality'");
        config.conversionQuality = int(std::atoi(argv[i]));
      }
      else if (std::strcmp(s, "-chromaerr") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-chromaerr'");
        config.colorErrorScale = float(std::atof(argv[i]));
      }
      else if (std::strcmp(s, "-dither") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing arguments for '-dither'");
        config.ditherType = int(std::atoi(argv[i]));
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-dither'");
        config.ditherDiffusion = float(std::atof(argv[i]));
      }
      else if (std::strcmp(s, "-scalemode") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-scalemode'");
        config.scaleMode = int(std::atoi(argv[i]));
      }
      else if (std::strcmp(s, "-scale") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing arguments for '-scale'");
        config.scaleX = float(std::atof(argv[i]));
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-scale'");
        config.scaleY = float(std::atof(argv[i]));
      }
      else if (std::strcmp(s, "-offset") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing arguments for '-offset'");
        config.offsetX = float(std::atof(argv[i]));
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-offset'");
        config.offsetY = float(std::atof(argv[i]));
      }
      else if (std::strcmp(s, "-nointerp") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-nointerp'");
        config.noInterpolation = bool(std::atoi(argv[i]));
      }
      else if (std::strcmp(s, "-ymin") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-ymin'");
        config.yMin = float(std::atof(argv[i]));
      }
      else if (std::strcmp(s, "-ymax") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-ymax'");
        config.yMax = float(std::atof(argv[i]));
      }
      else if (std::strcmp(s, "-saturation") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-saturation'");
        config.colorSaturationMult = float(std::atof(argv[i]));
      }
      else if (std::strcmp(s, "-gamma") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-gamma'");
        config.gammaCorrection = float(std::atof(argv[i]));
      }
      else if (std::strcmp(s, "-bias") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-bias'");
        config.fixBias = convertColorValue(argv[i], true);
      }
      else if (std::strcmp(s, "-color0") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-color0'");
        config.paletteColors[0] = convertColorValue(argv[i]);
      }
      else if (std::strcmp(s, "-color1") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-color1'");
        config.paletteColors[1] = convertColorValue(argv[i]);
      }
      else if (std::strcmp(s, "-color2") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-color2'");
        config.paletteColors[2] = convertColorValue(argv[i]);
      }
      else if (std::strcmp(s, "-color3") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-color3'");
        config.paletteColors[3] = convertColorValue(argv[i]);
      }
      else if (std::strcmp(s, "-color4") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-color4'");
        config.paletteColors[4] = convertColorValue(argv[i]);
      }
      else if (std::strcmp(s, "-color5") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-color5'");
        config.paletteColors[5] = convertColorValue(argv[i]);
      }
      else if (std::strcmp(s, "-color6") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-color6'");
        config.paletteColors[6] = convertColorValue(argv[i]);
      }
      else if (std::strcmp(s, "-color7") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-color7'");
        config.paletteColors[7] = convertColorValue(argv[i]);
      }
      else if (std::strcmp(s, "-outfmt") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-outfmt'");
        outputFormat = int(std::atoi(argv[i]));
      }
      else if (std::strcmp(s, "-raw") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-raw'");
        outputFormat = int(bool(std::atoi(argv[i])));
      }
      else if (std::strcmp(s, "-nocompress") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-nocompress'");
        noCompress = bool(std::atoi(argv[i]));
      }
      else if (std::strcmp(s, "-h") == 0 ||
               std::strcmp(s, "-help") == 0 ||
               std::strcmp(s, "--help") == 0) {
        helpFlag = true;
        throw Ep128Emu::Exception("");
      }
#ifdef __APPLE__
      else if (std::strncmp(s, "-psn_", 5) == 0) {
        continue;
      }
#endif
      else {
        printUsageFlag = true;
        throw Ep128Emu::Exception("invalid command line option");
      }
    }
    if (infileName.length() < 1 || outfileName.length() < 1) {
      printUsageFlag = true;
      throw Ep128Emu::Exception("missing file name");
    }
    if (config.paletteResolution != 0 && config.paletteResolution != 1) {
      throw Ep128Emu::Exception("palette resolution must be 0 or 1");
    }
    Ep128ImgConv::limitValue(config.width, -255, 255);
    Ep128ImgConv::limitValue(config.height, -16384, 16384);
    if (config.width < 1 || config.height < 1) {
      if (config.width < 1 && config.height < 1)
        throw Ep128Emu::Exception("invalid output image size");
      // width or height is unspecified, calculate from the other value
      // and image aspect ratio
      Fl_Shared_Image *f = Fl_Shared_Image::get(infileName.c_str());
      if (!f)
        throw Ep128Emu::Exception("error opening image file");
      int     w = f->w();
      int     h = f->h();
      f->release();
      f = (Fl_Shared_Image *) 0;
      if (w < 1 || w > 16384 || h < 1 || h > 16384)
        throw Ep128Emu::Exception("invalid input image size");
      double  aspectRatio = double(w) / double(h);
      w = config.width;
      h = config.height;
      if (w < 1) {
        w = int(double(h) * aspectRatio * 0.125 + 0.9);
        if (config.width < 0 && (-w) < config.width) {
          w = -(config.width);
          h = int(double(w) / (aspectRatio * 0.125) + 0.9);
        }
      }
      else {
        h = int(double(w) / (aspectRatio * 0.125) + 0.9);
        if (config.height < 0 && (-h) < config.height) {
          h = -(config.height);
          w = int(double(h) * aspectRatio * 0.125 + 0.9);
        }
      }
      Ep128ImgConv::limitValue(w, 1, 255);
      Ep128ImgConv::limitValue(h, 1, 16384);
      config.width = w;
      config.height = h;
      std::printf("Output image size: %d, %d\n", w, h);
    }
    config.borderColor = config.borderColor & 0xFF;
    Ep128ImgConv::limitValue(config.conversionQuality, 1, 9);
    Ep128ImgConv::limitValue(config.colorErrorScale, 0.05f, 1.0f);
    Ep128ImgConv::limitValue(config.ditherType, 0, 5);
    Ep128ImgConv::limitValue(config.ditherDiffusion, 0.0f, 1.0f);
    Ep128ImgConv::limitValue(config.scaleMode, 0, 1);
    Ep128ImgConv::limitValue(config.scaleX, 0.1f, 10.0f);
    Ep128ImgConv::limitValue(config.scaleY, 0.1f, 10.0f);
    Ep128ImgConv::limitValue(config.offsetX, -10000.0f, 10000.0f);
    Ep128ImgConv::limitValue(config.offsetY, -10000.0f, 10000.0f);
    Ep128ImgConv::limitValue(config.yMin, -0.5f, 1.0f);
    Ep128ImgConv::limitValue(config.yMax, 0.0f, 2.0f);
    Ep128ImgConv::limitValue(config.colorSaturationMult, 0.0f, 8.0f);
    Ep128ImgConv::limitValue(config.gammaCorrection, 0.25f, 4.0f);
    Ep128ImgConv::limitValue(config.fixBias, -1, 31);
    Ep128ImgConv::limitValue(outputFormat, 0, 59);
    if (!((outputFormat >= 0 && outputFormat <= 6) ||
          (outputFormat >= 11 && outputFormat <= 59 &&
           outputFormat != 20 && outputFormat != 30 && outputFormat != 40))) {
      throw Ep128Emu::Exception("invalid output format");
    }
    // TVC video modes are only valid when using TVC KEP output format
    if ((outputFormat >= 50) != ((config.conversionType % 10) >= 7))
      throw Ep128Emu::Exception("invalid video mode for output format");
    if (outputFormat >= 2 && outputFormat <= 5) {
      // non-IVIEW formats do not support interlace
      // Agsys CRF format only supports 4-color PIXEL mode
      // ZozoTools VL/VS format does not support attribute mode
      if (config.conversionType >= 10 ||
          (outputFormat == 2 && config.conversionType != 1) ||
          (outputFormat == 3 && config.conversionType == 6)) {
        throw Ep128Emu::Exception("invalid video mode for output format");
      }
      if (outputFormat >= 3 && outputFormat <= 5) {
        // make sure that the output image size is EXOS compatible
        int     newWidth = config.width;
        int     newHeight = (config.height + 8) / 9;
        Ep128ImgConv::limitValue(newWidth, 1, 42);
        Ep128ImgConv::limitValue(newHeight, 1, 27);
        newHeight = newHeight * 9;
        if (newWidth != config.width || newHeight != config.height) {
          config.width = newWidth;
          config.height = newHeight;
          std::fprintf(stderr, "WARNING: image size changed to %d,%d "
                               "for compatibility with output format\n",
                       newWidth, newHeight);
        }
      }
    }
    for (int i = 0; i < 8; i++)
      Ep128ImgConv::limitValue(config.paletteColors[i], -1, 255);
    int     videoMode = 0;
    int     biasResolution = 0;
    int     interlaceMode = 0;
    int     compressionType = 0;
    if (config.conversionType >= 10 && config.conversionType <= 19) {
      config.conversionType = config.conversionType - 10;
      interlaceMode = 0x9C;
    }
    if (config.paletteResolution == 0)
      interlaceMode = interlaceMode & 0x98;     // fixed palette
    switch (config.conversionType) {
    case 0:                                     // PIXEL / 2 colors
    case 7:                                     // TVC 2 colors
      videoMode = 0x02;
      interlaceMode = interlaceMode & 0x94;
      for (int i = 0; i < 2; i++) {
        if (config.paletteColors[i] < 0)
          break;
        if (i == 1) {
          config.paletteResolution = 0;
          interlaceMode = interlaceMode & 0x90;
        }
      }
      break;
    case 1:                                     // PIXEL / 4 colors
    case 8:                                     // TVC 4 colors
      videoMode = 0x22;
      interlaceMode = interlaceMode & 0x94;
      for (int i = 0; i < 4; i++) {
        if (config.paletteColors[i] < 0)
          break;
        if (i == 3) {
          config.paletteResolution = 0;
          interlaceMode = interlaceMode & 0x90;
        }
      }
      break;
    case 2:                                     // PIXEL / 16 colors
    case 3:
    case 4:
      videoMode = 0x42;
      interlaceMode = interlaceMode & 0x94;
      for (int i = 0; i < 8; i++) {
        if (config.paletteColors[i] < 0)
          break;
        if (i == 7) {
          config.paletteResolution = 0;
          interlaceMode = interlaceMode & 0x90;
        }
      }
      break;
    case 5:                                     // PIXEL / 256 colors
    case 9:                                     // TVC 16 colors
      videoMode = (config.conversionType == 5 ? 0x62 : 0x42);
      config.paletteResolution = 0;
      interlaceMode = interlaceMode & 0x90;
      break;
    case 6:                                     // ATTRIBUTE / 16 colors
      if (config.ditherType >= 4) {
        throw Ep128Emu::Exception("ordered dither is not supported in "
                                  "attribute mode");
      }
      videoMode = 0x04;
      for (int i = 0; i < 8; i++) {
        if (config.paletteColors[i] < 0)
          break;
        if (i == 7) {
          config.paletteResolution = 0;
          interlaceMode = interlaceMode & 0x98;
        }
      }
      break;
    default:
      throw Ep128Emu::Exception("invalid video mode");
    }
    if ((outputFormat >= 3 && outputFormat <= 6) &&
        config.paletteResolution != 0) {
      config.paletteResolution = 0;
      std::fprintf(stderr, "WARNING: output format supports fixed palette "
                           "only, setting -palres 0\n");
    }
    Ep128ImgConv::ImageConverter  *converter =
        (Ep128ImgConv::ImageConverter *) 0;
    Ep128ImgConv::ImageData       imgData(config.width, config.height,
                                          videoMode,  biasResolution,
                                          config.paletteResolution,
                                          interlaceMode, compressionType);
    try {
      switch (config.conversionType) {
      case 0:                                   // PIXEL / 2 colors
        converter = new Ep128ImgConv::ImageConv_Pixel2();
        break;
      case 1:                                   // PIXEL / 4 colors
        converter = new Ep128ImgConv::ImageConv_Pixel4();
        break;
      case 2:                                   // PIXEL / 16 colors
      case 3:
        converter = new Ep128ImgConv::ImageConv_Pixel16_1();
        break;
      case 4:
        converter = new Ep128ImgConv::ImageConv_Pixel16_2();
        break;
      case 5:                                   // PIXEL / 256 colors
        converter = new Ep128ImgConv::ImageConv_Pixel256();
        break;
      case 6:                                   // ATTRIBUTE / 16 colors
        converter = new Ep128ImgConv::ImageConv_Attr16();
        break;
      case 7:                                   // TVC 2 colors
        converter = new Ep128ImgConv::ImageConv_TVCPixel2();
        break;
      case 8:                                   // TVC 4 colors
        converter = new Ep128ImgConv::ImageConv_TVCPixel4();
        break;
      case 9:                                   // TVC 16 colors
        converter = new Ep128ImgConv::ImageConv_TVCPixel16();
        break;
      }
      Ep128ImgConv::YUVImageConverter imgConv;
      imgConv.setScaleMode(config.scaleMode);
      imgConv.setXYScaleAndOffset(config.scaleX, config.scaleY,
                                  config.offsetX, config.offsetY);
      imgConv.setEnableInterpolation(!config.noInterpolation);
      imgConv.setGammaCorrection(config.gammaCorrection, 1.0f);
      imgConv.setLuminanceRange(config.yMin, config.yMax);
      imgConv.setColorSaturation(config.colorSaturationMult);
      imgConv.setImageSize(config.width * 16, config.height * 2);
      imgConv.setPixelAspectRatio(1.0f);
      float   borderY = 0.0f;
      float   borderU = 0.0f;
      float   borderV = 0.0f;
      Ep128ImgConv::convertEPColorToYUV(config.borderColor,
                                        borderY, borderU, borderV, 1.0);
      imgConv.setBorderColor(borderY, borderU, borderV);
      imgData.setBorderColor(config.borderColor);
      if (converter->processImage(imgData, infileName.c_str(),
                                  imgConv, config)) {
        writeConvertedImageFile(outfileName.c_str(), imgData,
                                outputFormat, noCompress);
      }
      delete converter;
      converter = (Ep128ImgConv::ImageConverter *) 0;
    }
    catch (...) {
      if (converter)
        delete converter;
      throw;
    }
    return 0;
  }
  catch (std::exception& e) {
    if (printUsageFlag || helpFlag) {
      std::fprintf(stderr, "Usage: %s [OPTIONS...] <infile> <outfile>\n",
                           argv[0]);
      std::fprintf(stderr, "Options:\n");
      std::fprintf(stderr, "    -h | -help | --help\n");
      std::fprintf(stderr, "        print this message\n");
      std::fprintf(stderr, "    --\n");
      std::fprintf(stderr, "        interpret all remaining arguments as file "
                           "names\n");
      std::fprintf(stderr, "    -mode <N>           (0 to 9, or 10 to 19 for "
                           "interlace; default: 2)\n");
      std::fprintf(stderr, "        select video mode\n");
      std::fprintf(stderr, "    -palres <N>         (0 or 1, default: 1)\n");
      std::fprintf(stderr, "        set palette resolution (0: fixed for the "
                           "whole image, 1: palette\n"
                           "        can be set independently for each line)\n");
      std::fprintf(stderr, "    -outfmt <N>         (0 to 6 or 11 to 59; "
                           "default: 0)\n");
      std::fprintf(stderr, "        write output as program (0) or image data "
                           "file:\n"
                           "            1: IVIEW\n"
                           "            2: Agsys CRF\n"
                           "            3: ZozoTools VL/VS\n"
                           "            4: PaintBox\n"
                           "            5: Zaxial\n"
                           "            6: raw attribute and pixel data only\n"
                           "            11-49: IVIEW with compression type "
                           "N/10 and compression\n"
                           "                   level N%%10\n"
                           "            50-59: TVC KEP format (50: "
                           "uncompressed, 55: RLE)\n");
      std::fprintf(stderr, "    -raw <N>            (0 or 1, default: 0)\n");
      std::fprintf(stderr, "        same as -outfmt, but allows uncompressed "
                           "IVIEW format only\n");
      std::fprintf(stderr, "    -nocompress <N>     (0 or 1, default: 0)\n");
      std::fprintf(stderr, "        disable automatic compression of large "
                           "program files\n");
      std::fprintf(stderr, "    -ymin <MIN>         (default: 0.0)\n");
      std::fprintf(stderr, "    -ymax <MAX>         (default: 1.0)\n");
      std::fprintf(stderr, "        scale RGB input range from 0..1 to "
                           "MIN..MAX\n");
      std::fprintf(stderr, "    -scalemode <N>      (0 or 1, default: 0)\n");
      std::fprintf(stderr, "        0: resize to the maximum possible size "
                           "without clipping,\n"
                           "        1: resize to the minimum possible size "
                           "without empty areas\n");
      std::fprintf(stderr, "    -scale <X> <Y>      (defaults: 1.0, 1.0)\n");
      std::fprintf(stderr, "        scale image size\n");
      std::fprintf(stderr, "    -offset <X> <Y>     (defaults: 0.0, 0.0)\n");
      std::fprintf(stderr, "        set image position offset\n");
      std::fprintf(stderr, "    -nointerp <N>       (0 or 1, default: 0)\n");
      std::fprintf(stderr, "        disable sinc interpolation when resizing "
                           "the input image\n");
      std::fprintf(stderr, "    -saturation <N>     (default: 1.0)\n");
      std::fprintf(stderr, "        color saturation\n");
      std::fprintf(stderr, "    -gamma <N>          (default: 1.0)\n");
      std::fprintf(stderr, "        set gamma correction\n");
      std::fprintf(stderr, "    -dither <M> <S>     (defaults: 1, 0.95)\n");
      std::fprintf(stderr, "        dither mode (0: none, 1: Floyd-Steinberg, "
                           "2: Stucki, 3: Jarvis,\n"
                           "        4: ordered (Bayer), 5: ordered "
                           "(randomized)), and error\n"
                           "        diffusion factor\n");
      std::fprintf(stderr, "    -border <C>         (0 to 255, default: 0)\n");
      std::fprintf(stderr, "        set border color\n");
      std::fprintf(stderr, "    -size <W> <H>       (defaults: 40, 240)\n");
      std::fprintf(stderr, "        set horizontal size in characters, and "
                           "vertical size in lines\n");
      std::fprintf(stderr, "    -chromaerr <N>      "
                           "(0.05 to 1.0, default: 0.5)\n");
      std::fprintf(stderr, "        scale factor applied to squared "
                           "chrominance error\n");
      std::fprintf(stderr, "    -quality <N>        (1 to 9, default: 3)\n");
      std::fprintf(stderr, "        set conversion quality vs. speed\n");
      std::fprintf(stderr, "    -bias <C>           (-1 to 31, default: -1)\n");
      std::fprintf(stderr, "        set FIXBIAS value, or optimize if C = "
                           "-1\n");
      std::fprintf(stderr, "    -colorN <C>         (N: 0 to 7; C: -1 to 255, "
                           "default: -1)\n");
      std::fprintf(stderr, "        set palette color N to C, or optimize if "
                           "C = -1\n");
      std::fprintf(stderr, "Color values can be specified in decimal or #RGB "
                           "format\n");
    }
    if (!helpFlag) {
      const char  *errMsg = e.what();
      if (!errMsg)
        errMsg = "";
      std::fprintf(stderr, " *** epimgconv error: %s\n", errMsg);
      return -1;
    }
    return 0;
  }
  return 0;
}

