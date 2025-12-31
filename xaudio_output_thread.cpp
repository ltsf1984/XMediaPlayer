// xaudio_output_thread.cpp
#include "xaudio_output_thread.h"
#include <iostream>
#include "media_queues.h"
#include <fstream>


bool XAudioOutputThread::Init(
	AVRational audio_time_base,
	SwrParam& input, 
	SwrParam& output)
{
	processor_.Init(input, output);
	processor_.SetAudioStreamTimebase(audio_time_base);
	audio_outputer_.Init();
	// 计算每秒音频读取的字节数
	int bytes_per_sample = av_get_bytes_per_sample(output.sample_fmt);
	int bytes_per_sec = output.sample_rate * output.ch_layout.nb_channels * bytes_per_sample;
	audio_outputer_.SetBytesPerSec(bytes_per_sec);
	return true;
}

bool XAudioOutputThread::Start()
{
	if (running_)
	{
		std::cout << "audio output thread is already running!" << std::endl;
		return false;
	}

	running_ = true;
	should_exit_ = false;
	thread_ = std::make_unique<std::thread>(&XAudioOutputThread::Run, this);
	audio_outputer_.Play();
	return true;
}

bool XAudioOutputThread::Pause()
{
	audio_outputer_.Pause();
	return true;
}

bool XAudioOutputThread::Stop()
{
	if (running_ && thread_->joinable())
	{
		running_ = false;
		//should_exit_ = true;
		processor_.Close();
		audio_outputer_.Close();
		MediaQueues::Instance().AudioFrameQueue().Clear();
		MediaQueues::Instance().AudioFrameQueue().Push(nullptr);
		thread_->join();
	}
    return false;
}

void XAudioOutputThread::Run()
{
	static int write_count = 0;
	while (!should_exit_)
	{
		AVFramePtr frame = make_frame();
		std::cout << "XAudioOutputThread write count" << write_count++<<std::endl;
		MediaQueues::Instance().AudioFrameQueue().Pop(frame);

		if (!frame) {
			std::cout << " --------XAudioOutputThread exit!--------" << std::endl;
			return;
		}

		PcmBlockPtr pcm_block = processor_.FrameToPcmBlock(frame);
		audio_outputer_.WriteAudioData(
			pcm_block->buff, 
			pcm_block->size, 
			pcm_block->pts);
	}
	std::cout << " =========XAudioOutputThread exit!=========" << std::endl;
}
