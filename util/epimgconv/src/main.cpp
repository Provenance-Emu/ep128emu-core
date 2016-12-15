
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
#include "imgwrite.hpp"
#include "img_cfg.hpp"

#include <string>

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
  try {
    Fl::lock();
    fl_register_images();
    Ep128ImgConv::ImageConvConfig config;
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
        config["conversionType"] = int(std::atoi(argv[i]));
      }
      else if (std::strcmp(s, "-size") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing arguments for '-size'");
        config["width"] = int(std::atoi(argv[i]));
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-size'");
        config["height"] = int(std::atoi(argv[i]));
      }
      else if (std::strcmp(s, "-palres") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-palres'");
        config["paletteResolution"] = int(std::atoi(argv[i]));
      }
      else if (std::strcmp(s, "-border") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-border'");
        config["borderColor"] = convertColorValue(argv[i]);
      }
      else if (std::strcmp(s, "-quality") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-quality'");
        config["conversionQuality"] = int(std::atoi(argv[i]));
      }
      else if (std::strcmp(s, "-chromaerr") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-chromaerr'");
        config["colorErrorScale"] = std::atof(argv[i]);
      }
      else if (std::strcmp(s, "-dither") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing arguments for '-dither'");
        config["ditherType"] = int(std::atoi(argv[i]));
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-dither'");
        config["ditherDiffusion"] = std::atof(argv[i]);
      }
      else if (std::strcmp(s, "-scalemode") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-scalemode'");
        config["scaleMode"] = int(std::atoi(argv[i]));
      }
      else if (std::strcmp(s, "-scale") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing arguments for '-scale'");
        config["scaleX"] = std::atof(argv[i]);
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-scale'");
        config["scaleY"] = std::atof(argv[i]);
      }
      else if (std::strcmp(s, "-offset") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing arguments for '-offset'");
        config["offsetX"] = std::atof(argv[i]);
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-offset'");
        config["offsetY"] = std::atof(argv[i]);
      }
      else if (std::strcmp(s, "-nointerp") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-nointerp'");
        config["noInterpolation"] = bool(std::atoi(argv[i]));
      }
      else if (std::strcmp(s, "-ymin") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-ymin'");
        config["yMin"] = std::atof(argv[i]);
      }
      else if (std::strcmp(s, "-ymax") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-ymax'");
        config["yMax"] = std::atof(argv[i]);
      }
      else if (std::strcmp(s, "-saturation") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-saturation'");
        config["colorSaturationMult"] = std::atof(argv[i]);
      }
      else if (std::strcmp(s, "-gamma") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-gamma'");
        config["gammaCorrection"] = std::atof(argv[i]);
      }
      else if (std::strcmp(s, "-bias") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-bias'");
        config["fixBias"] = convertColorValue(argv[i], true);
      }
      else if (std::strncmp(s, "-color", 6) == 0 &&
               s[6] >= '0' && s[6] <= '7' && s[7] == '\0') {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-color'");
        config[std::string("color") + s[6]] = convertColorValue(argv[i]);
      }
      else if (std::strcmp(s, "-outfmt") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-outfmt'");
        config.setOutputFormat(int(std::atoi(argv[i])));
      }
      else if (std::strcmp(s, "-raw") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-raw'");
        config.setOutputFormat(int(bool(std::atoi(argv[i]))));
      }
      else if (std::strcmp(s, "-nocompress") == 0) {
        if (++i >= argc)
          throw Ep128Emu::Exception("missing argument for '-nocompress'");
        config["noCompress"] = bool(std::atoi(argv[i]));
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
    Ep128ImgConv::ImageData *imgData =
        Ep128ImgConv::convertImage(infileName.c_str(), config);
    if (imgData) {
      try {
        writeConvertedImageFile(outfileName.c_str(), *imgData,
                                config.getOutputFormat(), config.noCompress);
      }
      catch (...) {
        delete imgData;
        throw;
      }
      delete imgData;
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

