// xdemuxer_thread.h
#pragma once
#include "xdemuxer.h"
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

class XDemuxThread
{
public:
	XDemuxThread();
	~XDemuxThread() = default;

	// 线程开始和停止
	bool Start(const std::string& filename);
	void Stop();
	// 线程暂停和恢复（继续运行）
	void Pause();
	void Resume();
	bool IsRunning();

	// 获取解封装的数据包（解码线程调用）
	bool GetPacket(AVPacket*& packet);

	// 返回空数据包（解码完成后调用）
	void ReturnPacket(AVPacket* packet);

private:
    void Run();  // 线程主函数

    XDemuxer demuxer_;
    std::unique_ptr<std::thread> thread_;
    std::atomic<bool> running_{ false };
    std::atomic<bool> should_exit_{ false };
	std::atomic<bool> paused{ false };	// 线程是否暂停
	std::condition_variable pause_cv_;
	std::mutex mtx_;
};

