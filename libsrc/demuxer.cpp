#include "demuxer.h"
#include "libavcodec/avcodec.h"

clsDemuxer::clsDemuxer() :
    pAvFormatContext(NULL)
{
    avcodec_register_all();
    av_register_all();
}

clsDemuxer::~clsDemuxer()
{
    avformat_close_input(&this->pAvFormatContext);

}

bool clsDemuxer::init(const char* _fileName)
{

 /* initialize packet, set data to NULL, let the demuxer fill it */
    av_init_packet(&this->avPacket);
    this->avPacket.data = NULL;
    this->avPacket.size = 0;
    this->fileName.assign(_fileName);
    int number = -1;
    AVInputFormat *inputFormat = NULL;
    try {
        number = std::stoi(this->fileName);
        this->fileName = "/dev/video" + this->fileName;
        inputFormat = av_find_input_format("v4l2");
    } catch(std::invalid_argument e) {
    }
    AVDictionary *dict = NULL;
    if(this->fileName.find("rtsp") == 0)
        av_dict_set(&dict, "rtsp_transport", "tcp", 0);
//    this->fileName = "/home/esoroush/Videos/gladiator1.mp4";
    /* open input file, and allocate format context */

    if (avformat_open_input(&this->pAvFormatContext, this->fileName.c_str(), inputFormat, &dict) < 0) {
        std::cerr << "Could not open source file " << this->fileName.c_str() << std::endl;
        return false;
    }
//    this->pAvFormatContext->streams[0]->codec->width = 640;
//    this->pAvFormatContext->streams[0]->codec->width = 480;
    if (avformat_find_stream_info(this->pAvFormatContext, 0) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        return false;
    }
    /* dump input information to stderr */
    av_dump_format(this->pAvFormatContext, 0, _fileName, 0);
    initialized = true;
    return true;

}

bool clsDemuxer::reinit()
{
    avformat_close_input(&this->pAvFormatContext);
    this->pAvFormatContext = NULL;
    return init(this->fileName.c_str());
}

bool clsDemuxer::getNextAvPacketFrame(AVPacket &_avPacket)
{
    if(this->initialized == false)
        return false;
    int ret = av_read_frame(this->pAvFormatContext, &this->avPacket);
    _avPacket = this->avPacket;
    if(ret == 0)
        return true;
    else
        return false;
}

int clsDemuxer::getVideoStreamNumber()
{
    for (uint cnt = 0; cnt < this->pAvFormatContext->nb_streams; ++cnt) {
        if(this->pAvFormatContext->streams[cnt]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
            return int(cnt);
    }
    return -1;
}

uint clsDemuxer::getWidth()
{
    int ret = this->getVideoStreamNumber();
    if(ret < 0)
        return 0;
    else
        return this->pAvFormatContext->streams[ret]->codec->width;
}

uint clsDemuxer::getHeight()
{
    int ret = this->getVideoStreamNumber();
    if(ret < 0)
        return 0;
    else
        return this->pAvFormatContext->streams[ret]->codec->height;
}

