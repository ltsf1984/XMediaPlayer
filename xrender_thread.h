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

	// 线程开始和停止
	bool Start();
	bool Stop();
	// 线程暂停和恢复（继续运行）
	void Pause();
	void Resume();

private:
	void Run();
	bool Render(std::shared_ptr<AVFrame> frame);

private:
	XRenderer renderer_;
	std::unique_ptr<std::thread> thread_;
	std::atomic<bool> running_{ false };
	std::atomic<bool> should_exit_{ false };
	std::atomic<bool> paused{ false };	// 线程是否暂停
	std::condition_variable pause_cv_;
	std::mutex mtx_;

	// 用于计算pts（将计数pts转换成时间pts）
	AVRational video_time_base_{ 1, 1000000 };
	// 用于获取总时钟（当前的音频时钟）
	XAudioOutputThread* audio_output_thread_{ nullptr }; // 指向音频输出线程
};

