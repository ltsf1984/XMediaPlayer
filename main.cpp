#include "xmedia_player.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    XMediaPlayer w;
    w.show();
    return a.exec();
}
