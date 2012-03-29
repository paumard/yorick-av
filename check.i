/*
 * check.i - Check / example file for yorick-libav
 * This file is part of yorick-libav, a Yorick plu-ing to write movies
 * using LibAV/FFmpeg.
 *
 * ============================================================================
 * Copyright (c) 2012 Thibaut Paumard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * ============================================================================
 *
 * This script should create a number of movie files called
 * libavcheck.* with the same content encoded in various formats using
 * various codecs.
 *
 * Those movies should show a red band coming from the left (brighter
 * at the bottom), then yellow stuff comes, and finally white. The
 * movie ends when the red bar reaches the middle of the image.
 *
 * Unless the environment variable YAV_NODISPLAY is set, a window
 * should pop-up displaying a moving sine curve. This movie will be
 * saved as libavtest.*.
 *
 * If, in addition, mpgtest.i (from the ympeg plug-in) can be located,
 * it will be run, the mpeg_* functions replaced by their av_*
 * counterparts using libav-mpeg.i. This will create a file named
 * test.mpg.
 * 
 */

if (open("libav.i", "r", 1)) plug_dir, _("./", plug_dir());
#include "libav.i"

exts=["mpg", "avi", "ogg", "mkv", "mp4", "mov", "h264", "wmv", "vob"];

for (e=1; e<=numberof(exts); ++e) {
  fname="libavcheck."+exts(e);
  write, format="==========================================\n"+
                "     testing extension: '%s'\n"+
                "==========================================\n", exts(e);
  obj=av_create(fname);

  data = array(char, 3, 704, 288);

  for (i=1; i<=352; ++i) {
    data(1, i, ) = span(0,255,288);
    if (i>100) data(2,i-100,) = span(0,255,288);
    if (i>200) data(3,i-200,) = span(0,255,288);
    av_write, obj, data;
  }
  
  write, format="done, closing file '%s'\n", fname;
  av_close, obj;
 }

if (!get_env("YAV_NODISPLAY")) {

  require, "avtest.i";
  avtest, "libavtest.mpg";
  avtest, "libavtest.mp4";

  include,"mpgtest.i", 3;
  require, "libav-mpeg.i";
  if (is_func(mpgtest)) mpgtest;
 }
if (batch()) quit;

