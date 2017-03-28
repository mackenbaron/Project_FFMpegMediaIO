#include <iostream>
#include "avMediaDecoder.h"
extern "C"{
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
}

static AVFrame * icv_alloc_picture_FFMPEG(int pix_fmt, int width, int height, bool alloc)
{
    AVFrame * picture;
    uint8_t * picture_buf;
    int size;

    picture = av_frame_alloc();
    if (!picture)
        return NULL;
    size = avpicture_get_size( (AVPixelFormat) pix_fmt, width, height);
    if(alloc)
    {
        picture_buf = (uint8_t *) malloc(size);
        if (!picture_buf)
        {
            av_frame_free (&picture);
            std::cout << "picture buff = NULL" << std::endl;
            return NULL;
        }
        avpicture_fill((AVPicture *)picture, picture_buf,  (AVPixelFormat) pix_fmt, width, height);
    }
    return picture;
}



clsAvMediaDecoder::clsAvMediaDecoder()
{
    avcodec_register_all();
}

clsAvMediaDecoder::~clsAvMediaDecoder()
{
    avcodec_close(this->pAvVideoCodecContext);
    if(this->rgbFrame != NULL)
        av_frame_free(&this->rgbFrame);
    if(this->yuvFrame != NULL)
        av_frame_free(&this->yuvFrame);
    if(this->convertRgbYuvcontext != NULL)
        sws_freeContext(this->convertRgbYuvcontext);
}
#define COLOR_PIXEL_FORMAT AV_PIX_FMT_RGB24
bool clsAvMediaDecoder::init(AVFormatContext *_pAvFormatContext, int _videoStreamIndex)
{

    // \TODO Right now it doesn't handle multiple streams of even video
    if(_videoStreamIndex == -1)
        if (openCodecContext(&this->videoStreamIndex, _pAvFormatContext, AVMEDIA_TYPE_VIDEO) >= 0) {
            AVStream* videoStream = _pAvFormatContext->streams[this->videoStreamIndex];
            this->pAvVideoCodecContext = videoStream->codec;
        } else {
            std::cerr << "could not open any video stream!!!" << std::endl;
            return false;
        }
    else {
        AVStream* videoStream = _pAvFormatContext->streams[_videoStreamIndex];
        this->pAvVideoCodecContext = videoStream->codec;
        this->videoStreamIndex = _videoStreamIndex;
    }
    /* allocate image where the decoded image will be put */
    this->pixelFormat = this->pAvVideoCodecContext->pix_fmt;
    this->rgbSize = avpicture_get_size(COLOR_PIXEL_FORMAT, this->pAvVideoCodecContext->width, this->pAvVideoCodecContext->height);

    this->convertRgbYuvcontext = sws_getContext(this->pAvVideoCodecContext->width,
                                                this->pAvVideoCodecContext->height,
                                                this->pixelFormat,
                                                this->pAvVideoCodecContext->width,
                                                this->pAvVideoCodecContext->height,
                                                COLOR_PIXEL_FORMAT,
                                                SWS_FAST_BILINEAR,
                                                NULL, NULL, NULL);
    this->yuvFrame = icv_alloc_picture_FFMPEG(this->pixelFormat, this->pAvVideoCodecContext->width, this->pAvVideoCodecContext->height, true);
    this->rgbFrame = icv_alloc_picture_FFMPEG(COLOR_PIXEL_FORMAT, this->pAvVideoCodecContext->width, this->pAvVideoCodecContext->height, true);

    return true;

    /* // \TODO audio  encoder is not needed yet!!
    if (openCodecContext(&this->audioStreamIdx, _pAvFormatContext, AVMEDIA_TYPE_AUDIO) >= 0) {
        this->audioStream = _pAvFormatContext->streams[this->audioStreamIdx];
        this->audioDecoderCtx = this->audioStream->codec;
    } else {
        std::cout << "could not open any audio file" << std::endl;
    }
     if (!this->audioStream && !this->videoStream) {
        std::cerr << "Could not find audio or video stream in the input, aborting" << std::endl;
        return false;
    }
    */


}

