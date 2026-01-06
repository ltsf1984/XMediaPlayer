// xmedia_player_engine.h
#pragma once
#include "xdemux_thread.h"
#include "xdecode_video_thread.h"
#include "xdecode_audio_thread.h"
#include "xrender_thread.h"
#include "xaudio_outputer.h"
#include <iostream>
#include "pix_format.h"
#include "xaudio_output_thread.h"

class XMediaPlayerEngine
{
public:
	XMediaPlayerEngine();
	~XMediaPlayerEngine() = default;

	bool Play(std::string& input_filename,
		int width, int height,
		PixelFormat pix_fmt,
		void* win_id
	);
	void Stop();
	// ÔÝÍ£ºÍ¼ÌÐø
	void Pause();
	void Resume();
	void Close();

private:
	XDemuxThread demux_th_;
	XDecodeVideoThread decode_video_th_;
	XDecodeAudioThread decode_audio_th_;
	XRenderThread render_th_;
	XAudioOutputThread audio_output_th_;
	
};

