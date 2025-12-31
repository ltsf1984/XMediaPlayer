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
    void Play();
    void Stop();

private:
    XMediaPlayerEngine player_engine_;    

private:
    Ui::XMediaPlayerClass ui;
};
