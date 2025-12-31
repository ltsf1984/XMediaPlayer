// xaudio_processor.h
// 驱动接口 WASAPI 标准
// 常见的WASAPI播放格式
//WAVEFORMATEX wfx;
//wfx.wFormatTag = WAVE_FORMAT_PCM;      // PCM格式
//wfx.nChannels = 2;                     // 声道数（通常立体声）
//wfx.nSamplesPerSec = 48000;            // 采样率（常见44.1k或48k）
//wfx.wBitsPerSample = 16;               // 采样位数
//wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
//wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

#pragma once
#include "av_utils.h"
#include <atomic>

struct SwrContext;
struct AVChannelLayout;

struct SwrParam {
	AVChannelLayout ch_layout;
	AVSampleFormat sample_fmt;
	int sample_rate;
};

class XAudioProcessor
{
public:
	bool Init(SwrParam& input, SwrParam output);
	bool Close();
	bool SetAudioStreamTimebase(AVRational audio_time_base);

	PcmBlockPtr FrameToPcmBlock(AVFramePtr frame);
	AVChannelLayout GetChannelLayout(AVFramePtr frame);

private:
	SwrContext* swr_ctx_{ nullptr };
	//std::atomic<bool> initialized{ false };
	SwrParam output_;
	AVRational audio_stream_time_base_;
};

