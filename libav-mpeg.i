#include "libav.i"
if (is_void(old_mpg)) old_mpg=save(mpeg_create, mpeg_write, mpeg_close, mpeg_movie);
mpeg_create=av_create;
mpeg_write=av_write;
mpeg_close=av_close;
mpeg_movie=av_movie;
