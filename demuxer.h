#ifndef DEMUXER_H
#define DEMUXER_H
#include <string>
#include <iostream>
extern "C" {
#include <libavformat/avformat.h>
}
class clsDemuxer {

public:
    clsDemuxer();
    ~clsDemuxer();
    bool init(const char* _fileName);
    bool reinit();
    bool getNextAvPacketFrame(AVPacket &_avPacket);
    std::string getFileName() const {return this->fileName;}
    AVFormatContext *getAvFormatContext() const {return this->pAvFormatContext;}
    void dumpFormatContext() {av_dump_format(this->pAvFormatContext, 0, this->fileName.c_str(), 0);}
    int getVideoStreamNumber();
    uint getWidth();
    uint getHeight();

private:
    AVFormatContext *pAvFormatContext = NULL;
    AVPacket avPacket;
    std::string fileName;
    bool initialized = false;


};



#endif
