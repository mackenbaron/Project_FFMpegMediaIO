#include "clsVideoFramedSource.h"
#include "demuxer.h"
#include "x264encoder.h"
#include "avMediaDecoder.h"
#include <opencv2/opencv.hpp>

EventTriggerId clsVideoFramedSource::eventTriggerId = 0;
unsigned clsVideoFramedSource::referenceCount = 0;


clsVideoFramedSource *clsVideoFramedSource::create(UsageEnvironment &_usageEnvironment)
{
    return new clsVideoFramedSource(_usageEnvironment);
}

void clsVideoFramedSource::setDataFrameProvider(clsDemuxer *_demuxer, clsX264Encoder *_x264Encoder, clsAvMediaDecoder *_avMediaDecoder)
{
    this->pPrivate->demuxer         = _demuxer;
    this->pPrivate->x264Encoder     = _x264Encoder;
    this->pPrivate->avMediaDecoder  = _avMediaDecoder;
}

void clsVideoFramedSource::setDataFrameProvider(cv::VideoCapture *_videoCapture, clsX264Encoder *_x264Encoder)
{
    this->pPrivate->videoCapture    = _videoCapture;
    this->pPrivate->x264Encoder     = _x264Encoder;
}

void clsVideoFramedSource::setDataFrameProvider(stuDataFrameProvider *_dataFrameProvider)
{
    this->pPrivate = _dataFrameProvider;
//    decoder.initialize(this->pPrivate->avMediaDecoder->get(0), this->pPrivate->avMediaDecoder->get(1));
}

clsVideoFramedSource::clsVideoFramedSource(UsageEnvironment &_usageEnvironment) : FramedSource(_usageEnvironment)
{
    ++this->referenceCount;
    if(this->eventTriggerId == 0) {
        this->eventTriggerId = envir().taskScheduler().createEventTrigger(deliverFrame0);
    }
}

clsVideoFramedSource::~clsVideoFramedSource()
{
    --this->referenceCount;
    envir().taskScheduler().deleteEventTrigger(this->eventTriggerId);
    this->eventTriggerId = 0;
}

void clsVideoFramedSource::doGetNextFrame()
{
    AVPacket avPacket;
    uint width, height;
    if(this->pPrivate->videoCapture != NULL) {
        width = this->pPrivate->videoCapture->get(CV_CAP_PROP_FRAME_WIDTH);
        height = this->pPrivate->videoCapture->get(CV_CAP_PROP_FRAME_HEIGHT);
    } else if(this->pPrivate->avMediaDecoder != NULL) {
        width = this->pPrivate->avMediaDecoder->get(0);
        height = this->pPrivate->avMediaDecoder->get(1);
    }
    cv::Mat decodedFrame(height, width, CV_8UC3);
    x264_nal_t nalUnit;
    bool playing = false;
    if(this->pPrivate->x264Encoder->isNalsAvailableInOutputQueue()) {
        nalUnit = this->pPrivate->x264Encoder->getNalUnit();
    } else {
        while(true) {
            if(this->pPrivate->videoCapture != NULL) {
                playing = this->pPrivate->videoCapture->read(decodedFrame);
                cv::cvtColor(decodedFrame, decodedFrame, CV_BGR2RGB);
                break;
            }
            else if(this->pPrivate->demuxer != NULL) {
                this->pPrivate->demuxer->getNextAvPacketFrame(avPacket);
                if(avPacket.stream_index == this->pPrivate->demuxer->getVideoStreamNumber()) {
                    playing = this->pPrivate->avMediaDecoder->decodeFrame(avPacket, decodedFrame.data);
                     break;
                }
            }
        }
        if(playing) {
            avPacket = this->pPrivate->x264Encoder->encodeFrame(decodedFrame.data);
            gettimeofday(&currentTime,NULL);
            nalUnit = this->pPrivate->x264Encoder->getNalUnit();
#if 0
            decoder.decodeFrame((char*)avPacket.data, avPacket.size, decodedFrame.data);
            cv::imshow("dec", decodedFrame);
#endif
            cv::waitKey(1 + SLEEP_BETWEEN_FRAMES);
        }
    }
    if(nalUnit.i_payload == 0) {
        std::cerr << "Video is Done or what?" << std::endl;
        return;
    }
    deliverFrame(nalUnit);
}

void clsVideoFramedSource::deliverFrame0(void *clientData)
{
    //    ((clsVideoFramedSource*)clientData)->deliverFrame();
    std::cerr << "Why we see this??" << std ::endl;

}

void clsVideoFramedSource::deliverFrame(const AVPacket &_avPacket)
{
    if(_avPacket.size == 0) {
        std::cerr << "video is done or corrupted" << std::endl;
        return;
    }
    if(!isCurrentlyAwaitingData()) return;
    int trancate = 0;
    if (_avPacket.size >= 4 && _avPacket.data[0] == 0 && _avPacket.data[1] == 0 && _avPacket.data[2] == 0 && _avPacket.data[3] == 1 ) {
        trancate = 4;
    }
    else {
        if(_avPacket.size >= 3 && _avPacket.data[0] == 0 && _avPacket.data[1] == 0 && _avPacket.data[2] == 1 ) {
            trancate = 3;
        }
    }
    if(_avPacket.size - trancate > fMaxSize) {
        fFrameSize = fMaxSize;
        fNumTruncatedBytes = _avPacket.size - trancate - fMaxSize;
    }
    else {
        fFrameSize = _avPacket.size - trancate;
    }
    fPresentationTime = currentTime;
    memmove(fTo, _avPacket.data + trancate, fFrameSize);
    FramedSource::afterGetting(this);


}

void clsVideoFramedSource::deliverFrame(const x264_nal_t &_nalUnit)
{
    if(!isCurrentlyAwaitingData()) return;
    // You need to remove the start code which is there in front of every nal unit.
    // the start code might be 0x00000001 or 0x000001. so detect it and remove it. pass remaining data to live555
    int trancate = 0;
    if (_nalUnit.i_payload >= 4 && _nalUnit.p_payload[0] == 0 &&
            _nalUnit.p_payload[1] == 0 &&
            _nalUnit.p_payload[2] == 0 &&
            _nalUnit.p_payload[3] == 1 )
    {
        trancate = 4;
    }
    else if(_nalUnit.i_payload >= 3 &&
            _nalUnit.p_payload[0] == 0 &&
            _nalUnit.p_payload[1] == 0 &&
            _nalUnit.p_payload[2] == 1 )
    {
        trancate = 3;
    }
    if(_nalUnit.i_payload-trancate > fMaxSize) {
        fFrameSize = fMaxSize;
        fNumTruncatedBytes = _nalUnit.i_payload-trancate - fMaxSize;
    } else  {
        fFrameSize = _nalUnit.i_payload-trancate;
    }
    fPresentationTime = currentTime;
    memmove(fTo,_nalUnit.p_payload+trancate,fFrameSize);
    FramedSource::afterGetting(this);

}
