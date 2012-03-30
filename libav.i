if (!is_void(plug_in)) plug_in, "yav";
local libav;
/* DOCUMENT #include "libav.i"
 *  
 * This is yorick-av, a Yorick plug-ing to write movies using
 * LibAV/FFmpeg.
 *
 * This plug-in uses the LibAV / FFmpeg family of libraries
 * (libavformat, libavcodec, libavutils, libswscale...) to write
 * movies from within Yorick.
 *
 * In essence, it is a compatible rewrite of the yompeg plug-in, but
 * instead of shipping its own, trimmed-down version of FFmpeg, libav
 * uses the full-featured LibAV (or FFmpeg). This allows using more
 * recent codecs than MPEG1, such as h.264, MP4 / DivX, ogg/vorbis
 * (Theora). yorick-av can write in about any format supported by
 * LibAV: mpeg, mp4, avi, mov, ogg, mkv.
 *
 * libav-mpeg.i provides a drop-in replacement for mpeg.i
 * avtest.i contains a limited test suite
 *
 * COPYRIGHT AND LICENSE INFORMATION
 * The C code is based on the famous FFmpeg example file
 * example-output.c by Fabrice Bellard
 *
 * Copyright (c) 2003 Fabrice Bellard (for yav.c)
 * Copyright (c) 2012 Thibaut Paumard (for yav.c, *libav*.i)
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
 *
 * SEE ALSO: av_create, av_write, av_close, av_movie, avtest
 */

extern av_create;
/* DOCUMENT encoder = av_create(filename)
 *       or encoder = av_create(filename, params)
 *
 * Create a LibAV encoder object to write a movie to disk. The type of
 * file container is determine by FILENAME's extension (e.g. ogg, mpg,
 * avi, mkv, mov, wmv, h264). The video codec is LibAV's default for
 * that container.
 *
 * Frames can then be added to the movie using av_write and the file
 * is closed using av_close.
 *
 * For compatibility with mpeg_create, 4 parameters can be specified
 * as one positional argument:
 *    PARAMS = [b, r, g, bf]
 *
 * KEYWORDS
 * av_create accepts a few keywords corresponding to eponymous
 * options in avconv/ffmpeg, see "avconv -h" or "ffmpeg -h".
 *  vcodec:  string naming a specific video codec. If not specified,
 *           the default for FILENAME's extension will be used.
 *  pix_fmt: output pixel format, e.g. "rgb24", "yuv420p"... see
 *           avconv -pix_fmts
 *  b:       bit rate.   Default: 400000
 *  r:       frame rate. Default: 25
 *  g:       group of picture (a.k.a. gop) size. Default: 25
 *  bf:      max number of consecutive B frames. Default: 16.
 *
 * SEE ALSO: av_write, av_close, av_movie
 */

extern av_write;
/* DOCUMENT av_write, encoder, rgb
 *
 * Write the frame RGB to movie file using ENCODER, an LibAV encoder
 * object created using av_create.
 *
 * RGB must be a 3xWIDTHxHEIGHT array of char and every frame must
 * have the same WIDTH and HEIGHT. Certain codecs may have specific
 * requirements on WIDTH and HEIGHT. In particular, they must be even
 * numbers in all(?) cases, and using mutliples of 8 helps.
 *
 * Beware that movie frames are stored in top-to-bottom order. RGB
 * should be flipped vertically if pli displays it correctly:
 *    rbg = rgb(,,::-1)
 *
 * Once all frames have been fed to the ENCODER, close the movie using
 * av_close.
 *
 * SEE ALSO: av_create, av_close, av_movie
 */

extern av_codec_opt_set;
/* DOCUMENT av_codec_opt_set, encoder, parname, value
 *
 * Experimental, has never been shown to work.
 *
 * Set parameter PARNAME to VALUE (both strings) in the codec stored
 * in ENCODER.
 *
 * SEE ALSO: av_create, av_write, av_close, av_movie
 */


func av_close(&obj)
/* DOCUMENT av_close, encoder
 *
 * Close ENCODER, a LibAV encoder object previously created using
 * av_write. Currently, it merely destroys the object but it should
 * still be used for forward compatibility.
 *
 * Destroying the last reference to ENCODER writes the last, buffered
 * frames in the file and writes whatever trailer is required by the
 * video codec or container file format.
 *
 * SEE ALSO: av_create, av_write, av_movie
 */
{
  obj = [];
}


func av_movie(filename, draw_frame,time_limit,min_interframe,bracket_time,
              params=, vcodec=, pix_fmt=, b=, r=, g=, bf=)
/* DOCUMENT av_movie, filename, draw_frame
 *       or av_movie, filename, draw_frame, time_limit
 *       or av_movie, filename, draw_frame, time_limit, min_interframe
 *
 * A wrapper around the movie function in movie.i, which writes the
 * animation to file FILENAME using the av_* family of functions from
 * libav.i.
 *
 * KEYWORDS
 *  av_movie accepts the same keywords as av_create. In addition,
 *  av_create's PARAM argument can be passed as a keyword to av_movie.
 *
 * SEE ALSO: movie, av_create
 */
{
  require, "movie.i";
  if (is_void(params)) params=[0, 0, 16, 2];
  _av_movie_encoder = av_create(filename, params, vcodec=vcodec,
                                pix_fmt=pix_fmt, b=b, r=r, g=g, bf=bf);
  fma = _av_movie_fma;
  _av_movie_count = 0;
  return movie(draw_frame, time_limit, min_interframe, bracket_time);
}

if (is_void(_av_movie_real_fma)) _av_movie_real_fma = fma;
func _av_movie_fma
/* xDOCUMENT _av_movie_fma

   Called internally by av_movie.
   
   Writes image displayed in current graphic window to
   _av_movie_encoder and performs a real fma. Image is trimmed until
   its size is a multple of 8.

   SEE ALSO: movie, av_write, fma
*/
{
  if (_av_movie_count++) {
    img = rgb_read();
    rest = dimsof(img) % 8;
    if (anyof(rest)) {
      img = img(,rest(3)/2+1:-(rest(3)+1)/2,rest(4)/2+1:-(rest(4)+1)/2);
    }
    av_write, _av_movie_encoder, img;
  }
  _av_movie_real_fma;
}

// intilialize plug-in (register codecs et al.)
extern __av_init;
__av_init;
