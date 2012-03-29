/*
 * libav-mpeg.i - compatibility layer to expose av_* routines as mpeg_*
 * Use this as a drop-in replacemant for mpeg.i (from the ympeg plug-in).
 *
 * This file is part of yorick-libav, a Yorick plu-ing to write movies
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
#include "libav.i"
if (is_void(old_mpg))
  old_mpg=save(old_mpg, mpeg_create, mpeg_write, mpeg_close, mpeg_movie);
mpeg_create=av_create;
mpeg_write=av_write;
mpeg_close=av_close;
mpeg_movie=av_movie;
