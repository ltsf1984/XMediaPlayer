#pragma once
#include <sdl/SDL_audio.h>
#include <memory>
#include "media_queues.h"

struct AVFrame;

struct XAudioClock {
	std::atomic<int64_t> pts_us{ 0 };
	std::atomic<int64_t> wall_us{ 0 };
};

struct PtsEntry {
	int64_t pts_us;      // 音频样本的微秒 PTS
	int64_t buffer_pos;  // 对应的缓冲区写入位置
};

class XAudioOutputer
{
public:
	XAudioOutputer();
	~XAudioOutputer();

	bool Init();
	bool Open();
	bool Play();
	bool Pause();
	bool Stop();
	bool Close();

public:
	int64_t GetMasterClock() const;
	void SetBytesPerSec(int bytes_per_sec_);

	// 环形内存
	int GetAvailableWriteSpace() const;
	void DoRingBufferWrite(const uint8_t* data, int size);
	void WriteAudioData(const uint8_t* data, int size, int64_t pts_us); // 线程安全写入

	// pts记录管理
	void UpdateClock(int64_t played_pos);

private:
	bool Pause(bool pause);
	static void AudioCallback(void* userdata, unsigned char* stream, int len);
	void AudioCallbackImpl(unsigned char* stream, int len);

private:
	SDL_AudioDeviceID audio_device_{ 0 };

private:
	int bytes_per_sec_{ 0 };
	static constexpr int BUFFER_SIZE = 2 * 1024 * 1024; // 2MB
	std::vector<uint8_t> buffer_;
	std::vector<PtsEntry> pts_map_; // PTS 映射表
	// 读写位置，绝对位置，用取模计算得到在环形内存中的位置
	std::atomic<int> read_pos_{ 0 };	
	std::atomic<int> write_pos_{ 0 };
	mutable std::mutex buffer_mutex_;
	std::mutex pts_mutex_;
	std::condition_variable buffer_not_full_; // 写入等待条件

	XAudioClock clock_;
	int64_t last_known_pts_;
};


