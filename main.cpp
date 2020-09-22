#include <iostream>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame, SwsContext *pSwsContxt, AVFrame * pOutFrame);

int main(int argc, char const *argv[])
{
    AVFormatContext * pFormatContext = nullptr;
    const char *filename = argv[1];

    avformat_open_input(&pFormatContext, filename, nullptr, nullptr);
    if(!pFormatContext)
    {
        std::cout << "ERROR: Could not open input file" << std::endl;
        return -1;
    }
    std::cout << "Format " << pFormatContext->iformat->name
              << ", duration " << pFormatContext->duration
              << "us, bit_rate " << pFormatContext->bit_rate
              << std::endl;

    if (avformat_find_stream_info(pFormatContext,  nullptr) < 0) {
        std::cout <<"ERROR: Could not get input stream info" << std::endl;
        return -1;
    }

    AVCodecParameters *pCodecParamters = nullptr;
    int videoStreamIndex = -1;
    for(int i = 0; i < pFormatContext->nb_streams; i++) {
        AVCodecParameters *codecPar = pFormatContext->streams[i]->codecpar;
        if(codecPar->codec_type != AVMEDIA_TYPE_VIDEO) {
            continue;
        }

        videoStreamIndex = i;
        pCodecParamters = codecPar;
    }

    AVCodec *pCodec = avcodec_find_decoder(pCodecParamters->codec_id);
    if(!pCodec) {
        std::cout << "ERROR: Could not find codec" << std::endl;
        return -1;
    }

    std::cout << "Codec " << pCodec->name << " ID " << pCodec->id << std::endl;

    AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
    if(!pCodecContext) {
        std::cout << "ERROR: Failed to allocate memory for codec context" << std::endl;
        return -1;
    }

    if(avcodec_parameters_to_context(pCodecContext, pCodecParamters) < 0) {
        std::cout << "ERROR: Failed to copy codec parameter" << std::endl;
        return -1;
    }

    if(avcodec_open2(pCodecContext, pCodec, nullptr) < 0) {
        std::cout << "ERROR: Failed to open codec" << std::endl;
        return -1;
    }

    AVPacket * pPacket = av_packet_alloc();
    if(!pPacket) {
        std::cout << "ERROR: Could not allocate memory for packet" << std::endl;
        return -1;
    }

    AVFrame * pFrame = av_frame_alloc();
    if(!pFrame) {
        std::cout << "ERROR: Could not allocate memory for frame" << std::endl;
        return -1;
    }

    AVFrame *pRgbFrame = av_frame_alloc();
    if(!pRgbFrame) {
        std::cout << "ERROR: Could not allocate memory for frame" << std::endl;
        return -1;
    }
    pRgbFrame->width = pCodecParamters->width;
    pRgbFrame->height = pCodecParamters->height;
    pRgbFrame->format = AV_PIX_FMT_BGR24;
    if(av_frame_get_buffer(pRgbFrame, 16) < 0) {
        std::cout << "ERROR: Could not allocate memory for frame data" << std::endl;
        return -1;
    }

    SwsContext* pSwsContext = sws_getContext(pCodecParamters->width, pCodecParamters->height, AV_PIX_FMT_YUV420P,pRgbFrame->width, pRgbFrame->height, AV_PIX_FMT_BGR24, SWS_BILINEAR, nullptr, nullptr, nullptr);

    int i = 0;
    while(av_read_frame(pFormatContext, pPacket) >= 0) {
        if(pPacket->stream_index != videoStreamIndex) {
            break;
        }
        int response = decode_packet(pPacket, pCodecContext, pFrame, pSwsContext, pRgbFrame);
        if(response < 0) {
            break;
        } else {
            // std::cout << response << ", " << pRgbFrame->width << " x " << pRgbFrame->height << std::endl;
            // std::cout << pRgbFrame->linesize << std::endl;
            cv::Mat bgrImage(pRgbFrame->height, pRgbFrame->width, CV_8UC3, pRgbFrame->data[0]);
            cv::imwrite(std::to_string(i++) + ".jpg", bgrImage);
        }
        av_packet_unref(pPacket);
    }

    sws_freeContext(pSwsContext);
    avformat_close_input(&pFormatContext);
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
    av_frame_free(&pRgbFrame);
    avcodec_free_context(&pCodecContext);
    return 0;
}

int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame, SwsContext *pSwsContext, AVFrame *pOutFrame)
{
    int response = avcodec_send_packet(pCodecContext, pPacket);
    if(response < 0) {
        std::cout << "Error while sending a packet to the decoder: " 
                  << av_err2str(response) << std::endl;
        return response;
    }

    while(response >= 0) {
        response = avcodec_receive_frame(pCodecContext, pFrame);
        if(response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if(response < 0){
            std::cout << "Error while receiving a frame from the decoder: "
                      << av_err2str(response) << std::endl;
            return response;
        }
        std::cout << "Frame " << pCodecContext->frame_number
                  << " (type=" << av_get_picture_type_char(pFrame->pict_type)
                  << ", size=" << pFrame->pkt_size
                  << "bytes) pts " << pFrame->pts
                  << " key_frame " << pFrame->key_frame
                  << " [DTS " << pFrame->pkt_dts
                  << "]" << std::endl;
        sws_scale(pSwsContext, pFrame->data, pFrame->linesize, 0, pFrame->height, pOutFrame->data, pOutFrame->linesize);
    }

    return 0;
}