bool clsAvMediaDecoder::init(uint _width, uint _height, AVCodecID _codecId)
{
    avcodec_register_all();
    // \TODO We assume all video pixel formats are YUV420P. I don't know why!
    AVCodec* avCodec = avcodec_find_decoder(_codecId);
    this->pAvVideoCodecContext = avcodec_alloc_context3(avCodec);

    this->pixelFormat = AV_PIX_FMT_YUV420P;
    this->rgbSize = avpicture_get_size(COLOR_PIXEL_FORMAT, pAvVideoCodecContext->width, pAvVideoCodecContext->height);
    this->pAvVideoCodecContext->width = _width;
    this->pAvVideoCodecContext->height = _height;
    this->pAvVideoCodecContext->extradata = NULL;
    this->pAvVideoCodecContext->pix_fmt = this->pixelFormat;
    avcodec_open2(this->pAvVideoCodecContext, avCodec, NULL);
    this->convertRgbYuvcontext = sws_getContext(this->pAvVideoCodecContext->width, this->pAvVideoCodecContext->height,
                                          this->pixelFormat, pAvVideoCodecContext->width,
                                          this->pAvVideoCodecContext->height, COLOR_PIXEL_FORMAT,
                                          SWS_FAST_BILINEAR, NULL, NULL, NULL);

    this->yuvFrame = icv_alloc_picture_FFMPEG(this->pixelFormat, this->pAvVideoCodecContext->width, this->pAvVideoCodecContext->height, true);
    this->rgbFrame = icv_alloc_picture_FFMPEG(COLOR_PIXEL_FORMAT, this->pAvVideoCodecContext->width, this->pAvVideoCodecContext->height, true);

}

bool clsAvMediaDecoder::reinit(AVFormatContext* _pAvFormatContext)
{
//    avcodec_close(this->pAvVideoCodecContext);
    if(this->rgbFrame != NULL)
        av_frame_free(&this->rgbFrame);
    if(this->yuvFrame != NULL)
        av_frame_free(&this->yuvFrame);
    if(this->convertRgbYuvcontext != NULL)
        sws_freeContext(this->convertRgbYuvcontext);
    pAvVideoCodecContext = NULL;
    pAvAudioCodecContext = NULL;
    yuvFrame = NULL;
    rgbFrame = NULL;
    convertRgbYuvcontext = NULL;
    return init(_pAvFormatContext);
}


bool clsAvMediaDecoder::decodeFrame(AVPacket _avPacket, unsigned char *_decodedFrameData)
{
    int gotFrame = 0;
    /* decode video frame */
    int returnValue = avcodec_decode_video2(this->pAvVideoCodecContext, this->yuvFrame, &gotFrame, &_avPacket);
    if (returnValue < 0) {
        std::cerr << "Error decoding video frame " << std::endl;
        return false;
    }

    if (gotFrame == 0) {
        if (this->yuvFrame->width != this->pAvVideoCodecContext->width ||
                this->yuvFrame->height != this->pAvVideoCodecContext->height ||
                this->yuvFrame->format != this->pixelFormat)
        {
            return false;
        }
    } else {

        //Convert the frame from YUV420 to RGB24
        sws_scale(this->convertRgbYuvcontext,
                  this->yuvFrame->data,
                  this->yuvFrame->linesize, 0,
                  this->pAvVideoCodecContext->height,
                  this->rgbFrame->data,
                  this->rgbFrame->linesize);

        //Manadatory function to copy the image form an AVFrame to a generic buffer.
        avpicture_layout((AVPicture *)this->rgbFrame, COLOR_PIXEL_FORMAT,
                         this->pAvVideoCodecContext->width, this->pAvVideoCodecContext->height, _decodedFrameData, this->rgbSize);
        return true;
    }
    return false;
}

uint clsAvMediaDecoder::get(int propId)
{
    switch(propId) {
    case 0: // width
        return this->pAvVideoCodecContext->width;
    case 1: // height
        return this->pAvVideoCodecContext->height;
    case 3: // decodedSize
        return this->rgbSize;
    default:
        return 0;
    }

    return 0;

}

int clsAvMediaDecoder::openCodecContext(int *streamIdx, AVFormatContext *fmtCtx, enum AVMediaType type)
{
    int ret, stream_index;
    AVStream *st;
    AVCodecContext *dec_ctx = NULL;
    AVCodec *dec = NULL;
    AVDictionary *opts = NULL;

    ret = av_find_best_stream(fmtCtx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find stream in input file \n");
        return ret;
    } else {
        stream_index = ret;
        st = fmtCtx->streams[stream_index];

        /* find decoder for the stream */
        dec_ctx = st->codec;
        dec = avcodec_find_decoder(dec_ctx->codec_id);
        if (!dec) {
            fprintf(stderr, "Failed to find %s codec\n",
                    av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }

        /* Init the decoders, with or without reference counting */
        av_dict_set(&opts, "refcounted_frames", "1", 0);
        if ((ret = avcodec_open2(dec_ctx, dec, &opts)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }
        *streamIdx = stream_index;
    }

    return 0;

}
