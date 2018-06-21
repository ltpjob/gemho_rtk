#include <QCoreApplication>
#include <datafifo.h>
#include <QDebug>
#include <QIODevice>
#include <QDataStream>
#include "mylog.h"
#include <QMap>
#include "rtkprocess.h"
#include <QString>
#include "pthread.h"
#include <unistd.h>
#include "gemho_rtk.h"
#include "bsp_thr.h"
#include <QThread>
#include <QSerialPort>

class processThread: public QThread
{
public:
    processThread(void *handle);
    void run(); //声明继承于QThread虚函数 run()
private:
    void *m_handle;
};

processThread::processThread(void *handle)
{
    m_handle = handle;
}
void processThread::run()
{
    gemhoRtkSendInit(m_handle);
    while(1)
    {
        gemhoRtkProcess(m_handle);
        QCoreApplication::processEvents();
        usleep(100*1000);
    }
}

void *process_main(void *args)
{
    void *handle = gemhoRtkStart();

    traceclose();
//    traceopen(".//debug.txt");

    processThread proThread(handle);
    proThread.start();

    while(1)
    {
        QString str;
        QTextStream in(stdin);
        in >> str;
        if(str == "quit")
            break;
    }

    gemhoRtkStop(handle);

    exit(0);

    return NULL;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    log_init(QCoreApplication::applicationDirPath()+ "/log4j.properties");

    BSP_ThrHndl mainThread;
    BSP_thrCreate(&mainThread, process_main, BSP_THR_PRI_DEFAULT, BSP_THR_STACK_SIZE_DEFAULT, NULL);

    return a.exec();
}
