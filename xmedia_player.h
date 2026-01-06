#pragma once

#include <QtWidgets/QWidget>
#include "ui_xmedia_player.h"
#include "xmedia_player_engine.h"


class XMediaPlayer : public QWidget
{
    Q_OBJECT
        
public:
    enum PlayerStatus {
        PLAYING,    //正在播放
        PAUSED,     //已暂停（可恢复）
        STOP,       //已停止（不可恢复，需要重新start）
        NONE        //无（初始状态）
    };

public:
    XMediaPlayer(QWidget *parent = nullptr);
    ~XMediaPlayer();

public slots:
    void tbPlayPause();
    void tbStop();
    void tbSeekForward();
    void tbSeekBackward();

public:
    // 开始播放和停止
    bool Play();
    void Stop();
    // 暂停和继续
    void Pause();
    void Resume();

private:
    PlayerStatus player_status_{ NONE };
    XMediaPlayerEngine player_engine_;    

private:
    Ui::XMediaPlayerClass ui;
};
