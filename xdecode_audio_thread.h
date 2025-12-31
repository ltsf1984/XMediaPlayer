// xdecode_audio_thread.h
#pragma once
#include "xdecoder.h"
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

class XDecodeAudioThread
{
public:
	XDecodeAudioThread();
	~XDecodeAudioThread() = default;

	bool Init(AVCodecParameters* codecpar);
	bool Start();
	void Stop();
	bool IsRunning();

	// 获取解封装的数据包（解码线程调用）
	bool GetPacket(AVPacket*& packet);

	// 返回空数据包（解码完成后调用）
	void ReturnPacket(AVPacket* packet);

private:
	void Run();  // 线程主函数

	XDecoder decoder_;
	std::unique_ptr<std::thread> thread_;
	std::atomic<bool> running_{ false };
	std::atomic<bool> should_exit_{ false };
};

