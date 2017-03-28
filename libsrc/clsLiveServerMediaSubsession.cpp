#include "clsLiveServerMediaSubsession.h"

static void afterPlayingDummy(void* _clientData)
{
    clsLiveServerMediaSubsession *session = (clsLiveServerMediaSubsession*)_clientData;
    session->afterPlayingDummy1();
}

static void checkForAuxSDPLine(void* _clientData)
{
    clsLiveServerMediaSubsession* session = (clsLiveServerMediaSubsession*)_clientData;
    session->checkForAuxSdpLine1();
}


clsLiveServerMediaSubsession *clsLiveServerMediaSubsession::createNew(UsageEnvironment &_usageEnvironment, bool reuseFirstSource)
{
    return new clsLiveServerMediaSubsession(_usageEnvironment, reuseFirstSource);
}

void clsLiveServerMediaSubsession::checkForAuxSdpLine1()
{
    char const* dasl;
    if(this->fAuxSDPLine != NULL) {
        setDoneFlag();
    }
    else if(this->fDummySink != NULL && (dasl = this->fDummySink->auxSDPLine()) != NULL) {
        this->fAuxSDPLine = strDup(dasl);
        this->fDummySink = NULL;
        setDoneFlag();
    }
    else {
        int uSecsDelay = 100000;
        nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsDelay, (TaskFunc*)checkForAuxSDPLine, this);
    }
}

void clsLiveServerMediaSubsession::afterPlayingDummy1()
{
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
    this->setDoneFlag();
}


clsLiveServerMediaSubsession::clsLiveServerMediaSubsession(UsageEnvironment &_usageEnvironment, bool _reuseFirstSource) :
    OnDemandServerMediaSubsession(_usageEnvironment, _reuseFirstSource),
    fAuxSDPLine(NULL),
    fDoneFlag(0),
    fDummySink(NULL)
{

}

clsLiveServerMediaSubsession::~clsLiveServerMediaSubsession()
{
    delete[] fAuxSDPLine;
}

const char *clsLiveServerMediaSubsession::getAuxSDPLine(RTPSink *_rtpSink, FramedSource *_inputSource)
{
    if(this->fAuxSDPLine != NULL) return this->fAuxSDPLine;
    if(this->fDummySink == NULL) {
        this->fDummySink = _rtpSink;
        this->fDummySink->startPlaying(*_inputSource, afterPlayingDummy, this);
        checkForAuxSDPLine(this);
    }

    envir().taskScheduler().doEventLoop(&fDoneFlag);
    return fAuxSDPLine;
}

// \TODO right now we just send video stream over network
FramedSource *clsLiveServerMediaSubsession::createNewStreamSource(unsigned _clientSessionId, unsigned &_estBitRate)
{
    // Based on encoder configuration i kept it 90000
    _estBitRate = 90000;
    clsVideoFramedSource *source = clsVideoFramedSource::create(envir());
    // are you trying to keep the reference of the source somewhere? you shouldn't.
    // Live555 will create and delete this class object many times. if you store it somewhere
    // you will get memory access violation. instead you should configure you source to always read from your data source
    std::cout << "clsLiveServerMediaSubsession::create" << std::endl;
    return H264VideoStreamDiscreteFramer::createNew(envir(), source);
}

RTPSink *clsLiveServerMediaSubsession::createNewRTPSink(Groupsock *_rtpGroupsock, unsigned char _rtpPayloadTypeIfDynamic, FramedSource *_inputSource)
{
    return H264VideoRTPSink::createNew(envir(), _rtpGroupsock, _rtpPayloadTypeIfDynamic);

}
