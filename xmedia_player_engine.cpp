// xmedia_player_engine.cpp
#include "xmedia_player_engine.h"

XMediaPlayerEngine::XMediaPlayerEngine()
{
}

bool XMediaPlayerEngine::Play(
	std::string& input_filename,
	int width, int height,
	PixelFormat pix_fmt,
	void* win_id)
{
	XDemuxer demuxer;
	demuxer.Open(input_filename);
	demuxer.GetVideoCodecpar();
	AVRational video_time_base = demuxer.GetVideoStreamTimebase();
	AVRational audio_time_base = demuxer.GetAudioStreamTimebase();
	AVChannelLayout ch_layout = demuxer.GetAudioCodecpar()->ch_layout;
	AVSampleFormat sample_fmt = (AVSampleFormat)demuxer.GetAudioCodecpar()->format;
	int sample_rate = demuxer.GetAudioCodecpar()->sample_rate;
	
	// 初始化音视频解码器
	decode_video_th_.Init(demuxer.GetVideoCodecpar());
	decode_audio_th_.Init(demuxer.GetAudioCodecpar());
	if (!decode_audio_th_.Init(demuxer.GetAudioCodecpar())) {
		std::cout << "Audio decoder init failed!" << std::endl;
	}
	// 初始化视频渲染器
	render_th_.Init(width, height, pix_fmt, win_id, video_time_base, &audio_output_th_);
	SwrParam input;
	input.ch_layout = ch_layout;
	input.sample_fmt = sample_fmt;
	input.sample_rate = sample_rate;

	// WASAPI 标准参数
	SwrParam output;
	AVChannelLayout output_ch_layout;
	av_channel_layout_default(&output.ch_layout, 2); // stereo
	output.sample_fmt = AV_SAMPLE_FMT_S16;
	output.sample_rate = 48000;

	audio_output_th_.Init(audio_time_base, input, output);
	demuxer.Close();

	// 开启解封装线程
	demux_th_.Start(input_filename);
	// 开启音视频解码线程
	decode_video_th_.Start();
	decode_audio_th_.Start();
	// 开启渲染线程
	render_th_.Start();
	audio_output_th_.Start();
	
	return false;
}

bool XMediaPlayerEngine::Pause()
{
	return false;
}

bool XMediaPlayerEngine::Stop()
{
	return false;
}

void XMediaPlayerEngine::Close()
{
	// 1. 先停止生产者（解封装）
	demux_th_.Stop(); // 它会自然结束（EOF）

	// 2. 再向解码队列注入毒药，让解码线程退出
	decode_video_th_.Stop(); // Push(nullptr) to VideoPacketQueue
	decode_audio_th_.Stop(); // Push(nullptr) to AudioPacketQueue

	// 3. 解码线程退出后，帧队列不再有新数据
	//    向渲染/音频输出注入毒药
	render_th_.Stop();       // Push(nullptr) to VideoFrameQueue
	audio_output_th_.Stop(); // Push(nullptr) to AudioFrameQueue
}
