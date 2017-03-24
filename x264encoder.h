#ifndef X264ENCODER_H
#define X264ENCODER_H

#ifdef __cplusplus
#define __STDINT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <iostream>
#include <queue>
#include <stdint.h>

#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

extern "C" {
    #include "x264.h"
    #include <libswscale/swscale.h>
    #include <libavcodec/avcodec.h>
}


class clsX264Encoder
{

public:
    clsX264Encoder();
    ~clsX264Encoder();
    void initialize(int _width, int _height);
    void unInitilize();
    AVPacket& encodeFrame(unsigned char *rgb_buffer);
    bool isNalsAvailableInOutputQueue();
    AVPacket& getPacket() {return this->avPacket;}

    x264_nal_t getNalUnit();
    x264_t *getx264Encoder() { return encoder; }
    int nal_size() { return output_queue.size(); }
private:
    // Use this context to convert your BGR Image to YUV image since x264 do not support RGB input
    SwsContext* convertContext;
    std::queue<x264_nal_t> output_queue;
    x264_param_t parameters;
    x264_picture_t picture_in, picture_out;
    x264_t* encoder;
    AVPacket avPacket;

};

#endif // X264ENCODER_H
