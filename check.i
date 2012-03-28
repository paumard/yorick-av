if (open("libav.i", "r", 1)) plug_dir, _("./", plug_dir());
#include "libav.i"
"obj created";

exts=["mpg", "avi", "ogg", "mkv", "mp4", "mov", "h264", "wmv", "vob"];

for (e=1; e<=numberof(exts); ++e) {
  fname="tmp."+exts(e);
  write, format="==========================================\n"+
                "     testing extenstion: '%s'\n"+
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

#include "avtest.i"
avtest;
avtest2, "test2.mpg";
avtest2, "test2.mp4";

include,"mpgtest.i", 3;
#include "libav-mpeg.i"
if (is_func(mpgtest)) mpgtest;

quit;
