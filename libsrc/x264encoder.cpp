#include "x264encoder.h"

using namespace std;

clsX264Encoder::clsX264Encoder()
{
    av_init_packet(&this->avPacket);
    this->avPacket.data = NULL;
    this->avPacket.size = 0;
    this->encoder = NULL;
    this->convertContext = NULL;
}

clsX264Encoder::~clsX264Encoder()
{
    this->unInitilize();
}

bool clsX264Encoder::init(int _width = 640, int _height = 480)
{
    x264_param_default_preset(&parameters, "veryfast", "zerolatency");
    parameters.i_log_level = X264_LOG_INFO;
    parameters.i_threads = 1;
    parameters.i_width = _width;
    parameters.i_height = _height;
    parameters.i_fps_num = 25;
    parameters.i_fps_den = 1;
    parameters.i_keyint_max = 25;
    parameters.b_intra_refresh = 1;
    parameters.rc.i_rc_method = X264_RC_CRF;
    parameters.rc.i_vbv_buffer_size = 1000000;
    parameters.rc.i_vbv_max_bitrate = 90000;
    parameters.rc.f_rf_constant = 25;
    parameters.rc.f_rf_constant_max = 35;
    parameters.i_sps_id = 7;
    // the following two value you should keep 1
    parameters.b_repeat_headers = 1;    // to get header before every I-Frame
    parameters.b_annexb = 1; // put start code in front of nal. we will remove start code later
    x264_param_apply_profile(&parameters, "high");

    encoder = x264_encoder_open(&parameters);
    x264_picture_alloc(&picture_in, X264_CSP_I420, parameters.i_width, parameters.i_height);
    picture_in.i_type = X264_TYPE_AUTO;
    picture_in.img.i_csp = X264_CSP_I420;
    // i have initilized my color space converter for BGR24 to YUV420 because my opencv video capture gives BGR24 image. You can initilize according to your input pixelFormat
    convertContext = sws_getContext(parameters.i_width,parameters.i_height, AV_PIX_FMT_RGB24, parameters.i_width,parameters.i_height,AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
    return true;
}

void clsX264Encoder::unInitilize()
{
//    x264_encoder_close(encoder);
    sws_freeContext(convertContext);
}

bool clsX264Encoder::reinit()
{
    uint width =  this->parameters.i_width;
    uint height = this->parameters.i_height;
    unInitilize();
    return init(width, height);

}

//Encode the rgb frame into a sequence of NALs unit that are stored in a std::vector
AVPacket &clsX264Encoder::encodeFrame(unsigned char *rgb_buffer)
{
    const uint8_t * rgb_buffer_slice[1] = {(const uint8_t *)rgb_buffer};
    int stride = 3 * parameters.i_width; // RGB stride
    //Convert the frame from RGB to YUV420
    int sliceHeight = sws_scale(convertContext, rgb_buffer_slice, &stride, 0, parameters.i_height, picture_in.img.plane, picture_in.img.i_stride);
    if(sliceHeight <= 0) {
        std::cerr << "Could not convert RGB to YUV" << std::endl;
        throw 1;
    }
    x264_nal_t* nals ;
    int i_nals = 0;
    int frame_size = -1;
    uint cursor = 0;
    picture_in.i_dts = this->frameNumbe;
    picture_in.i_pts = this->frameNumbe++;
    frame_size = x264_encoder_encode(encoder, &nals, &i_nals, &picture_in, &picture_out);
    if(frame_size > 0) {
        this->avPacket.size = frame_size;
        this->avPacket.data = (uint8_t*) malloc(frame_size * sizeof(uint8_t));
        for(int i = 0; i< i_nals; i++) {
            memcpy(this->avPacket.data + cursor, nals[i].p_payload, nals[i].i_payload);
            cursor += nals[i].i_payload;
            output_queue.push(nals[i]);
        }
    }
    // this is required for muxer to write currectly!! something related to time !!!
    this->avPacket.pts = picture_out.i_pts;
    this->avPacket.dts = picture_out.i_dts;
    this->avPacket.dts = 1;

    return this->avPacket;
}


bool clsX264Encoder::isNalsAvailableInOutputQueue()
{
    if(output_queue.empty() == true) {
        return false;
    } else {
        return true;
    }
}

x264_nal_t clsX264Encoder::getNalUnit()
{
    x264_nal_t nal;
    nal = output_queue.front();
    output_queue.pop();
    return nal;
}
