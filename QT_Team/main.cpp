#include "main_game_window.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainGameWindow w;
    w.setWindowTitle("神奇宝贝连连看");
    w.show();

    return a.exec();
}
