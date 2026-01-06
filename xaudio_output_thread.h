#pragma once

#include "xaudio_processor.h"
#include <thread>
#include <atomic>
#include <memory>
#include "xaudio_outputer.h"

class XAudioOutputThread
{
public:
	bool Init(
		AVRational audio_time_base,
		SwrParam& input,
		SwrParam& output);

	// 线程开始和停止
	bool Start();
	void Stop();
	// 线程暂停和恢复（继续运行）
	void Pause();
	void Resume();
	
public:
	int64_t GetMasterClock() const {
		return audio_outputer_.GetMasterClock();
	}

private:
	void Run();

private:
	XAudioProcessor processor_;
	XAudioOutputer audio_outputer_;
	std::unique_ptr<std::thread> thread_;
	std::atomic<bool> running_{ false };
	std::atomic<bool> should_exit_{ false };
	std::atomic<bool> paused{ false };	// 线程是否暂停
	std::condition_variable pause_cv_;
	std::mutex mtx_;
};

