#ifndef CLSLIVESERVERMEDIASUBSESSION_H
#define CLSLIVESERVERMEDIASUBSESSION_H

#include <liveMedia/liveMedia.hh>
#include <liveMedia/OnDemandServerMediaSubsession.hh>
#include "clsVideoFramedSource.h"

class clsLiveServerMediaSubsession : public OnDemandServerMediaSubsession
{
public:
    static clsLiveServerMediaSubsession* createNew(UsageEnvironment& _usageEnvironment, bool reuseFirstSource);
    void checkForAuxSdpLine1();
    void afterPlayingDummy1();

private:
    clsLiveServerMediaSubsession(UsageEnvironment& _usageEnvironment, bool _reuseFirstSource);
    virtual ~clsLiveServerMediaSubsession();
    void setDoneFlag() { this->fDoneFlag = ~0; }
    virtual char const* getAuxSDPLine(RTPSink* _rtpSink, FramedSource* _inputSource);
    virtual FramedSource* createNewStreamSource(unsigned _clientSessionId, unsigned& _estBitRate);
    virtual RTPSink* createNewRTPSink(Groupsock* _rtpGroupsock, unsigned char _rtpPayloadTypeIfDynamic, FramedSource* _inputSource);
private:
    char* fAuxSDPLine;
    char fDoneFlag;
    RTPSink* fDummySink;
};

#endif // CLSLIVESERVERMEDIASUBSESSION_H
