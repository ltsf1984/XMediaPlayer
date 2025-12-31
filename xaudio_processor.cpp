//xaudio_processor.cpp
#include "xaudio_processor.h"
#include <iostream>

extern "C" {
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}

#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swresample.lib")


bool XAudioProcessor::Init(SwrParam& input, SwrParam output)
{
	if (swr_ctx_)
	{
		swr_free(&swr_ctx_);
	}

	// 1、创建重采样上下文
	swr_ctx_ = swr_alloc();
	if (!swr_ctx_)
	{
		return false;
	}

	// 2. 设置重采样选项（特别是planar->packed转换）

	// 1、创建重采样上下文（使用 swr_alloc_set_opts 更简单）
	int ret = swr_alloc_set_opts2(
		&swr_ctx_,  // 传入nullptr创建新上下文
		&output.ch_layout,  // 输出声道布局
		output.sample_fmt,  // 输出格式
		output.sample_rate, // 输出采样率
		&input.ch_layout,	// 输入声道布局
		input.sample_fmt,   // 输入格式
		input.sample_rate,  // 输入采样率
		0,                  // 日志偏移
		nullptr             // 日志上下文
	);

	ret = swr_init(swr_ctx_);
	if (ret < 0) {
		char errbuf[256];
		av_strerror(ret, errbuf, sizeof(errbuf));
		std::cerr << "swr_init failed: " << errbuf
			<< " (in_layout=" << input.ch_layout.nb_channels
			<< ", out_layout=" << output.ch_layout.nb_channels << ")\n";
		swr_free(&swr_ctx_);
		return false;
	}
	//initialized = true;
	std::cout << "Swr init: in=" << input.ch_layout.nb_channels << "ch "
		<< av_get_sample_fmt_name(input.sample_fmt) << " " << input.sample_rate
		<< "Hz -> out=" << output.ch_layout.nb_channels << "ch "
		<< av_get_sample_fmt_name(output.sample_fmt) << " " << output.sample_rate << "Hz\n";
	return true;
}

bool XAudioProcessor::Close()
{
	if (swr_ctx_)
	{
		swr_free(&swr_ctx_);
		swr_ctx_ = nullptr;
	}
	return true;
}

bool XAudioProcessor::SetAudioStreamTimebase(AVRational audio_time_base)
{
	audio_stream_time_base_ = audio_time_base;
	return false;
}

PcmBlockPtr XAudioProcessor::FrameToPcmBlock(AVFramePtr frame)
{
	if (!swr_ctx_ || !swr_is_initialized(swr_ctx_))
	{
		return nullptr;
	}

	// 2. 计算输出样本数
	int64_t delay_samples = swr_get_delay(swr_ctx_, frame->sample_rate);
	int dst_nb_samples = av_rescale_rnd(
		delay_samples + frame->nb_samples,
		48000,
		frame->sample_rate,
		AV_ROUND_UP);

	// 3. 分配输出缓冲区
	int dst_linesize = 0;
	uint8_t* dst_data = nullptr;
	int dst_channels = 2;
	av_samples_alloc(
		&dst_data, &dst_linesize,
		dst_channels, dst_nb_samples,
		AV_SAMPLE_FMT_S16, 1);

	//3. 创建输出指针数组（关键！）
	//uint8_t* dst_data_array[2] = { dst_data, nullptr }; // S16 是 packed，只需 [0]
	uint8_t* dst_data_array[8] = { nullptr }; // 初始化全为 nullptr
	dst_data_array[0] = dst_data;             // interleaved 只用 [0]

	if (!frame || !frame->extended_data || !frame->extended_data[0]) {
		std::cout << "Invalid frame!" << std::endl;
		return nullptr;
	}
	// 4. 执行转换（FLTP → S16，8ch → 2ch，可能还有采样率转换）
	int converted_samples = swr_convert(
		swr_ctx_,
		dst_data_array, dst_nb_samples,
		frame->extended_data, frame->nb_samples);

	//std::cout << "Converted samples: " << converted_samples << std::endl;
	//if (converted_samples > 0) {
	//	short* s = (short*)dst_data;
	//	std::cout << "First samples: " << s[0] << ", " << s[1] << std::endl;
	//}

	// 5. 获取实际数据大小（字节）
	int dst_buffsize = av_samples_get_buffer_size(
		&dst_linesize, dst_channels,
		converted_samples, AV_SAMPLE_FMT_S16,
		1);

	// 6. 设置PcmBlock - 关键：需要复制数据，而不是直接引用
	PcmBlockPtr pcm_block = make_pcm_block();
	pcm_block->buff = (uint8_t*)av_mallocz(dst_buffsize);
	if (!pcm_block->buff) {
		av_freep(&dst_data);
		swr_free(&swr_ctx_);
		return nullptr;
	}

	// 复制数据到pcm_block的缓冲区
	memcpy(pcm_block->buff, dst_data, dst_buffsize);
	pcm_block->size = dst_buffsize;
	pcm_block->pts = av_rescale_q(
		frame->pts,
		audio_stream_time_base_,   // 输入时间基
		AV_TIME_BASE_Q             // 输出时间基 = {1, 1000000}
	);

	pcm_block->duration = frame->duration;

	av_freep(&dst_data);

	return pcm_block;
}

AVChannelLayout XAudioProcessor::GetChannelLayout(AVFramePtr frame)
{
	if (!frame) {
		AVChannelLayout layout;
		av_channel_layout_default(&layout, 2); // stereo
		return layout;
	}

	// 如果 frame 的 ch_layout 有效，直接返回副本
	if (frame->ch_layout.order != AV_CHANNEL_ORDER_UNSPEC) {
		AVChannelLayout layout;
		av_channel_layout_copy(&layout, &frame->ch_layout);
		return layout;
	}

	// 否则根据 nb_channels 推断
	AVChannelLayout layout;
	av_channel_layout_default(&layout, frame->ch_layout.nb_channels);
	if (layout.order == AV_CHANNEL_ORDER_UNSPEC) {
		// fallback
		av_channel_layout_default(&layout, 2); // stereo
	}
	return layout;
}
