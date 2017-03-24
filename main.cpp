#include "mainwindow.h"
#include <QApplication>
#include <datafifo.h>
#include <string.h>
#include <QDebug>
#include <QIODevice>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();


//    DataFifo df;
//    int i=100;

//    while(i--)
//    {
//        char *test = "123456789";
//        char buf[1024] = "";
//        int len = 0;

//        df.pushData(test, strlen(test));
//        qDebug()<<df.getDataSize();
//    }

//    i = 10;
//    while(i--)
//    {
//        char *test = "123456789";
//        char buf[1024] = "";
//        int len = 0;

//        len = df.popData(buf, sizeof(buf));
//        qDebug()<<len<<df.getDataSize();
//    }

//    i=5;

//    while(i--)
//    {
//        char *test = "123456789";
//        char buf[1024] = "";
//        int len = 0;

//        df.pushData(test, strlen(test));
//        qDebug()<<df.getDataSize();
//    }





    return a.exec();
}
