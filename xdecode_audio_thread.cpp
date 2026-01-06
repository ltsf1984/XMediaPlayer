#include "xdecode_audio_thread.h"

//#include "av_utils.h"
#include "media_queues.h"
#include <fstream>


XDecodeAudioThread::XDecodeAudioThread()
{
}

bool XDecodeAudioThread::Init(AVCodecParameters* codecpar)
{
    if (!codecpar) return false;
    // 创建解码器
    decoder_.Create(codecpar->codec_id, false);
    // 复制参数
    if (avcodec_parameters_to_context(decoder_.GetContext(), codecpar) < 0)
    {
        codecpar->ch_layout;
        codecpar->format;
        codecpar->sample_rate;
        
        std::cerr << "Error: Failed to copy decoder parameters" << std::endl;
        return false;
    }
    // 打开解码器
    if (!decoder_.Open()) {
        std::cerr << "Failed to open decoder!" << std::endl;
        return false;
    }
    return true;
}

bool XDecodeAudioThread::Start()
{
    if (running_)
    {
        std::cout << "decode audio thread is already running!" << std::endl;
        return true;
    }

    running_ = true;
    should_exit_ = false;

    thread_ = std::make_unique<std::thread>(&XDecodeAudioThread::Run, this);
    return true;
}

void XDecodeAudioThread::Stop()
{
    if (thread_ && thread_->joinable())
    {
        running_ = false;
        //should_exit_ = true;
        // 注入毒药包
        MediaQueues::Instance().AudioPacketQueue().Clear();
        MediaQueues::Instance().AudioPacketQueue().Push(nullptr);
        thread_->join();
    }
}

void XDecodeAudioThread::Pause()
{
    paused.store(false);
}

void XDecodeAudioThread::Resume()
{
    paused.store(false);
    pause_cv_.notify_one();
}

bool XDecodeAudioThread::IsRunning()
{
    return running_;
}

bool XDecodeAudioThread::GetPacket(AVPacket*& packet)
{
    return false;
}

void XDecodeAudioThread::ReturnPacket(AVPacket* packet)
{
}

void XDecodeAudioThread::Run()
{
    while (!should_exit_)
    {
        // 暂停判断
        std::unique_lock<std::mutex> lock(mtx_);
        pause_cv_.wait(lock, [this]() {
            return !paused;
            });

        AVPacketPtr pkt = make_packet();
        MediaQueues::Instance().AudioPacketQueue().Pop(pkt);
        if (!pkt) {
            std::cout << " --------XDecodeAudioThread exit!--------" << std::endl;
            decoder_.Close();
            return;
        }
        decoder_.SendPacket(pkt.get());
        while (true)
        {
            AVFramePtr frame = make_frame();
            auto recv_ret = decoder_.ReceiveFrame(frame.get());
            if (recv_ret == XDecoder::ReceiveResult::Failed)
            {
                std::cout << "decode failed!" << std::endl;
                continue;
            }
            if (recv_ret == XDecoder::ReceiveResult::NeedFeed ||
                recv_ret == XDecoder::ReceiveResult::Ended)
            {
                break;
            }

            MediaQueues::Instance().AudioFrameQueue().Push(frame);
        }
    }

    std::cout << " =========XDecodeAudioThread exit!=========" << std::endl;
}
