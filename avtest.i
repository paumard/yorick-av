/*
 * avtest.i - test / example file for libav.i 
 * This file is part of yorick-av, a Yorick plu-ing to write movies
 * using LibAV/FFmpeg.
 *
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
 */
if (open("libav.i", "r", 1)) plug_dir, _("./", plug_dir());
require, "libav.i";

func avtest_draw_frame(i)
{
  x   = span(0, 2*pi, 100);
  nframes=200.;
  y   = sin(x-2.*pi*i/nframes);
  plg, y, x;
  return i<=nframes;
}

func avtest(fname, params=, vcodec=, pix_fmt=, b=, r=, g=, bf=)
{
  if (is_void(fname)) fname = "avtest2.ogg";
  winkill;
  window;
  range, -1, 1;
  av_movie, fname, avtest_draw_frame, params=params,
    vcodec=vcodec, pix_fmt=pix_fmt,
    b=b, r=r, g=g, bf=bf;
  winkill;
}
