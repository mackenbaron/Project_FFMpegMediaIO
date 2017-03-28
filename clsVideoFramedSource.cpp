#include "clsVideoFramedSource.h"
#include "demuxer.h"
#include "x264encoder.h"
#include "avMediaDecoder.h"

EventTriggerId clsVideoFramedSource::eventTriggerId = 0;
unsigned clsVideoFramedSource::referenceCount = 0;
std::string clsVideoFramedSource::filePath = "0";
bool clsVideoFramedSource::useOpenCV = false;

clsVideoFramedSource *clsVideoFramedSource::create(UsageEnvironment &_usageEnvironment)
{
    return new clsVideoFramedSource(_usageEnvironment);
}


clsVideoFramedSource::clsVideoFramedSource(UsageEnvironment &_usageEnvironment) : FramedSource(_usageEnvironment)
{
    std::cout << "clsVideoFramedSource::referenceCount: " << clsVideoFramedSource::referenceCount << std::endl;
    bool openedVideo = false;
    if(clsVideoFramedSource::useOpenCV) {
        try {
            int deviceNumber = std::stoi(clsVideoFramedSource::filePath);
            openedVideo = this->videoCapture.open(deviceNumber);
        } catch(std::invalid_argument e) {
            openedVideo = this->videoCapture.open(clsVideoFramedSource::filePath);
        }
        this->width = this->videoCapture.get(CV_CAP_PROP_FRAME_WIDTH);
        this->height = this->videoCapture.get(CV_CAP_PROP_FRAME_HEIGHT);
    } else {
        openedVideo = this->demuxer.init(clsVideoFramedSource::filePath.c_str());
        this->width = this->demuxer.getWidth();
        this->height = this->demuxer.getHeight();
        if (openedVideo)
            this->decoder.init(this->demuxer.getAvFormatContext());
    }
    if(openedVideo) {
        encoder.init(this->width, this->height);
    } else {
        std::cerr << "Could not open " + clsVideoFramedSource::filePath << std::endl;
    }
    ++clsVideoFramedSource::referenceCount;
    if(this->eventTriggerId == 0) {
        this->eventTriggerId = envir().taskScheduler().createEventTrigger(deliverFrame0);
    }
}

clsVideoFramedSource::~clsVideoFramedSource()
{
    --clsVideoFramedSource::referenceCount;
    if(clsVideoFramedSource::useOpenCV)
        this->videoCapture.release();
    else {
        demuxer.reinit();
        this->decoder.reinit(this->demuxer.getAvFormatContext());
    }
    envir().taskScheduler().deleteEventTrigger(this->eventTriggerId);
    this->eventTriggerId = 0;
}

void clsVideoFramedSource::doGetNextFrame()
{
    cv::Mat decodedFrame(this->height, this->width, CV_8UC3);
    AVPacket avPacket;
    bool playing = false;
    if(this->encoder.isNalsAvailableInOutputQueue()) {
        this->nalUnit = this->encoder.getNalUnit();
    } else {
        if(clsVideoFramedSource::useOpenCV) {
            if((playing = this->videoCapture.read(decodedFrame)))
                cv::cvtColor(decodedFrame, decodedFrame, CV_BGR2RGB);
        } else {
            while(true) {
                playing = demuxer.getNextAvPacketFrame(avPacket);
                if(avPacket.stream_index == demuxer.getVideoStreamNumber())
                    break;
            }
            this->decoder.decodeFrame(avPacket, decodedFrame.data);
        }
    }
    if(playing) {
        this->encoder.encodeFrame(decodedFrame.data);
        gettimeofday(&currentTime,NULL);
        this->nalUnit = this->encoder.getNalUnit();
        cv::waitKey(1 + SLEEP_BETWEEN_FRAMES);
    }
    if(this->nalUnit.i_payload == 0) {
        std::cerr << "Video is Done or what?" << std::endl;
        return;
    }
    deliverFrame();
}

void clsVideoFramedSource::deliverFrame0(void *clientData)
{
    ((clsVideoFramedSource*)clientData)->deliverFrame();
    std::cerr << "Why we see this??" << std ::endl;

}

void clsVideoFramedSource::deliverFrame()
{
    if(!isCurrentlyAwaitingData()) return;
    int trancate = 0;
    if (this->nalUnit.i_payload >= 4 && this->nalUnit.p_payload[0] == 0 && this->nalUnit.p_payload[1] == 0 && this->nalUnit.p_payload[2] == 0 && this->nalUnit.p_payload[3] == 1 ) {
        trancate = 4;
    }
    else {
        if(this->nalUnit.i_payload >= 3 && this->nalUnit.p_payload[0] == 0 && this->nalUnit.p_payload[1] == 0 && this->nalUnit.p_payload[2] == 1 ) {
            trancate = 3;
        }
    }
    if(this->nalUnit.i_payload - trancate > fMaxSize) {
        fFrameSize = fMaxSize;
        fNumTruncatedBytes = this->nalUnit.i_payload - trancate - fMaxSize;
    }
    else {
        fFrameSize = this->nalUnit.i_payload - trancate;
    }
    fPresentationTime = currentTime;
    memmove(fTo, this->nalUnit.p_payload + trancate, fFrameSize);
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
