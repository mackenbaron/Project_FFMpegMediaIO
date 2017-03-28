#ifndef CLSVIDEOFRAMEDSOURCE_H
#define CLSVIDEOFRAMEDSOURCE_H

#include <iostream>
#include <liveMedia/FramedSource.hh>
#include <queue>
#include <opencv2/opencv.hpp>
#include "demuxer.h"
#include "avMediaDecoder.h"
#include "x264encoder.h"



#define SLEEP_BETWEEN_FRAMES 30

class clsVideoFramedSource : public FramedSource
{
public:
    static clsVideoFramedSource* create(UsageEnvironment &_usageEnvironment);
    static EventTriggerId eventTriggerId;

public:
    static std::string filePath;
    static bool        useOpenCV;

private:
    clsVideoFramedSource(UsageEnvironment &_usageEnvironment);
    virtual ~clsVideoFramedSource();

    virtual void doGetNextFrame();
    static void deliverFrame0(void *clientData);
    void deliverFrame();
    void deliverFrame(const x264_nal_t &_nalUnit);

private:
    static unsigned referenceCount;
    static bool firstTime;
    timeval currentTime;
    cv::VideoCapture videoCapture;
    clsDemuxer demuxer;
    clsAvMediaDecoder decoder;
    clsX264Encoder encoder;
    uint width, height;
    x264_nal_t nalUnit;





};

#endif // CLSVIDEOFRAMEDSOURCE_H
