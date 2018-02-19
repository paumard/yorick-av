/*
 * yav.c
 * This file is part of yorick-av, a Yorick plu-ing to write movies
 * using LibAV/FFmpeg. It is based on the famous FFmpeg example file
 * example-output.c by Fabrice Bellard and keeps its license.
 *
 * Copyright (c) 2003 Fabrice Bellard
 * Copyright (c) 2012-2013, 2016, 2018 Thibaut Paumard
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
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libavutil/opt.h>

/* default parameter values */
#define YAV_BIT_RATE 400000
#define YAV_FRAME_RATE   24
#define YAV_GOP_SIZE     25
#define YAV_MAX_B_FRAMES 16
#define YAV_PIX_FMT AV_PIX_FMT_YUV420P /* default pix_fmt */

inline int yav_arg_set(iarg) { return (iarg >= 0) && !yarg_nil(iarg); }

typedef struct yav_ctxt {
  AVFrame *picture, *tmp_picture;
  uint8_t *video_outbuf;
  int frame_count, video_outbuf_size;
  AVOutputFormat *fmt;
  AVFormatContext *oc;
  AVStream *audio_st, *video_st;
  struct SwsContext *img_convert_ctx;
  AVCodec *codec;
  int open;
} yav_ctxt;
void yav_free(void*obj);
static y_userobj_t yav_ops = {"LibAV object", &yav_free, 0, 0, 0, 0};

void yav_opencodec(yav_ctxt *obj, unsigned int width, unsigned int height);

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
  obj->codec=0;
  obj->open=0;
  //  audio_st=0;
  return obj;
}

void yav_write_frame(yav_ctxt * obj, AVFrame * frame) {
  int ret = 0.;
  /* Set frame to NULL to close movie */
  AVCodecContext * c = obj->video_st->codec;
  /* encode the image */
  ret = avcodec_send_frame(c, frame);
  if (ret<0)
    y_errorn("Error submitting frame for encoding: %d", ret);
  while (ret >=0) {
    AVPacket pkt = { 0 };
    av_init_packet(&pkt);
    ret = avcodec_receive_packet(c, &pkt);
    if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
      y_errorn( "Error encoding a video frame: %d", ret);
    } else if (ret >= 0) {
      av_packet_rescale_ts(&pkt, c->time_base, obj->video_st->time_base);
      pkt.stream_index = obj->video_st->index;
      /* Write the compressed frame to the media file. */
      ret = av_interleaved_write_frame(obj->oc, &pkt);
      if (ret < 0) {
	y_errorn("Error while writing video frame: %d", ret);
      }
    }
  }
}

