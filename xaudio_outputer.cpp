//xaudio_outputer.cpp
#include "xaudio_outputer.h"
#include <iostream>
#include "media_queues.h"
#include <sdl/SDL.h>
#include <sdl/SDL_audio.h>
#include <chrono>

extern"C" {
#include"libavutil/time.h"
}

#pragma comment(lib, "avutil.lib")

XAudioOutputer::XAudioOutputer() :buffer_(BUFFER_SIZE)
{
	// 预先清零
	std::fill(buffer_.begin(), buffer_.end(), 0);
}

XAudioOutputer::~XAudioOutputer()
{
}

bool XAudioOutputer::Init()
{
	// 强制使用 WinMM 后端（与 SDL_OpenAudio 相同）
	//SDL_setenv("SDL_AUDIODRIVER", "winmm", 1);

	if (SDL_Init(SDL_INIT_AUDIO) < 0)
	{
		std::string str_err = SDL_GetError();
		std::cout << str_err << std::endl;
	}

	// 准备SDL音频参数
	SDL_AudioSpec desired, obtained;
	SDL_zero(desired);

	// 回调模式：SDL定期回调我们获取数据
	desired.freq = 48000;       // 采样率（Hz）
	desired.format = AUDIO_S16SYS;  // 采样格式
	desired.channels = 2;       // 声道数（1=单声道，2=立体声）
	//desired.samples = 4096;     // 缓冲区大小（样本数）
	desired.samples = 1024;     // 缓冲区大小（样本数）
	desired.callback = AudioCallback;  // 音频回调函数
	desired.userdata = this;       // 传递给回调的用户数据

	//SDL_AUDIO_ALLOW_ANY_CHANGE	//使用这个时音频会断断续续，抖动
	audio_device_ = SDL_OpenAudioDevice(
		nullptr, 0,
		&desired, &obtained,
		//SDL_AUDIO_ALLOW_ANY_CHANGE
		SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE
	);

	if (audio_device_ == 0)
	{
		char buff[1024]{ 0 };
		std::string str_err = SDL_GetError();
		std::cout << str_err << std::endl;
	}

	return false;
}

bool XAudioOutputer::Open()
{
	return false;
}

bool XAudioOutputer::Play()
{
	Pause(false);
	return false;
}

bool XAudioOutputer::Pause()
{
	Pause(true);
	return false;
}

bool XAudioOutputer::Pause(bool pause)
{
	if (audio_device_ != 0)
	{
		SDL_PauseAudioDevice(audio_device_, pause ? 1 : 0);
	}
	return false;
}

bool XAudioOutputer::Stop()
{
	return true;
}

bool XAudioOutputer::Close()
{
	// 1. 立即暂停音频设备（立刻静音）
	if (audio_device_ != 0) {
		SDL_PauseAudioDevice(audio_device_, 1);
	}

	// 2. 清空环形缓冲区和 PTS 映射
	{
		std::lock_guard<std::mutex> lock(buffer_mutex_);
		std::lock_guard<std::mutex> pts_lock(pts_mutex_);

		// 重置读写位置
		read_pos_.store(0, std::memory_order_relaxed);
		write_pos_.store(0, std::memory_order_relaxed);

		// 清空缓冲区内容（可选，但推荐）
		std::fill(buffer_.begin(), buffer_.end(), 0);

		// 清空 PTS 映射
		pts_map_.clear();
		last_known_pts_ = 0;
		clock_.pts_us.store(0, std::memory_order_relaxed);
		clock_.wall_us.store(0, std::memory_order_relaxed);
	}

	// 3. 唤醒可能阻塞在 WriteAudioData 的线程
	buffer_not_full_.notify_all();
	return true;
}

int64_t XAudioOutputer::GetMasterClock() const
{
	int64_t pts = clock_.pts_us.load(std::memory_order_relaxed);
	int64_t wall = clock_.wall_us.load(std::memory_order_relaxed);

	// 如果 wall 为 0，说明未初始化或已关闭，返回 0
	if (wall == 0) {
		return 0;
	}

	int64_t now = av_gettime();
	return pts + (now - wall);
}

void XAudioOutputer::SetBytesPerSec(int bytes_per_sec)
{
	bytes_per_sec_ = bytes_per_sec;
}

int XAudioOutputer::GetAvailableWriteSpace() const
{
	int write_pos = write_pos_.load(std::memory_order_acquire);
	int read_pos = read_pos_.load(std::memory_order_acquire);

	// 计算实际在环形缓冲区中的距离
	int buffered = write_pos - read_pos;	// 已写未读的字节数
	if (buffered < 0)
	{
		// 理论上不会发生（write_pos >= read_pos）
		buffered = 0;
	}
	
	// 可写空间 = 总大小 - 已用空间 - 1（保留1字节区分空/满）
	int available = BUFFER_SIZE - buffered - 1;
	if (available<0)
	{
		available = 0;
	}
	return available;
}

