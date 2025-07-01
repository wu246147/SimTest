#include "mainwindow.h"

#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    //log显示
    std::shared_ptr<qtStreamBuf>buffer;
    buffer = std::make_shared<qtStreamBuf>(&w);
    new(&std::cout) std::ostream(buffer.get());


    w.setWindowFlags(Qt::FramelessWindowHint);
    // w.showFullScreen();
    w.show();

    return a.exec();
}
