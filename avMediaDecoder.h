#ifndef AVMEDIADECODER_H
#define AVMEDIADECODER_H
extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
class clsAvMediaDecoder {
public:
    clsAvMediaDecoder();
    ~clsAvMediaDecoder();
    bool init(AVFormatContext *_pAvFormatContext, int _videoStreamIndex = -1);
    bool init(uint _width, uint _height, enum AVCodecID _codecId);
    bool decodeFrame(AVPacket _avPacket, unsigned char *_decodedFrameData);
    uint get(int propID);
    private:
    int openCodecContext(int *streamIdx, AVFormatContext *fmtCtx, enum AVMediaType type);

private:
    int videoStreamIndex = -1;
    int audioStreamIdx = -1;
    enum AVPixelFormat pixelFormat;
    AVCodecContext *pAvVideoCodecContext = NULL;
    AVCodecContext *pAvAudioCodecContext = NULL;
    AVFrame *yuvFrame = NULL;
    AVFrame *rgbFrame = NULL;
    uint frameNumber;
    int rgbSize;
    SwsContext* convertRgbYuvcontext = NULL;


};

#endif
