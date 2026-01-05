#pragma once

#include <QtWidgets/QWidget>
#include "ui_xmedia_player.h"
#include "xmedia_player_engine.h"

class XMediaPlayer : public QWidget
{
    Q_OBJECT

public:
    XMediaPlayer(QWidget *parent = nullptr);
    ~XMediaPlayer();

public slots:
    void tbPlayPause();
    void tbStop();
    void tbSeekForward();
    void tbSeekBackward();

public:
    void Play();
    void Pause();
    void Stop();

private:
    bool playing_{ false };
    XMediaPlayerEngine player_engine_;    

private:
    Ui::XMediaPlayerClass ui;
};
