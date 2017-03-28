#include <unistd.h>

#include <opencv2/opencv.hpp>
#include <BasicUsageEnvironment.hh>

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>
#include<libavutil/opt.h>
#include <libavformat/avformat.h>
}

#include <x264encoder.h>
#include <muxer.h>
#include <demuxer.h>
#include <avMediaDecoder.h>
#include <clsLiveServerMediaSubsession.h>


// parsing arguments using getopt unistd function
bool doNetworkStreaming = false;
bool useOpenCV           = false;
std::string inputFileName; //"/home/esoroush/Videos/gladiator1.mp4"; //argv[1];"/home/esoroush/Videos/gladiator1.mp4";
std::string outputFileName; //"result2.mp4"; //argv[2];"result2.mp4";

int usage(char** argv) {
    std::cerr << "Usge: " << argv[0] << "-i <input file>(Required: Address of Input video or Camera Address)"
                                        " -o <output file>(Required: Address of encoded Video or Address of stream) "
                                        "-n(Optional: Stream video over network )"  << std::endl;
    return 1;
}

int inputParser (int argc, char** argv) {
    if(argc < 3)
        return usage(argv);
    opterr = 0;
    int c;
    while((c = getopt(argc, argv, "vni:o:")) != -1) {
        switch (c) {
        case 'n':
            doNetworkStreaming = true;
            break;
        case 'i':
            inputFileName.assign(optarg);
            break;
        case 'o':
            outputFileName.assign(optarg);
            break;
        case 'v':
            useOpenCV = true;
            break;
        case '?':
            if (optopt == 'i' || optopt == 'o')
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr,
                         "Unknown option character `\\x%x'.\n",
                         optopt);
            return usage(argv);
        default:
            break;
        }
    }
    if(inputFileName.empty())
        return usage(argv);
    return 0;
}

int main(int argc, char *argv[])
{
    // parse input arguments
    int returnParser = inputParser(argc, argv);
    if(returnParser != 0)
        return returnParser;
    if(outputFileName.empty())
        outputFileName = "ebi";
    av_register_all();
    avcodec_register_all();
    avformat_network_init();
    avdevice_register_all();
    if(doNetworkStreaming) {
        TaskScheduler* taskSchedular = BasicTaskScheduler::createNew();
        BasicUsageEnvironment* usageEnvironment = BasicUsageEnvironment::createNew(*taskSchedular);
        RTSPServer* rtspServer = RTSPServer::createNew(*usageEnvironment, 8554, NULL);
        if(rtspServer == NULL)
        {
            *usageEnvironment << "Failed to create rtsp server ::" << usageEnvironment->getResultMsg() <<"\n";
            exit(1);
        }
        ServerMediaSession* sms = ServerMediaSession::createNew(*usageEnvironment, outputFileName.c_str(), outputFileName.c_str(), "Live H264 Stream");
        clsLiveServerMediaSubsession *liveSubSession = clsLiveServerMediaSubsession::createNew(*usageEnvironment, true);
        clsVideoFramedSource::filePath = inputFileName;
        clsVideoFramedSource::useOpenCV = useOpenCV;
        sms->addSubsession(liveSubSession);
        rtspServer->addServerMediaSession(sms);
        char* url = rtspServer->rtspURL(sms);
        *usageEnvironment << "Play the stream using url "<< url << "\n";
        delete[] url;
        taskSchedular->doEventLoop();
    } else {
        Muxer muxer;
        clsDemuxer demuxer;
        clsAvMediaDecoder videoInputDecoder;
        clsX264Encoder encoder;
        cv::VideoCapture videoCapture;
        uint width, height;
        bool openedWithDemuxer = false;
        if(useOpenCV == false) {
            openedWithDemuxer = demuxer.init(inputFileName.c_str());
            if(openedWithDemuxer == false)
                std::cerr << "Could not initialize demuxer" << std::endl;
        }
        if(openedWithDemuxer) {
            width = demuxer.getWidth();
            height = demuxer.getHeight();
            videoInputDecoder.init(demuxer.getAvFormatContext());
        } else {
            try {
                int num = std::stoi(inputFileName);
                videoCapture.open(num);
            } catch(std::invalid_argument e) {
                videoCapture.open(inputFileName);
            }
            if(videoCapture.isOpened() == false) {
                std::cerr << "Even OpenCV couldn't open " + std::string(inputFileName) + std::string(" !!!");
                return 1;
            }
            width = videoCapture.get(CV_CAP_PROP_FRAME_WIDTH);
            height = videoCapture.get(CV_CAP_PROP_FRAME_HEIGHT);
            // \TODO muxer for video writing doesn't work with opencv videoCapture
            if(inputFileName.find("rtsp") != 0) {
                if(muxer.init(outputFileName.c_str(), width, height) == false)
                    return 1;
            }
        }
        cv::Mat decodedFrame(cv::Size(width, height), CV_8UC3);
        encoder.init(width, height);
        AVPacket avPacket;
        if(inputFileName.find("rtsp") != 0) {
            if(openedWithDemuxer) {
                if(muxer.init(outputFileName.c_str(), demuxer.getAvFormatContext(), true) == false)
                    return 1;
            } else {
                std::cerr << "Currently videoWriting with OpenCV VideoCapture is not supported";
            }
        }
        std::cout << decodedFrame.rows << std::endl;
        cv::Mat frame;
        while(videoCapture.read(frame) || demuxer.getNextAvPacketFrame(avPacket)) {
            if(openedWithDemuxer == false) { // decode frame with opencv leads to BGR frame
                if(frame.data == NULL)
                    break;
                cv::cvtColor(frame, decodedFrame, CV_BGR2RGB);
            } else {
                if(avPacket.stream_index == demuxer.getVideoStreamNumber()) { // reading streams of local video could have other than video streams eg. Audio, Subtitle ...
                    if(videoInputDecoder.decodeFrame(avPacket, decodedFrame.data) == false)
                        continue;
                } else { // write all other streams to the file
                    muxer.writeFrame(avPacket);
                    continue;
                }
            }
#ifdef DEBUG
            cv::imshow("frame", decodedFrame);
            cv::waitKey(12);
#endif
            AVPacket tmpPacket = encoder.encodeFrame(decodedFrame.data);
            if(openedWithDemuxer) {
                avPacket.data = tmpPacket.data;
                avPacket.size = tmpPacket.size;
                muxer.writeFrame(avPacket);
            } else
                muxer.writeFrame(tmpPacket);

            cv::cvtColor(decodedFrame, decodedFrame, CV_RGB2BGR);
            cv::imshow("", decodedFrame);
            cv::waitKey(13);
#if DEBUG
            x264decoder.decodeFrame((char*)avPacket.data, avPacket.size, decodedFrame.data);
            cvtColor(decodedFrame, decodedFrame, CV_BGR2RGB);
            cv::imshow("decd", decodedFrame);
            cv::waitKey(12);
#endif
        }
        muxer.close();
    }



    return 0;
}
