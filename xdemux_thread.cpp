// xdemux_thread.cpp
#include "xdemux_thread.h"
#include "media_queues.h"
#include "av_utils.h"
#include <memory>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#pragma comment(lib, "avformat.lib")   // 格式处理
#pragma comment(lib, "avcodec.lib")    // 编解码

XDemuxThread::XDemuxThread()
{
}

bool XDemuxThread::Start(const std::string& filename)
{
	if (running_)
	{
		return true;
	}

	// 设置信号量
	running_ = true;
	should_exit_ = false;
	demuxer_.Open(filename);
	// 开启线程
	thread_ = std::make_unique<std::thread>(&XDemuxThread::Run, this);
	return true;
}

void XDemuxThread::Stop()
{
	if (thread_ && thread_->joinable())
	{
		running_ = false;
		should_exit_ = true;
		thread_->join();
	}
}

bool XDemuxThread::IsRunning()
{
	return running_;
}

bool XDemuxThread::GetPacket(AVPacket*& packet)
{
	return false;
}

void XDemuxThread::ReturnPacket(AVPacket* packet)
{
}

void XDemuxThread::Run()
{
	while (!should_exit_)
	{
		std::shared_ptr<AVPacket> pkt = make_packet();
		demuxer_.Read(pkt.get());
		if (!pkt->buf)
		{
			break;
		}
		if (pkt->stream_index == demuxer_.video_index())
		{
			MediaQueues::Instance().VideoPacketQueue().Push(pkt);
		}
		else if (pkt->stream_index == demuxer_.audio_index())
		{
			MediaQueues::Instance().AudioPacketQueue().Push(pkt);
		}
	}
	std::cout << " =========XDemuxThread exit!=========" << std::endl;
}
