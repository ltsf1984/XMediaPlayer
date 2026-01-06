#include "xmedia_player.h"
#include <iostream>
#include "xdemux_thread.h"
#include "pix_format.h"

XMediaPlayer::XMediaPlayer(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);
}

XMediaPlayer::~XMediaPlayer()
{
	player_engine_.Close();
}

void XMediaPlayer::tbPlayPause()
{
	if (player_status_ == NONE || player_status_ == STOP)
	{
		if (Play())
		{
			// 切换到暂停图标
			ui.tb_play_pause->setIcon(
				QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackPause));
			player_status_ = PLAYING;
		}
	}
	else if(player_status_ == PLAYING)
	{
		Pause();
		// 切换到开始图标
		ui.tb_play_pause->setIcon(
			QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackStart));
		player_status_ = PAUSED;
	}
	else if (player_status_ == PAUSED)
	{
		Resume();
		player_status_ = PLAYING;
	}
}

void XMediaPlayer::tbStop()
{
	if (player_status_ == NONE || player_status_ == STOP)
	{
		return;
	}
	else if (player_status_ == PLAYING)
	{
		Stop();
		// 切换到开始图标
		ui.tb_play_pause->setIcon(
			QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackStart));
		player_status_ = STOP;
	}
	else if (player_status_ == PAUSED)
	{
		Resume();
		Stop();
		player_status_ = STOP;
	}

}

void XMediaPlayer::tbSeekForward()
{
	std::cout << "tbSeekForward" << std::endl;
}

void XMediaPlayer::tbSeekBackward()
{
	std::cout << "tbSeekBackward" << std::endl;
}


bool XMediaPlayer::Play()
{
	std::cout << "Start Play!" << std::endl;
	std::string input_filename = "400x300_25.h264aac.mp4";
	int width = ui.label_video->width();
	int height = ui.label_video->height();
	void* win_id = (void*)ui.label_video->winId();
	PixelFormat pix_fmt = PixelFormat::YUV420P;
	player_engine_.Play(input_filename, width, height, pix_fmt, win_id);

	return true;
}

void XMediaPlayer::Stop()
{
	player_engine_.Stop();
}

void XMediaPlayer::Pause()
{
	player_engine_.Pause();
}

void XMediaPlayer::Resume()
{
	player_engine_.Resume();
}