void XAudioOutputer::DoRingBufferWrite(const uint8_t* data, int size)
{
	int write_pos = write_pos_.load(std::memory_order_relaxed);
	int local_write_pos = write_pos % BUFFER_SIZE;

	// 检查是否需要回绕
	if (local_write_pos + size <= BUFFER_SIZE) {
		memcpy(buffer_.data() + local_write_pos, data, size);
	}
	else {
		int first_part = BUFFER_SIZE - local_write_pos;
		int second_part = size - first_part;
		memcpy(buffer_.data() + local_write_pos, data, first_part);
		memcpy(buffer_.data(), data + first_part, second_part);
	}
	write_pos_.store(write_pos + size, std::memory_order_release);	// 绝对位置 + size
}

void XAudioOutputer::WriteAudioData(const uint8_t* data, int size, int64_t pts_us)
{
	std::unique_lock<std::mutex> lock(buffer_mutex_);
	std::cout << "GetAvailableWriteSpace() = " << GetAvailableWriteSpace() << std::endl;
	std::cout << "size = " << size << std::endl;
	// 等待直到有足够空间（自动处理虚假唤醒）
	buffer_not_full_.wait(lock, [this, size]() {
		return GetAvailableWriteSpace() >= size;
		});

	// 获取绝对写入位置
	int64_t current_write_pos = write_pos_.load();

	// 执行写入
	DoRingBufferWrite(data, size);

	// 记录 PTS 映射（关键！）
	{
		std::lock_guard<std::mutex> pts_lock(pts_mutex_);
		pts_map_.push_back(PtsEntry{ pts_us, current_write_pos });
	}
}

void XAudioOutputer::UpdateClock(int64_t played_pos)
{
	std::lock_guard<std::mutex> lock(pts_mutex_);

	// 1. 查找 played_pos 对应的 PTS
	int64_t best_pts = last_known_pts_;
	for (auto it = pts_map_.rbegin(); it != pts_map_.rend(); ++it) {
		if (it->buffer_pos <= played_pos) {
			best_pts = it->pts_us;
			break;
		}
	}
	clock_.pts_us.store(best_pts, std::memory_order_relaxed);
	clock_.wall_us.store(av_gettime(), std::memory_order_relaxed);
	last_known_pts_ = best_pts;

	// 2. 【关键】只删除“已经播放过”的 PTS（安全删除）
	auto it = pts_map_.begin();
	while (it != pts_map_.end() && it->buffer_pos < played_pos) {
		++it;
	}
	if (it != pts_map_.begin()) {
		pts_map_.erase(pts_map_.begin(), it); // 只删已播放的
	}
}


void XAudioOutputer::AudioCallback(void* userdata, unsigned char* stream, int len)
{
	// 通过userdata获取对象实例
	XAudioOutputer* self = static_cast<XAudioOutputer*>(userdata);
	self->AudioCallbackImpl(stream, len);
}

void XAudioOutputer::AudioCallbackImpl(unsigned char* stream, int len)
{
	std::memset(stream, 0, len);

	int read_pos = read_pos_.load(std::memory_order_acquire);
	int write_pos = write_pos_.load(std::memory_order_acquire);

	// 计算可读数据量
	int available;
	if (write_pos >= read_pos) {
		available = write_pos - read_pos;
	}
	else {
		available = (static_cast<int64_t>(BUFFER_SIZE) - read_pos) + write_pos;
	}

	int copy = std::min(len, available);
	if (copy > 0)
	{
		int local_read_pos = static_cast<int>(read_pos % BUFFER_SIZE);
		// 检查是否需要回绕
		if (local_read_pos + copy <= BUFFER_SIZE) {
			memcpy(stream, buffer_.data() + local_read_pos, copy);
		}
		else {
			int first_part = BUFFER_SIZE - read_pos;
			int second_part = copy - first_part;

			memcpy(stream, buffer_.data() + read_pos, first_part);
			memcpy(stream + first_part, buffer_.data(), second_part);
		}
		int64_t new_read_pos = read_pos + copy;
		read_pos_.store(new_read_pos, std::memory_order_release);

		// 更新时钟 & 清理 PTS
		UpdateClock(new_read_pos);	// 传绝对位置

		// 通知写入线程有新空间
		std::lock_guard<std::mutex> lock(buffer_mutex_);
		buffer_not_full_.notify_one();
	}
}

