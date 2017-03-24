#ifndef CLSVIDEOFRAMEDSOURCE_H
#define CLSVIDEOFRAMEDSOURCE_H

#include <iostream>
#include <liveMedia/FramedSource.hh>
#include <queue>
#include <x264.h>



#define SLEEP_BETWEEN_FRAMES 30

class clsDemuxer;
class clsX264Encoder;
class clsAvMediaDecoder;
class AVPacket;
namespace cv {
class VideoCapture;
}

struct stuDataFrameProvider {
    stuDataFrameProvider() {
        demuxer = NULL;
        x264Encoder = NULL;
        avMediaDecoder = NULL;
        videoCapture = NULL;
    }
    clsDemuxer          *demuxer;
    clsX264Encoder      *x264Encoder;
    clsAvMediaDecoder   *avMediaDecoder;
    cv::VideoCapture    *videoCapture;
};

class clsVideoFramedSource : public FramedSource
{
public:
    static clsVideoFramedSource* create(UsageEnvironment &_usageEnvironment);
    static EventTriggerId eventTriggerId;
    void setDataFrameProvider(clsDemuxer *_demuxer, clsX264Encoder *_x264Encoder = NULL, clsAvMediaDecoder *_avMediaDecoder = NULL);
    void setDataFrameProvider(cv::VideoCapture* _videoCapture, clsX264Encoder* _x264Encoder);
    void setDataFrameProvider(stuDataFrameProvider* _dataFrameProvider);

private:
    clsVideoFramedSource(UsageEnvironment &_usageEnvironment);
    virtual ~clsVideoFramedSource();

private:
    virtual void doGetNextFrame();
    static void deliverFrame0(void *clientData);
    void deliverFrame(const AVPacket &_avPacket);
    void deliverFrame(const x264_nal_t &_nalUnit);

private:
    stuDataFrameProvider *pPrivate;

private:
    static unsigned referenceCount;
    timeval currentTime;
    bool doEncoding;


};

#endif // CLSVIDEOFRAMEDSOURCE_H
