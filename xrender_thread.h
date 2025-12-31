#pragma once
#include <memory>
#include <atomic>
#include <thread>
#include "XRenderer.h"
#include "xaudio_output_thread.h"
//#include "av_utils.h"

struct AVFrame;

class XRenderThread
{
public:
	XRenderThread();
	
	bool Init(
		int width, int height, 
		PixelFormat pix_fmt, 
		void* win_id, 
		AVRational video_time_base,
		XAudioOutputThread* audio_output_thread);

	bool Start();
	bool Pause();
	bool Stop();

private:
	void Run();
	bool Render(std::shared_ptr<AVFrame> frame);

private:
	XRenderer renderer_;
	std::unique_ptr<std::thread> thread_;
	std::atomic<bool> running_{ false };
	std::atomic<bool> should_exit_{ false };

	AVRational video_time_base_{ 1, 1000000 };
	XAudioOutputThread* audio_output_thread_{ nullptr }; // 指向音频输出线程
};

