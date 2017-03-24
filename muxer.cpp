#include <iostream>
extern "C" {
#include <libavutil/error.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}
#include "muxer.h"
Muxer::Muxer(){
}

Muxer::~Muxer()
{

}
bool Muxer::init(const char* _fileName, unsigned int _width, unsigned int _height, const char *_codecName)
{
    this->width = _width;
    this->height = _height;
	// Initialize libavcodec, and register all codecs and formats
	av_register_all();
    avformat_alloc_output_context2(&this->pOutputAvFormatContext, NULL, _codecName, _fileName);
    if (this->pOutputAvFormatContext == false) {
		printf("Could not deduce output format from file extension");
		return false;
	}
    this->pAvOutputFormat = this->pOutputAvFormatContext->oformat;
	// add the video stream using default format codecs and initialize the codec
    bool returnValue = addVideoStream(NULL);
//    returnValue &= openVideo();
    av_dump_format(this->pOutputAvFormatContext, 0, _fileName, 1);
    // open the output file
    if(avio_open(&this->pOutputAvFormatContext->pb, _fileName, AVIO_FLAG_WRITE) < 0) {
        std::cerr << "Could not open " << _fileName << std::endl;
        return false;
    }
    // Write the stream header, if any!
    if(avformat_write_header(this->pOutputAvFormatContext, &this->avDict) < 0) {
        std::cerr << "Error occured when openning output file" << std::endl;
        return false;
    }
    this->pInputAvFormatContext = this->pOutputAvFormatContext;
    return returnValue;
}

bool Muxer::init(const char* _fileName, AVFormatContext *_pInputAvFormatContext, bool _isH264)
{

    this->pInputAvFormatContext = _pInputAvFormatContext;
    this->pAvOutputFormat = av_guess_format("mp4", NULL, NULL);
    avformat_alloc_output_context2(&this->pOutputAvFormatContext, this->pAvOutputFormat, 0, 0);
//    if(_isH264)
//        avformat_alloc_output_context2(&this->pOutputAvFormatContext, 0, 0, _fileName);
//    else
//        avformat_alloc_output_context2(&this->pOutputAvFormatContext, 0, 0, _fileName);

    if (this->pOutputAvFormatContext == false) {
        printf("Could not deduce output format from file extension");
        return false;
    }

//    this->pAvOutputFormat = this->pOutputAvFormatContext->oformat;
    if(_isH264)
        this->pAvOutputFormat->video_codec = AV_CODEC_ID_H264;
    // create all streams with their codecs in pAvFormatContext
    for (int cnt = 0; cnt < this->pInputAvFormatContext->nb_streams; cnt++) {
        AVStream *inputStream = this->pInputAvFormatContext->streams[cnt];
        if(inputStream->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            this->width = inputStream->codec->width;
            this->height = inputStream->codec->height;
            if(_isH264) {
                if(addVideoStream(inputStream) == false)
                    return false;
                continue;
            }
        }
        AVStream *outputStream = avformat_new_stream(this->pOutputAvFormatContext, inputStream->codec->codec);
        if(outputStream == NULL) {
            std::cerr << "Failed to allocate output stream." << std::endl;
            return false;
        }
        int returnValue = avcodec_copy_context(outputStream->codec, inputStream->codec);
        if(returnValue < 0) {
            std::cerr << "Failed to copy codec context from input to output." << std::endl;
            return false;
        }
        outputStream->codec->codec_tag = 0;
        if(this->pOutputAvFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
            outputStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
           }

    av_dump_format(this->pOutputAvFormatContext, 0, _fileName, 1);
    if((this->pAvOutputFormat->flags & AVFMT_NOFILE) == false) {
        if(avio_open(&this->pOutputAvFormatContext->pb, _fileName, AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Could not open " << _fileName << std::endl;
            return false;
        }
    } else {
        std::cerr << "openning file for writing has not been set" << std::endl;
        return false;
    }

    // Write the stream header, if any!
    if(avformat_write_header(this->pOutputAvFormatContext, NULL) < 0) {
        std::cerr << "Error occured when openning output file" << std::endl;
        return false;
    }
    return true;
}

void Muxer::writeFrame(char* frameData, unsigned int frameSize)
{
//    this->videoStream.frame->pts = this->videoStream.nextPts++;
    AVPacket avPacket;
    av_new_packet(&avPacket, frameSize);
    avPacket.data = (uint8_t *)frameData;
    avPacket.size = frameSize;
    av_packet_rescale_ts(&avPacket, this->pOutputAvFormatContext->streams[0]->codec->time_base, this->pOutputAvFormatContext->streams[0]->time_base);
    this->writeFrame(avPacket);
////    AVCodecContext* codecContext;
////    codecContext = this->videoStream.avCodecCtx;
//    avPacket.stream_index = this->pOutputAvFormatContext->streams[0]->index;
////    AVFormatContext* tmpFmtCtx = this->pOutputAvFormatContext;
//   int returnValue = av_interleaved_write_frame(this->pOutputAvFormatContext, &avPacket);
//   if(returnValue < 0)
//       std::cerr << "Error while writing video frame" << std::endl;

}

void Muxer::writeFrame(AVPacket &_packet)
{

    AVStream *inputStream = this->pInputAvFormatContext->streams[_packet.stream_index];
    AVStream *outputStream = this->pOutputAvFormatContext->streams[_packet.stream_index];
    // copy packet
    _packet.pts = av_rescale_q_rnd(_packet.pts, inputStream->time_base, outputStream->time_base, AVRounding(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
    _packet.dts = av_rescale_q_rnd(_packet.dts, inputStream->time_base, outputStream->time_base, AVRounding(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
    _packet.duration = av_rescale_q(_packet.duration, inputStream->time_base, outputStream->time_base);
    _packet.pos = -1;
    int returnValue = av_interleaved_write_frame(this->pOutputAvFormatContext, &_packet);
    if(returnValue < 0)
        std::cerr << "Error in muxing a packet";
//    av_packet_unref(&_packet);


}

void Muxer::close()
{
    av_write_trailer(this->pOutputAvFormatContext);
    if ((this->pAvOutputFormat->flags & AVFMT_NOFILE) == false)
        /* Close the output file. */
        avio_closep(&this->pOutputAvFormatContext->pb);
    avformat_free_context(this->pOutputAvFormatContext);
}


bool Muxer::addVideoStream(AVStream *_inputStream)
{
    AVCodecContext* codecContext;
    AVCodec *videoCodec = avcodec_find_encoder(AV_CODEC_ID_H264/*this->pAvOutputFormat->video_codec*/);
    if(videoCodec == false) {
        std::cerr << "Could not find encoder for " << this->pAvOutputFormat->video_codec << std::endl;
        return false;
    }
    AVStream *videoStream = avformat_new_stream(this->pOutputAvFormatContext, videoCodec);
    if(videoStream == false) {
        std::cerr << "Could not allocate stream" << std::endl;
        return false;
    }

    videoStream->id = this->pOutputAvFormatContext->nb_streams - 1;
    codecContext = avcodec_alloc_context3(videoCodec);
    if(codecContext == NULL) {
        std::cerr << "Could not allocate an encoding context" << std::endl;
        return false;
     }
    codecContext->codec_id = AV_CODEC_ID_H264;// this->pAvOutputFormat->video_codec;

    codecContext->bit_rate = _inputStream->codec->bit_rate;
	/* Resolution must be a multiple of two. */
    codecContext->width    = _inputStream->codec->width;
    codecContext->height   = _inputStream->codec->height;
	/* timebase: This is the fundamental unit of time (in seconds) in terms
	 * of which frame timestamps are represented. For fixed-fps content,
	 * timebase should be 1/framerate and timestamp increments should be
	 * identical to 1. */
    videoStream->time_base = _inputStream->codec->time_base;
    codecContext->time_base       = videoStream->time_base;
    codecContext->framerate = _inputStream->codec->framerate;

    codecContext->gop_size      = _inputStream->codec->gop_size; /* emit one intra frame every twelve frames at most */
    codecContext->pix_fmt       = _inputStream->codec->pix_fmt;
	if (codecContext->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
		/* just for testing, we also add B-frames */
        codecContext->max_b_frames = _inputStream->codec->max_b_frames;
	}
	if (codecContext->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
		/* Needed to avoid using macroblocks in which some coeffs overflow.
		 * This does not happen with normal video, it just happens here as
		 * the motion of the chroma plane does not match the luma plane. */
        codecContext->mb_decision = _inputStream->codec->mb_decision;
	}
    if(this->pOutputAvFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
        codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    videoStream->codec = codecContext;
//    int returnValue = avcodec_copy_context(videoStream->codec, codecContext);
////    videoStream->codec = codecContext;
//    if(returnValue < 0) {
//        std::cerr << "Failed to copy codec context from input to output." << std::endl;
//        return false;
//    }

//    if (!videoStream->codec->codec_tag)
//        {
//            if (! this->pOutputAvFormatContext->oformat->codec_tag
//                || av_codec_get_id (pOutputAvFormatContext->oformat->codec_tag, this->pInputAvFormatContext->streams[0]->codec->codec_tag) == videoStream->codec->codec_id
//                || av_codec_get_tag(pOutputAvFormatContext->oformat->codec_tag, this->pInputAvFormatContext->streams[0]->codec->codec_id) <= 0)
//                        videoStream->codec->codec_tag = this->pInputAvFormatContext->streams[0]->codec->codec_tag;
//        }
//    av_codec_get_tag(, AV_CODEC_ID_H264);
//    videoStream->codec->codec_tag = 0;
//    if(this->pOutputAvFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
//        videoStream->codec->codec_tag |= AV_CODEC_FLAG_GLOBAL_HEADER;
    return true;
}

