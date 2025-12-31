#include "xrender_thread.h"
#include "media_queues.h"
#include <iostream>

extern"C" {
#include <libavutil/time.h>
}

XRenderThread::XRenderThread()
{
}

bool XRenderThread::Init(
	int width, int height,
	PixelFormat pix_fmt,
	void* win_id, AVRational video_time_base,
	XAudioOutputThread* audio_output_thread)
{
	video_time_base_ = video_time_base;
	audio_output_thread_ = audio_output_thread;
	int ret = renderer_.Init(width, height, pix_fmt, win_id);
	return ret;
}

bool XRenderThread::Start()
{
	if (running_)
	{
		std::cout << "render thread is already running!" << std::endl;
		return false;
	}

	running_ = true;
	should_exit_ = false;
	thread_ = std::make_unique<std::thread>(&XRenderThread::Run, this);
	return true;
}

bool XRenderThread::Pause()
{
	return true;
}

bool XRenderThread::Stop()
{
	if (thread_ && thread_->joinable())
	{
		running_ = false;
		//should_exit_ = true;
		renderer_.Close();
		MediaQueues::Instance().VideoFrameQueue().Clear();
		MediaQueues::Instance().VideoFrameQueue().Push(nullptr);
		thread_->join();
	}
	return false;
}

void XRenderThread::Run()
{
	const int64_t MAX_DELAY_US = 100 * 1000; // 100ms，最大等待
	const int64_t MIN_SLEEP_US = 1000;       // 最少休眠1ms

	while (!should_exit_)
	{
		AVFramePtr frame = make_frame();
		MediaQueues::Instance().VideoFrameQueue().Pop(frame);
		if (!frame) {
			std::cout << " --------XRenderThread exit!--------" << std::endl;
			return;
		}

		if (!frame || frame->pts == AV_NOPTS_VALUE) {
			// 无有效PTS，直接渲染（fallback）
			renderer_.Draw(frame);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		// 1. 将视频 PTS 转为微秒
		int64_t video_pts_us = av_rescale_q(frame->pts, video_time_base_, AV_TIME_BASE_Q);

		// 2. 获取音频主时钟（单位：微秒）
		int64_t master_clock = 0;
		if (audio_output_thread_) {
			master_clock = audio_output_thread_->GetMasterClock();
		}
		else {
			// 无音频时，用系统时间（fallback）
			master_clock = av_gettime();
		}

		// 3. 计算差值：master_clock - video_pts
		int64_t diff = master_clock - video_pts_us;


		// 4. 丢帧：如果视频帧太旧（>100ms）
		if (diff > MAX_DELAY_US) {
			std::cout << "[Render] Drop late frame: diff=" << (diff / 1000) << "ms" << std::endl;
			continue;
		}

		// 5. 等待：如果视频帧太早（>10ms ahead）
		if (diff < -MIN_SLEEP_US) {
			int64_t sleep_us = std::min(-diff, MAX_DELAY_US);
			std::this_thread::sleep_for(std::chrono::microseconds(sleep_us));
		}
		renderer_.Draw(frame);
	}
	
	std::cout << " =========XRenderThread exit!=========" << std::endl;
}

bool XRenderThread::Render(std::shared_ptr<AVFrame> frame)
{

	return false;
}
