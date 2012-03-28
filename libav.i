if (!is_void(plug_in)) plug_in, "yav";
extern av_create;
/* DOCUMENT obj = av_create(filename)
 *       or obj = av_create(filename, params)
 *
 * Create an movie file FILENAME. Several types are supported,
 * depending on your LibAV installation (e.g. mpg, avi, wmv). Write
 * frames with av_write, close with av_close.  The return value is a
 * LibAV encoder object.
 *
 * If given, PARAMS is [bit_rate, frame_rate, gop_size, max_b_frames]
 *    which default to [ 400000,      25,       10,         1       ]
 * The rates are per second, the gop_size is the number of frames
 * before an I-frame is emitted, and max_b_frames is the largest
 * number of consecutive B-frames.  (The third kind of frame is the
 * P-frame; generally, the encoder emits B-frames until it is forced
 * to emit a P-frame by max_b_frames, or an I-frame by gop_size.  The
 * smaller these numbers, the higher quality the movie, but the lower
 * the compression.)  Any of the four PARAMS values may be zero to
 * get the default value, except for max_b_frames, which should be <0
 * to get the default value.
 *
 * SEE ALSO: av_write, av_close, av_movie
 */

extern av_write;
/* DOCUMENT av_write, obj, rgb
 *
 * Write a RGB frame into the movie file corresponding to the LibAV
 * encoder OBJ returned by av_create. RGB is a 3-by-width-by-height
 * array of char.  Every frame must have the same width and height.
 * To finish the movie and close the file, call av_close.
 *
 * SEE ALSO: av_create, av_close, av_movie
 */

extern av_codec_opt_set;

func av_close(&obj)
/* DOCUMENT av_close, obj
 *
 * Close the movie file corresponding to the LibAV encoder OBJ.  Actually,
 * this merely destroys the reference to the encoder; the file will
 * remain open until the final reference is destroyed.
 *
 * SEE ALSO: av_create, av_write, av_movie
 */
{
  obj = [];
}


func av_movie(filename, draw_frame,time_limit,min_interframe,bracket_time)
/* DOCUMENT av_movie, filename, draw_frame
 *       or av_movie, filename, draw_frame, time_limit
 *       or av_movie, filename, draw_frame, time_limit, min_interframe
 *
 * An extension of the movie function (#include "movie.i") that generates
 * a movie file FILENAME.  The other arguments are the same as the movie
 * function (which see).  The draw_frame function is:
 *
 *   func draw_frame(i)
 *   {
 *     // Input argument i is the frame number.
 *     // draw_frame should return non-zero if there are more
 *     // frames in this movie.  A zero return will stop the
 *     // movie.
 *     // draw_frame must NOT include any fma command if the
 *     // making_movie variable is set (movie sets this variable
 *     // before calling draw_frame)
 *   }
 *
 * SEE ALSO: movie, av_create, av_write, av_close
 */
{
  require, "movie.i";
  _av_movie_obj = av_create(filename, [0, 0, 16, 2]);
  fma = _av_movie_fma;
  _av_movie_count = 0;
  return movie(draw_frame, time_limit, min_interframe, bracket_time);
}

if (is_void(_av_movie_fma0)) _av_movie_fma0 = fma;
func _av_movie_fma
{
  /* movie function does one fma before first draw_frame, skip it */
  if (_av_movie_count++) {
    rgb = rgb_read();
    /* trim image until divisible by 8 */
    n = dimsof(rgb)(3:4) & 7;
    if (anyof(n))
      rgb = rgb(,n(1)/2+1:-(n(1)+1)/2,n(2)/2+1:-(n(2)+1)/2);
    av_write, _av_movie_obj, rgb;
  }
  _av_movie_fma0;
}


extern __av_init;
__av_init;
