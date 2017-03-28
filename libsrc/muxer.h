#ifndef MUXER_H
#define MUXER_H
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
#define STREAM_PIX_FMT AV_PIX_FMT_YUV420P
#define STREAM_FRAME_RATE 30


class Muxer {
public:
    Muxer();
    ~Muxer();
    bool init(const char* _fileName, unsigned int _width, unsigned int _height, const char *_codecName = "H264");
    bool init(const char* _fileName, AVFormatContext *_pInputAvFormatContext, bool _isH264 = true);
    void writeFrame(char* frameData, unsigned int frameSize, int64_t _pts = 0);
    void writeFrame(AVPacket &_packet);
    void close();
private:
    bool openVideo();
    bool addVideoStream(AVStream *_inputStream);
    bool addVideoStream();
    bool allocPicture(enum AVPixelFormat pix_fmt, int width, int height, AVFrame* picture);

private:
    AVOutputFormat* pAvOutputFormat;
    AVFormatContext* pOutputAvFormatContext;
    AVFormatContext* pInputAvFormatContext;
    AVDictionary* avDict = NULL;
    unsigned int width;
    unsigned int height;
    int64_t frameNumber = 0;
    bool isInitialized = false;

};


#endif

