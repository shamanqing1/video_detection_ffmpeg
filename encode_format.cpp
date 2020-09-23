#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace fs = boost::filesystem;

int encode(AVFormatContext *fmt_ctx, AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt);

int get_files(std::string dir, std::vector<std::string> &files)
{
    fs::path dirPath(dir);
    for(fs::directory_iterator itr(dirPath); itr != fs::directory_iterator(); ++itr)
    {
        if(!fs::is_directory(itr->path()))
        {
            files.push_back(itr->path().filename().string());
        }
    }
    return 0;
}


int main(int argc, char const *argv[])
{
    std::vector<std::string> files;
    std::string inputDir = argv[1];
    get_files(inputDir, files);

    std::sort(files.begin(), files.end(), [](const std::string &a, const std::string &b){
        size_t lastIndex;
        lastIndex = a.find_last_of(".");
        int na = std::stoi(a.substr(0, lastIndex));
        lastIndex = b.find_last_of(".");
        int nb = std::stoi(b.substr(0, lastIndex));
        return na < nb;
    });

    const char * outputFile = argv[2];

    AVFormatContext *pFormatContext = nullptr;
    avformat_alloc_output_context2(&pFormatContext, nullptr, nullptr, outputFile);
    if(!pFormatContext) {
        std::cout << "ERROR: Could not create output format context" << std::endl;
        return -1;
    }

    AVStream *pStream = avformat_new_stream(pFormatContext, nullptr);
    if(!pStream) {
        std::cout << "ERROR: Could not create output video stream" << std::endl;
        return -1;
    }

    const char * codecName = "libx265";
    AVCodec *pCodec = avcodec_find_encoder_by_name(codecName);
    if(!pCodec) {
        std::cout << "ERROR: Codec " << codecName << " not found" << std::endl;
        return -1;
    }
    std::cout << "Using Codec: " << pCodec->long_name << std::endl;

    AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
    if(!pCodecContext) {
        std::cout << "ERROR: Could not allocate video codec context" << std::endl;
        return -1;
    }
    pCodecContext->bit_rate = 4216000;
    pCodecContext->width = 1280;
    pCodecContext->height = 720;
    pCodecContext->time_base = (AVRational){1, 10};
    pCodecContext->framerate = (AVRational){10, 1};
    // pCodecContext->gop_size = 10;
    // pCodecContext->max_b_frames = 1;
    pCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;

    if(pFormatContext->oformat->flags & AVFMT_GLOBALHEADER) {
        pCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    int ret = avcodec_open2(pCodecContext, pCodec, nullptr);
    if(ret < 0) {
        std::cout << "ERROR: Could not open codec " << av_err2str(ret);
        return -1;
    }

    ret = avcodec_parameters_from_context(pStream->codecpar, pCodecContext);
    if(ret < 0) {
        std::cout << "ERROR: Could not copy codec parameters: " << av_err2str(ret);
        return -1;
    }

    pStream->time_base = pCodecContext->time_base;
    
    av_dump_format(pFormatContext, 0, outputFile, 1);

    if(!(pFormatContext->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&pFormatContext->pb, outputFile, AVIO_FLAG_WRITE);
        if(ret < 0) {
            std::cout << "ERROR: Could not open output file " << outputFile
                      << ": " << av_err2str(ret) << std::endl;
            return -1;
        }
    }

    AVDictionary* opts = nullptr;

    ret = avformat_write_header(pFormatContext, &opts);
    if(ret < 0) {
        std::cout << "ERROR: Failed when opening outputfile "
                  << ": " << av_err2str(ret) << std::endl;
        return -1;
    }

    AVPacket *pPacket = av_packet_alloc();
    if(!pPacket) {
        std::cout << "ERROR: Could not allocate video packet" << std::endl;
        return -1;
    }

    AVFrame *pFrame = av_frame_alloc();
    if(!pFrame){
        std::cout << "ERROR: Could not allocate video frame" << std::endl;
        return -1;
    }
    pFrame->format = pCodecContext->pix_fmt;
    pFrame->width = pCodecContext->width;
    pFrame->height = pCodecContext->height;

    if(av_frame_get_buffer(pFrame, 0)) {
        std::cout << "ERROR: Could not allocate the video frame data" << std::endl;
        return -1;
    }

    // cv::Mat pic = cv::imread(inputDir+"/"+files[0]);
    // if(pic.empty()) {
    //     std::cout << "ERROR: Could not read input images" << std::endl;
    //     return -1;
    // }
    // AVFrame *pRgbFrame = av_frame_alloc();
    // if(!pRgbFrame) {
    //     std::cout << "ERROR: Could not allocate memory for frame" << std::endl;
    //     return -1;
    // }

    // pRgbFrame->width = pic.cols;
    // pRgbFrame->height = pic.rows;
    // pRgbFrame->format = AV_PIX_FMT_BGR24;
    // if(av_frame_get_buffer(pRgbFrame, 16)) {
    //     std::cout << "ERROR: Could not allocate the video frame data" << std::endl;
    //     return -1;
    // }

    // SwsContext* pSwsContext = sws_getContext(pRgbFrame->width, pRgbFrame->height, AV_PIX_FMT_BGR24, pFrame->width, pFrame->height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr, nullptr, nullptr);

    int pixels = pFrame->width * pFrame->height;

    int pts = 0;
    for(std::string f: files) {
        cv::Mat rgbImage = cv::imread(inputDir+'/'+f);
        // memcpy(pRgbFrame->data[0], rgbImage.data, pRgbFrame->width*pRgbFrame->height*3);
        // sws_scale(pSwsContext, pRgbFrame->data, pRgbFrame->linesize, 0, pRgbFrame->height, pFrame->data, pFrame->linesize);
        cv::resize(rgbImage, rgbImage, cv::Size(pFrame->width, pFrame->height));
        cv::Mat yuvImage;
        cv::cvtColor(rgbImage, yuvImage, cv::COLOR_BGR2YUV_I420);
        memcpy(pFrame->data[0], yuvImage.data, pixels);
        memcpy(pFrame->data[1], yuvImage.data + pixels, pixels/4);
        memcpy(pFrame->data[2], yuvImage.data + pixels*5/4, pixels/4);

        pFrame->pts = pts;
        pts += av_rescale_q(1, pCodecContext->time_base, pStream->time_base);
        encode(pFormatContext, pCodecContext, pFrame, pPacket);
    }

    av_write_trailer(pFormatContext);

    av_frame_free(&pFrame);
    av_packet_free(&pPacket);
    avformat_close_input(&pFormatContext);
    avformat_free_context(pFormatContext);
    avcodec_free_context(&pCodecContext);
    return 0;
}

int encode(AVFormatContext *fmt_ctx, AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt)
{
    int ret;

    /* send the frame to the encoder */
    if (frame)
        std::cout << "Send frame " << frame->pts << std::endl;

    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        std::cout << "Error sending a frame for encoding" << std::endl;
        return -1;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            std::cout << "Error during encoding" << std::endl;
            return -1;
        }

        std::cout << "Write packet " << pkt->pts << " (size=" << pkt->size << ")" << std::endl;
        ret = av_interleaved_write_frame(fmt_ctx, pkt);
        av_packet_unref(pkt);
    }

    return 0;
}