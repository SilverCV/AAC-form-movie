#include <stdio.h>
#include <stdlib.h>
#include <libswresample/swresample.h>
#include <libavcodec/avcodec.h>
#include <libavutil/log.h>
#include <libavformat/avformat.h>
int main(int argc,char* argv[])
{
    if (argc != 3)
    {
        printf("usage: mp3Read <input file> <output file>\n");
        return -1;
    }
    const char* input = argv[1];
    av_register_all();
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    int err_code = avformat_open_input(&pFormatCtx,input,NULL,NULL);
    if (err_code != 0)
    {
        av_log(NULL,AV_LOG_ERROR,"fail to open file: %s\n",av_err2str(err_code));
        printf("fail to open input\n");
        return -1;
    }
    if (avformat_find_stream_info(pFormatCtx,NULL) < 0)
    {
        printf("fail to find stream info");
        return -1;
    }
    //音频
    int audio_stream_index = -1;
    audio_stream_index = av_find_best_stream(pFormatCtx,AVMEDIA_TYPE_AUDIO,-1,-1,NULL,0);
    if (audio_stream_index == -1)
    {
        av_log(NULL,AV_LOG_ERROR,"无法找到最好的音频流\n");
        return -1;
    }
    //将音频流写入到aac,mp3文件中
    const char *output = argv[2];
    AVFormatContext *ofmt_ctx = avformat_alloc_context();
    AVOutputFormat *pout_fmt= av_guess_format(NULL,output,NULL);

    if (!pout_fmt)
    {
        av_log(NULL,AV_LOG_ERROR,"无法生成此格式的多媒体文件\n");
        return -1;
    }
    if(avformat_alloc_output_context2(ofmt_ctx,NULL,NULL,output) < 0)
    {
        printf("fail to create output format context\n");
        return -1;
    }
    ofmt_ctx->oformat = pout_fmt;

    AVStream *in_stream = pFormatCtx->streams[audio_stream_index];
    AVCodecParameters *in_codecPar = in_stream->codecpar;
     AVStream *out_stream = avformat_new_stream(ofmt_ctx,in_stream->codec->codec);
    if (!out_stream)
    {
        av_log(NULL,AV_LOG_DEBUG,"fail to create the output stream\n");
        return -1;
    }
    err_code = avcodec_parameters_copy(out_stream->codecpar,in_codecPar);
    if(err_code < 0)
    {
        av_log(NULL,AV_LOG_DEBUG,"fail to copy parameters : %d\n",err_code);
        return -1;
    }
    av_dump_format(ofmt_ctx,0,output,1);
    //创建AVIOContext
    if (avio_open(&ofmt_ctx->pb,output,AVIO_FLAG_WRITE) < 0)
    {
        printf("fail to open AVIO context\n");
        return -1;
    }
    //写入信息头
    if(avformat_write_header(ofmt_ctx,NULL) < 0)
    {
        printf("fail to write header\n");
        return -1;
    }
    AVPacket *pkt = av_packet_alloc();
    av_init_packet(pkt);
    while(av_read_frame(pFormatCtx,pkt)>=0)
    {
        if (pkt->stream_index == audio_stream_index)
        {
            pkt->pts = av_rescale_q_rnd(pkt->pts,in_stream->time_base,out_stream->time_base,(AV_ROUND_PASS_MINMAX | AV_ROUND_NEAR_INF));
            pkt->dts = pkt->pts;
            pkt->duration = av_rescale_q(pkt->duration,in_stream->time_base,out_stream->time_base);
            pkt->pos = -1;
            pkt->stream_index = 0;
            av_interleaved_write_frame(ofmt_ctx,pkt);
            av_packet_unref(pkt);
        }
    }
    av_free_packet(pkt);
    av_write_trailer(ofmt_ctx);
    avio_close(ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);
    avformat_close_input(&pFormatCtx);
    return 0;
}
