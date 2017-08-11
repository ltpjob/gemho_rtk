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

void *process(void *args)
{
    void *handle = args;

    while(1)
    {
        gemhoRtkProcess(handle);
        usleep(100*1000);
    }

    return NULL;
}

void *process_main(void *args)
{
    void *handle = gemhoRtkStart();

    traceclose();
//    traceopen(".//debug.txt");

    pthread_t pid;
    pthread_create(&pid, NULL, process, handle);

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
