/*
 * yav.c
 * This file is part of yorick-av, a Yorick plu-ing to write movies
 * using LibAV/FFmpeg. It is based on the famous FFmpeg example file
 * example-output.c by Fabrice Bellard and keeps its license.
 *
 * Copyright (c) 2003 Fabrice Bellard
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

#include "yapi.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>

#ifndef AVIO_FLAG_WRITE
#  define AVIO_FLAG_WRITE AVIO_WRONLY
#endif

#if (LIBAVFORMAT_VERSION_MAJOR < 53) || \
  ((LIBAVFORMAT_VERSION_MAJOR == 53) && (LIBAVFORMAT_VERSION_MINOR < 9))
#  define avformat_new_stream(a, b) av_new_stream(a, 0)
#endif
#if (LIBAVUTIL_VERSION_MAJOR < 50) || \
  ((LIBAVUTIL_VERSION_MAJOR == 50) && (LIBAVUTIL_VERSION_MINOR < 44))
  // av_opt_set missing at least until this version
  int av_opt_set (void *obj, const char * name, const char * val,
		  int search_flags)
  { y_error("av_opt_set unimplemented in this libav/ffmpeg"); return -1; }
#endif

/* default parameter values */
#define YAV_BIT_RATE 400000
#define YAV_FRAME_RATE 24
#define YAV_GOP_SIZE 10
#define YAV_MAX_B_FRAMES 1
#define YAV_PIX_FMT PIX_FMT_YUV420P /* default pix_fmt */


typedef struct yav_ctxt {
  AVFrame *picture, *tmp_picture;
  uint8_t *video_outbuf;
  int frame_count, video_outbuf_size;
  AVOutputFormat *fmt;
  AVFormatContext *oc;
  AVStream *audio_st, *video_st;
  struct SwsContext *img_convert_ctx;

} yav_ctxt;
void yav_free(void*obj);
static y_userobj_t yav_ops = {"LibAV object", &yav_free, 0, 0, 0, 0};

void yav_opencodec(yav_ctxt *obj, unsigned int width, unsigned int height);
void yav_h264preset(AVCodecContext *ctx);

yav_ctxt *ypush_av()
{
  yav_ctxt * obj = (yav_ctxt *)ypush_obj(&yav_ops, sizeof(yav_ctxt));
  obj->picture=0;
  obj->tmp_picture=0;
  obj->video_outbuf=0;
  obj->frame_count=0;
  obj->video_outbuf_size=0;
  obj->oc=0;
  obj->video_st=0;
  obj->img_convert_ctx = 0;
  //  audio_st=0;
}

void yav_free(void*obj_) {
  yav_ctxt * obj = (yav_ctxt *)obj_;
  av_write_trailer(obj->oc);
  if (obj->picture){
    av_free(obj->picture->data[0]);
    av_free(obj->picture);
  }
  if (obj->tmp_picture){
    av_free(obj->tmp_picture->data[0]);
    av_free(obj->tmp_picture);
  }
  if (obj->video_outbuf) {
    av_free(obj->video_outbuf);
  }
  //if (obj->frame_count);
  //  if (obj->video_outbuf_size);
  if (obj->video_st) {
    avcodec_close(obj->video_st->codec);
    obj->video_st=0;
  }
  if (obj->oc) {
    avformat_free_context(obj->oc);
    obj->oc=0;
  }
  if (obj->img_convert_ctx) {
    sws_freeContext(obj->img_convert_ctx);
    obj->img_convert_ctx=0;
  }
}

