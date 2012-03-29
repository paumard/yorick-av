/*
 * zlibav-auto.i - auto-loads for libav.i
 * 
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

autoload, "libav.i";
autoload, "libav.i", libav, av_create, av_write, av_close, av_movie;
autoload, "avtest.i";
autoload, "avtest.i", avtest;

// be nice to mpeg.i: if autoloads are already in place, don't overide them
// this file should be parsed after mpeg, ympeg, yompeg or yorz 
autoload, "libav-mpeg.i";
if (!is_void(mpeg_create))
  autoload, "libav-mpeg.i", mpeg_create, mpeg_write, mpeg_close, mpeg_movie;
