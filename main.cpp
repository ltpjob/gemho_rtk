#include <QCoreApplication>
#include <datafifo.h>
#include <QDebug>
#include <QIODevice>
#include <QDataStream>
#include "mylog.h"
#include <QMap>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
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
//    pairInfo pi;
//    pi.device[0].id = "0";
//    pi.device[1].id = "1";
//    prcopt_t prcopt = prcopt_default;
//    prcopt.mode = PMODE_KINEMA;
//    prcopt.rb[0] = -2687333.72293365;
//    prcopt.rb[1] = 4293496.54669035;
//    prcopt.rb[2] = 3863214.82189491;
//    solopt_t solopt[2] = {solopt_default, solopt_default};
//    void *handle = rtkprocess_create(&pi, &prcopt, solopt);
////    rtkprocess_destory(handle);

//    traceclose();
//    traceopen("e:\\debug.txt");

//    pthread_t pid;
//    pthread_create(&pid, NULL, process, handle);

//    QSerialPort *serial_1 = new QSerialPort;
//    QSerialPort *serial_2 = new QSerialPort;

//    serial_1->setPortName("COM13");
//    serial_1->setBaudRate(QSerialPort::Baud115200);
//    serial_1->setParity(QSerialPort::NoParity);
//    serial_1->setDataBits(QSerialPort::Data8);
//    serial_1->setStopBits(QSerialPort::OneStop);
//    serial_1->setFlowControl(QSerialPort::NoFlowControl);

//    serial_2->setPortName("COM21");
//    serial_2->setBaudRate(QSerialPort::Baud115200);
//    serial_2->setParity(QSerialPort::NoParity);
//    serial_2->setDataBits(QSerialPort::Data8);
//    serial_2->setStopBits(QSerialPort::OneStop);
//    serial_2->setFlowControl(QSerialPort::NoFlowControl);

//    if (serial_1->open(QIODevice::ReadWrite) && serial_2->open(QIODevice::ReadWrite))
//    {
//        serial_1->clearError();
//        serial_1->clear();

//        serial_2->clearError();
//        serial_2->clear();

//        QDataStream *pDataStream[3] = { NULL };
//        pDataStream[0] = new QDataStream(serial_1);
//        pDataStream[1] = new QDataStream(serial_2);

//        while (1)
//        {
//            if(pDataStream[0]->device()->waitForReadyRead(3000) &&
//                    pDataStream[1]->device()->waitForReadyRead(3000))
//            {
//                int len = 0;
//                char buffer[2048] = "";


//                len = pDataStream[0]->readRawData(buffer, sizeof(buffer));
//                if(len >0)
//                    rtkprocess_pushData(handle, "0", buffer, len);

//                len = pDataStream[1]->readRawData(buffer, sizeof(buffer));
//                if(len >0)
//                    rtkprocess_pushData(handle, "1", buffer, len);
//            }
//        }
//    }

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