void
Y_av_create(int argc)
{
  char *filename = ygets_q(argc-1);
  long *params = 0;
  if (argc>=2) {
    long ntot ;
    long dims[Y_DIMSIZE]={0,0};
    params = ygeta_l(argc-2, &ntot, dims);
    if (dims[0]!=1 || dims[1]!=4)
      y_error("bad dimensions  for PARAMS vector");
    if (params[0]<0 || params[1]<0 || params[2]<0)
      y_error("bad values in PARAMS vector");
  }

  yav_ctxt * obj = ypush_av();

  /* allocate the output media context */
  obj->oc = avformat_alloc_context();
  if (!obj->oc) {
    y_error("Memory error");
  }

  /* auto detect the output format from the name. default is
     mpeg. */
  obj->oc->oformat = av_guess_format(NULL, filename, NULL);
  if (!obj->oc->oformat) {
    y_warn("Could not deduce output format from file extension: using MPEG.");
    obj->oc->oformat = av_guess_format("mpeg", NULL, NULL);
  }
  if (!obj->oc->oformat) {
    y_error("Could not find suitable output format.");
  }
  snprintf(obj->oc->filename, sizeof(obj->oc->filename), "%s", filename);

  /* add the audio and video streams using the default format codecs
     and initialize the codecs */
  obj->video_st = NULL;
  //  audio_st = NULL;
  if (obj->oc->oformat->video_codec != CODEC_ID_NONE) {
    AVCodecContext *c;
    obj->video_st = avformat_new_stream(obj->oc, NULL);
    c = obj->video_st->codec;
    c->codec_id = obj->oc->oformat->video_codec;
    c->codec_type = AVMEDIA_TYPE_VIDEO;

    /* put sample parameters */
    c->bit_rate = (params && params[0])? params[0] : YAV_BIT_RATE;
    /* resolution must be a multiple of two */
    c->width = 0;
    c->height = 0;
    /* time base: this is the fundamental unit of time (in seconds) in terms
       of which frame timestamps are represented. for fixed-fps content,
       timebase should be 1/framerate and timestamp increments should be
       identically 1. */
    c->time_base.den = (params && params[1])? params[1] : YAV_FRAME_RATE;
    c->time_base.num = 1;
    c->gop_size = (params && params[2])? params[2] : YAV_GOP_SIZE;
    c->pix_fmt = YAV_PIX_FMT;
    c->max_b_frames = (params && params[3]>=0)? params[3] : YAV_MAX_B_FRAMES;
    if(obj->oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;


    if (obj->oc->oformat->video_codec == CODEC_ID_MSMPEG4V3) {
      // MSMPEG4 dosen't support B frames
      c->max_b_frames = 0;
    } else if (obj->oc->oformat->video_codec == CODEC_ID_H264) {
      yav_h264preset(c);
      if (params && params[0])     c->bit_rate      = params[0];
      if (params && params[1])     c->time_base.den = params[1];
      if (params && params[2])     c->gop_size      = params[2];
      if (params && params[3]>=0)  c->max_b_frames  = params[3];
    }

  }

  /*
    if (obj->oc->oformat->audio_codec != CODEC_ID_NONE) {
    audio_st = add_audio_stream(obj->oc, obj->oc->oformat->audio_codec);
    }
   */
    if (!(obj->oc->oformat->flags & AVFMT_RAWPICTURE)) {
        /* allocate output buffer */
        /* XXX: API change will be done */
        /* buffers passed into lav* can be allocated any way you prefer,
           as long as they're aligned enough for the architecture, and
           they're freed appropriately (such as using av_free for buffers
           allocated with av_malloc) */
        obj->video_outbuf_size = 200000;
        obj->video_outbuf = av_malloc(obj->video_outbuf_size);
    }

}

void yav_h264preset(AVCodecContext *ctx) {
  //      av_opt_set( c->priv_data, "preset" , "fast",  0);
  ctx->bit_rate = 500*1000;
  ctx->bit_rate_tolerance = 0;
  ctx->rc_max_rate = 0;
  ctx->rc_buffer_size = 0;
  ctx->gop_size = 40;
  ctx->max_b_frames = 3;
  ctx->b_frame_strategy = 1;
  ctx->coder_type = 1;
  ctx->me_cmp = 1;
  ctx->me_range = 16;
  ctx->qmin = 10;
  ctx->qmax = 51;
  ctx->scenechange_threshold = 40;
  ctx->flags |= CODEC_FLAG_LOOP_FILTER;
  ctx->me_method = ME_HEX;
  ctx->me_subpel_quality = 5;
  ctx->i_quant_factor = 0.71;
  ctx->qcompress = 0.6;
  ctx->max_qdiff = 4;
  ctx->directpred = 1;
  ctx->flags2 |= CODEC_FLAG2_FASTPSKIP;
/*
  // libx264-medium.ffpreset preset
  ctx->coder_type = 1;  // coder = 1
  ctx->flags|=CODEC_FLAG_LOOP_FILTER;   // flags=+loop
  ctx->me_cmp|= 1;  // cmp=+chroma, where CHROMA = 1
  ctx->partitions|=X264_PART_I8X8+X264_PART_I4X4+X264_PART_P8X8+X264_PART_B8X8; // partitions=+parti8x8+parti4x4+partp8x8+partb8x8
  ctx->me_method=ME_HEX;    // me_method=hex
  ctx->me_subpel_quality = 7;   // subq=7
  ctx->me_range = 16;   // me_range=16
  ctx->gop_size = 250;  // g=250
  ctx->keyint_min = 25; // keyint_min=25
  ctx->scenechange_threshold = 40;  // sc_threshold=40
  ctx->i_quant_factor = 0.71; // i_qfactor=0.71
  ctx->b_frame_strategy = 1;  // b_strategy=1
  ctx->qcompress = 0.6; // qcomp=0.6
  ctx->qmin = 10;   // qmin=10
  ctx->qmax = 51;   // qmax=51
  ctx->max_qdiff = 4;   // qdiff=4
  ctx->max_b_frames = 3;    // bf=3
  ctx->refs = 3;    // refs=3
  ctx->directpred = 1;  // directpred=1
  ctx->trellis = 1; // trellis=1
  ctx->flags2|=CODEC_FLAG2_BPYRAMID+CODEC_FLAG2_MIXED_REFS+CODEC_FLAG2_WPRED+CODEC_FLAG2_8X8DCT+CODEC_FLAG2_FASTPSKIP;  // flags2=+bpyramid+mixed_refs+wpred+dct8x8+fastpskip
  ctx->weighted_p_pred = 2; // wpredp=2
*/
}

void yav_opencodec(yav_ctxt *obj, unsigned int width, unsigned int height) {
  obj->video_st->codec->width=width;
  obj->video_st->codec->height=height;
  av_dump_format(obj->oc, 0, obj->oc->filename, 1);

  /* now that all the parameters are set, we can open the audio and
     video codecs and allocate the necessary encode buffers */
  if (obj->video_st) {
    AVCodec *codec;
    AVCodecContext *c;

    c = obj->video_st->codec;

    /* find the video encoder */
    codec = avcodec_find_encoder(c->codec_id);
    if (!codec) {
        y_error("codec not found");
    }


    /* open the codec */
    if (avcodec_open2(c, codec, NULL) < 0) {
        y_error("could not open codec\n");
    }


    /* allocate the encoded raw picture */
    obj->picture = avcodec_alloc_frame();
    if (!obj->picture) {
      y_error("Could not allocate picture");
    }
    int size = avpicture_get_size(c->pix_fmt, c->width, c->height);
    uint8_t *picture_buf = av_malloc(size);
    if (!picture_buf) {
      av_freep(obj->picture);
      y_error("unable to allocate memory");
    }
    avpicture_fill((AVPicture *)obj->picture, picture_buf,
                   c->pix_fmt, c->width, c->height);
    if (obj->oc->oformat->video_codec == CODEC_ID_H264 ||
	obj->oc->oformat->video_codec == CODEC_ID_THEORA) obj->picture->pts=-1;


    /* if the output format is not RGB24, then a temporary RGB24
       picture is needed too. It is then converted to the required
       output format */
    if (c->pix_fmt != PIX_FMT_RGB24) {
    obj->tmp_picture = avcodec_alloc_frame();
    if (!obj->tmp_picture) {
      y_error("Could not allocate picture");
    }
    size = avpicture_get_size(PIX_FMT_RGB24, c->width, c->height);
    uint8_t *tmp_picture_buf = av_malloc(size);
    if (!tmp_picture_buf) {
      av_freep(obj->tmp_picture);
      av_freep(obj->picture);
      y_error("unable to allocate memory");
    }
    avpicture_fill((AVPicture *)obj->tmp_picture, tmp_picture_buf,
                   PIX_FMT_RGB24, c->width, c->height);
    }

  }
  /*
    if (audio_st)
    open_audio(oc, audio_st);
   */

  /* open the output file, if needed */
  if (!(obj->oc->oformat->flags & AVFMT_NOFILE)) {
    if (avio_open(&obj->oc->pb, obj->oc->filename, AVIO_FLAG_WRITE) < 0) {
      y_errorq("Could not open '%s'", obj->oc->filename);
    }
  }

  /* write the stream header, if any */
  avformat_write_header(obj->oc, NULL);

}


void
Y_av_codec_opt_set(int argc)
{
  yav_ctxt * obj = yget_obj(argc-1, &yav_ops);
  AVCodecContext *c = obj->video_st->codec;

  char* name = ygets_q(argc-2);
  char* val = ygets_q(argc-3);

  ypush_long(av_opt_set(c, name, val, AV_OPT_SEARCH_CHILDREN ));

}


void
Y_av_write(int argc)
{
  yav_ctxt * obj = yget_obj(argc-1, &yav_ops);
  AVCodecContext *c = obj->video_st->codec;

  long ntot=0;
  long dims[Y_DIMSIZE]={0,0};
  uint8_t *data = ygeta_c(argc-2, &ntot, dims);

  if (!c->width)
    yav_opencodec(obj, dims[2], dims[3]);

  if (dims[0]!=3 || dims[1]!=3 || 
      dims[2]!=c->width || dims[3]!=c->height)
    y_error("DATA should be an array(char, 3, width, height)");

  long npix=dims[2]*dims[3];

  uint8_t *src[4] = {data, 0, 0, 0};
  int src_linesizes[4] = {3*c->width,0,0,0};

  if (c->pix_fmt != PIX_FMT_RGB24) {
    /* as we only generate a RGB24 picture, we must convert it
       to the codec pixel format if needed */
    obj->img_convert_ctx = sws_getCachedContext(obj->img_convert_ctx,
						c->width, c->height,
						PIX_FMT_RGB24,
						c->width, c->height,
						c->pix_fmt,
						SWS_BICUBIC, NULL, NULL, NULL);
    if (obj->img_convert_ctx == NULL)
      y_error("Cannot initialize the conversion context");

    av_image_copy(obj->tmp_picture->data, obj->tmp_picture->linesize,
    		  src, src_linesizes, PIX_FMT_RGB24, c->width, c->height);
    sws_scale(obj->img_convert_ctx, (const uint8_t * const*)obj->tmp_picture->data,
	      obj->tmp_picture->linesize,
	      0, c->height, obj->picture->data, obj->picture->linesize);
  } else {
    av_image_copy(obj->picture->data, obj->tmp_picture->linesize,
		  src, src_linesizes, PIX_FMT_RGB24, c->width, c->height);
  }


  /* encode the image */
  if (obj->oc->oformat->flags & AVFMT_RAWPICTURE)
    y_error("RAW picture not supported");

  if (obj->oc->oformat->video_codec == CODEC_ID_H264 ||
      obj->oc->oformat->video_codec == CODEC_ID_THEORA) ++obj->picture->pts;

  int out_size
    = avcodec_encode_video(c, obj->video_outbuf, obj->video_outbuf_size,
			   obj->picture);
  int ret = 0;
  /* if zero size, it means the image was buffered */
  if (out_size > 0) {
    AVPacket pkt;
    av_init_packet(&pkt);

    if (c->coded_frame->pts != AV_NOPTS_VALUE)
      pkt.pts= av_rescale_q(c->coded_frame->pts, c->time_base,
			    obj->video_st->time_base);
    if(c->coded_frame->key_frame)
      pkt.flags |= AV_PKT_FLAG_KEY;
    pkt.stream_index= obj->video_st->index;
    pkt.data= obj->video_outbuf;
    pkt.size= out_size;
    /* write the compressed frame in the media file */
    ret = av_interleaved_write_frame(obj->oc, &pkt);
  }

  if (ret != 0)
    y_errorn("Error while writing video frame: %d", ret);

  /* return [] */
  ypush_nil();
}

/* currently it's just obj = []
void
Y_av_close(int argc)
{
}
*/

void
Y___av_init(int argc)
{
  /* initialize libavcodec, and register all codecs and formats */
  av_register_all();
}
