#include "xmedia_player.h"
#include <iostream>
#include "xdemux_thread.h"
#include "pix_format.h"

XMediaPlayer::XMediaPlayer(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
}

XMediaPlayer::~XMediaPlayer()
{
    player_engine_.Close();
}

void XMediaPlayer::Play()
{
    std::cout << "Start Play!" << std::endl;
    std::string input_filename = "400x300_25.h264aac.mp4";
    int width = ui.label_video->width();
    int height = ui.label_video->height();
    void* win_id = (void*)ui.label_video->winId();
    PixelFormat pix_fmt = PixelFormat::YUV420P;
    player_engine_.Play(input_filename, width, height, pix_fmt, win_id);


}

void XMediaPlayer::Stop()
{
    std::cout << "Stop" << std::endl;
    player_engine_.Close();
}

