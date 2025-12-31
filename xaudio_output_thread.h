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
	bool Start();
	bool Pause();
	bool Stop();
	
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
};