void yav_free(void*obj_) {
  yav_ctxt * obj = (yav_ctxt *)obj_;
  if (obj->open) {
    yav_write_frame(obj, NULL); // flush buffer of encoded frames
    av_write_trailer(obj->oc);
  }
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
  static long default_params[]=
    {YAV_BIT_RATE, YAV_FRAME_RATE, YAV_GOP_SIZE, YAV_MAX_B_FRAMES};
  long *params=default_params;

  // PARSE ARGUMENTS: SEPARATE KEYWORDS FROM POSITIONAL ARGUMENTS
  static char * knames[] = {
    "vcodec", "pix_fmt", "b", "r", "g", "bf", 0
  };
#define YAC_CREATE_NKW 6
  static long kglobs[YAC_CREATE_NKW+1];
  int kiargs[YAC_CREATE_NKW];
  int piargs[]={-1, -1};

  yarg_kw_init(knames, kglobs, kiargs);
  int iarg=argc-1, parg=0;
  while (iarg>=0) {
    iarg = yarg_kw(iarg, kglobs, kiargs);
    if (iarg>=0) {
      if (parg<2) piargs[parg++]=iarg--;
      else y_error("av_create takes at most 2 positional arguments");
    }
  }

  // INTERPRET POSITIONAL ARGUMENTS
  // filename (mandatory)
  if ((iarg=piargs[0])<0) y_error("FILENAME must be specified");
  char *filename = ygets_q(iarg);

  // params vector (optional)
  if (yav_arg_set(iarg=piargs[1])) {
    long ntot ;
    long dims[Y_DIMSIZE]={0,0};
    params = ygeta_l(iarg, &ntot, dims);
    if (dims[0]!=1 || dims[1]!=4)
      y_error("bad dimensions  for PARAMS vector");
    if (params[0]<0 || params[1]<0 || params[2]<0)
      y_error("bad values in PARAMS vector");
  }

  // INTERPRET KEYWORD ARGUMENTS
  char* vcodec = NULL, *pix_fmt = NULL;
  int k=0;
  if (yav_arg_set(iarg=kiargs[k++])) vcodec  = ygets_q(iarg); // vcodec
  if (yav_arg_set(iarg=kiargs[k++])) pix_fmt = ygets_q(iarg); // pix_fmt
  if (yav_arg_set(iarg=kiargs[k++])) params[0] = ygets_l(iarg); // b
  if (yav_arg_set(iarg=kiargs[k++])) params[1] = ygets_l(iarg); // r
  if (yav_arg_set(iarg=kiargs[k++])) params[2] = ygets_l(iarg); // g
  if (yav_arg_set(iarg=kiargs[k++])) params[3] = ygets_l(iarg); // bf

  // PUSH RETURN VALUE
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
  if (obj->oc->oformat->video_codec != AV_CODEC_ID_NONE) {
    AVCodecContext *c;
    obj->video_st = avformat_new_stream(obj->oc, NULL);
    c = obj->video_st->codec;
    if (vcodec) {
      obj->codec = avcodec_find_encoder_by_name(vcodec);
      if (!obj->codec) y_error("can't find requested codec");
      c->codec_id = obj->codec->id;
    } else {
      c->codec_id = obj->oc->oformat->video_codec;
      obj->codec = avcodec_find_encoder(c->codec_id);
      if (!obj->codec) y_error("default codec not found");
    }
    c->codec_type = AVMEDIA_TYPE_VIDEO;

    avcodec_get_context_defaults3(c, obj->codec);
    if (c->codec_id == AV_CODEC_ID_NONE) c->codec_id = obj->codec->id;

    /* put sample parameters */
    c->width   = 0;
    c->height  = 0;
    c->pix_fmt = pix_fmt ? av_get_pix_fmt(pix_fmt) : YAV_PIX_FMT; 
    c->bit_rate      =  params[0]     ? params[0] : YAV_BIT_RATE;
    c->time_base.den =  params[1]     ? params[1] : YAV_FRAME_RATE;
    c->time_base.num =  1;
    obj->video_st->time_base.den =  c->time_base.den;
    obj->video_st->time_base.num =  c->time_base.num;
    c->gop_size      =  params[2]     ? params[2] : YAV_GOP_SIZE;
    c->max_b_frames  = (params[3]>=0) ? params[3] : YAV_MAX_B_FRAMES;
    if(obj->oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    // codec-specific limitations
    switch (c->codec_id) {
    case AV_CODEC_ID_RAWVIDEO:
    case AV_CODEC_ID_GIF:
      if (!pix_fmt) c->pix_fmt = AV_PIX_FMT_RGB24;
      break;
    case AV_CODEC_ID_MSMPEG4V3:
    case AV_CODEC_ID_H263:
    case AV_CODEC_ID_H263P:
    case AV_CODEC_ID_RV10:
    case AV_CODEC_ID_RV20:
    case AV_CODEC_ID_FLV1:
    case AV_CODEC_ID_ASV1:
    case AV_CODEC_ID_ASV2:
      c->max_b_frames = 0;
      break;
    default:;
    }
  }

  if (!(obj->oc->oformat->flags & AVFMT_RAWPICTURE)) {
    obj->video_outbuf_size = 200000;
    obj->video_outbuf = av_malloc(obj->video_outbuf_size);
  }

}

void yav_opencodec(yav_ctxt *obj, unsigned int width, unsigned int height) {
  obj->video_st->codec->width=width;
  obj->video_st->codec->height=height;
  if (obj->video_st->codec->codec_id == AV_CODEC_ID_MPEG1VIDEO ||
      obj->video_st->codec->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
    AVCPBProperties *props;
    props = (AVCPBProperties*) av_stream_new_side_data
      (obj->video_st, AV_PKT_DATA_CPB_PROPERTIES, sizeof(*props));
    props->buffer_size = width*height*4;
    props->max_bitrate = 0;
    props->min_bitrate = 0;
    props->avg_bitrate = 0;
    props->vbv_delay = UINT64_MAX;
  }
  av_dump_format(obj->oc, 0, obj->oc->filename, 1);

  if (obj->video_st) {
    AVCodecContext *c;
    c = obj->video_st->codec;

    if (avcodec_open2(c, obj->codec, NULL) < 0)
      y_error("could not open codec\n");

    avcodec_parameters_from_context(obj->video_st->codecpar, obj->video_st->codec);

    obj->picture = av_frame_alloc();
    if (!obj->picture)
      y_error("Could not allocate picture");

    int size = avpicture_get_size(c->pix_fmt, c->width, c->height);
    uint8_t *picture_buf = av_malloc(size);
    if (!picture_buf) {
      av_frame_free(&obj->picture);
      y_error("unable to allocate memory");
    }
    avpicture_fill((AVPicture *)obj->picture, picture_buf,
                   c->pix_fmt, c->width, c->height);
    obj->picture->width=c->width;
    obj->picture->height=c->height;
    obj->picture->format=c->pix_fmt;
    if (obj->oc->oformat->video_codec == AV_CODEC_ID_H264 ||
	obj->oc->oformat->video_codec == AV_CODEC_ID_THEORA) obj->picture->pts=-1;


    /* if the output format is not RGB24, then a temporary RGB24
       picture is needed too. It is then converted to the required
       output format */
    if (c->pix_fmt != AV_PIX_FMT_RGB24) {
      obj->tmp_picture = av_frame_alloc();
      if (!obj->tmp_picture) {
	y_error("Could not allocate picture");
      }
      size = avpicture_get_size(AV_PIX_FMT_RGB24, c->width, c->height);
      uint8_t *tmp_picture_buf = av_malloc(size);
      if (!tmp_picture_buf) {
	av_frame_free(&obj->tmp_picture);
	av_frame_free(&obj->picture);
	y_error("unable to allocate memory");
      }
      avpicture_fill((AVPicture *)obj->tmp_picture, tmp_picture_buf,
		     AV_PIX_FMT_RGB24, c->width, c->height);
      obj->tmp_picture->width=c->width;
      obj->tmp_picture->height=c->height;
      obj->tmp_picture->format=c->pix_fmt;
    }
  }

  /* open the output file, if needed */
  if (!(obj->oc->oformat->flags & AVFMT_NOFILE))
    if (avio_open(&obj->oc->pb, obj->oc->filename, AVIO_FLAG_WRITE) < 0)
      y_errorq("Could not open '%s'", obj->oc->filename);

  obj->open = 1;

  /* write the stream header, if any */
  int ret = avformat_write_header(obj->oc, NULL);
  if (ret<0)
    y_errorn("Error writing header: %d", ret);

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

  const uint8_t *src[4] = {data, 0, 0, 0};
  int src_linesizes[4] = {3*c->width,0,0,0};

  if (c->pix_fmt != AV_PIX_FMT_RGB24) {
    /* as we only generate a RGB24 picture, we must convert it
       to the codec pixel format if needed */
    obj->img_convert_ctx = sws_getCachedContext(obj->img_convert_ctx,
						c->width, c->height,
						AV_PIX_FMT_RGB24,
						c->width, c->height,
						c->pix_fmt,
						SWS_BICUBIC, NULL, NULL, NULL);
    if (obj->img_convert_ctx == NULL)
      y_error("Cannot initialize the conversion context");

    av_image_copy(obj->tmp_picture->data, obj->tmp_picture->linesize,
		  src, src_linesizes, AV_PIX_FMT_RGB24, c->width, c->height);
    sws_scale(obj->img_convert_ctx,
	      (const uint8_t * const*)obj->tmp_picture->data,
	      obj->tmp_picture->linesize,
	      0, c->height, obj->picture->data, obj->picture->linesize);
  } else {
    av_image_copy(obj->picture->data, obj->picture->linesize,
		  src, src_linesizes, AV_PIX_FMT_RGB24, c->width, c->height);
  }

  /* encode the image */
  if (obj->oc->oformat->flags & AVFMT_RAWPICTURE)
    y_error("RAW picture not supported");

  if (obj->oc->oformat->video_codec == AV_CODEC_ID_H264 ||
      obj->oc->oformat->video_codec == AV_CODEC_ID_THEORA) ++obj->picture->pts;

  if (obj->oc->oformat->flags & AVFMT_RAWPICTURE) {
    int ret=0;
    /* Raw video case - directly store the picture in the packet */
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.flags |= AV_PKT_FLAG_KEY;
    pkt.stream_index = obj->video_st->index;
    pkt.data= obj->video_outbuf;
    // pkt.size= out_size;
    //    pkt.data = dst_picture.data[0];
    pkt.size = sizeof(AVPicture);
    av_packet_rescale_ts(&pkt, c->time_base, obj->video_st->time_base);
    ret = av_interleaved_write_frame(obj->oc, &pkt);
    if (ret<0)
      y_errorn("Error writing frame: %d", ret);
  } else {
    yav_write_frame(obj, obj->picture);
  }

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
