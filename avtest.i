/*
 * mpgtest.i -- $Id: mpgtest.i,v 1.1 2007-10-27 22:18:21 dhmunro Exp $
 * test yorick mpeg encoder
 */
if (open("libav.i", "r", 1)) plug_dir, _("./", plug_dir());
require, "libav.i";

require, "movie.i";
if (is_void(_orig_movie)) _orig_movie = movie;
require, "demo2.i";

func avtest(void)
{
  if (is_void(_avtest_name)) _avtest_name = "test.ogg";
  movie = _avtest_movie;
  demo2, 3;
}

func _avtest_movie(__f, __a, __b, __c)
{
  movie = _orig_movie;
  return av_movie(_avtest_name, __f, __a, __b, __c);
}

func avtest_draw_frame(i)
{
  x   = span(0, 2*pi, 100);
  nframes=200.;
  y   = sin(x-2.*pi*i/nframes);
  plg, y, x;
  return i<=nframes;
}

func avtest2(fname)
{
  if (is_void(fname)) fname = "test2.ogg";
  av_movie, fname, avtest_draw_frame;
}
