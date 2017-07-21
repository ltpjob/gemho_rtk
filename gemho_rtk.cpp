#include <QCoreApplication>
#include <QXmlStreamReader>
#include <QFile>
#include <QList>
#include <QMultiMap>
#include <QStringList>
#include <QSet>
#include <QDebug>
#include <QtNetwork>
#include <QTime>
#include "mylog.h"
#include "gemho_rtk.h"
#include "rtkprocess.h"
#include "bsp_thr.h"
#include <unistd.h>
typedef struct tagupComDataHead
{
    quint8  start[2];
    qint16 type;
    qint32 size;
} __attribute__((packed)) upComDataHead;

static int readDevice(QXmlStreamReader *reader, deviceInfo *device)
{
    do
    {
        reader->readNext();
    }
    while(!reader->isStartElement() && !reader->atEnd() && !reader->hasError());

    if(reader->name() != "device")
    {
        return -1;
    }

    do
    {
        reader->readNext();
    }
    while(!reader->isStartElement() && !reader->atEnd() && !reader->hasError());
    if(reader->name() != "id")
        return -1;
    device->id = reader->readElementText();

    do
    {
        reader->readNext();
    }
    while(!reader->isStartElement() && !reader->atEnd() && !reader->hasError());
    if(reader->name() != "ip")
        return -1;
    device->ip = reader->readElementText();

    do
    {
        reader->readNext();
    }
    while(!reader->isStartElement() && !reader->atEnd() && !reader->hasError());
    if(reader->name() != "port")
        return -1;
    device->port = reader->readElementText();

    do
    {
        reader->readNext();
    }
    while(!reader->isStartElement() && !reader->atEnd() && !reader->hasError());
    if(reader->name() != "type")
        return -1;
    device->type = reader->readElementText();

    do
    {
        reader->readNext();
    }
    while(!reader->isStartElement() && !reader->atEnd() && !reader->hasError());
    if(reader->name() != "lat")
        return -1;
    device->lat = reader->readElementText();

    do
    {
        reader->readNext();
    }
    while(!reader->isStartElement() && !reader->atEnd() && !reader->hasError());
    if(reader->name() != "lon")
        return -1;
    device->lon = reader->readElementText();

    do
    {
        reader->readNext();
    }
    while(!reader->isStartElement() && !reader->atEnd() && !reader->hasError());
    if(reader->name() != "height")
        return -1;
    device->height = reader->readElementText();

    return 0;
}

static int readxml(QList<pairInfo> *list, RtkConfig *rtkcfg)
{
    QString filename = QCoreApplication::applicationDirPath() + "//config.xml";
    QString strlog;

    QFile file(filename);

    pairInfo pair;

    list->clear();

    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QXmlStreamReader reader(&file);
        while(!reader.atEnd() && !reader.hasError())
        {
            QXmlStreamReader::TokenType token = reader.readNext();
            if(token == QXmlStreamReader::StartDocument)
            {
                continue;
            }

            if (reader.isStartElement() && reader.name() == "savepath")
            {
                rtkcfg->savePath  = reader.readElementText();

            }

            if (reader.isStartElement() && reader.name() == "resultpath")
            {
                rtkcfg->savePath  = reader.readElementText();

            }

            if (reader.isStartElement() && reader.name() == "pair")
            {
                int ret = -1;

                ret = readDevice(&reader, &pair.device[0]);
                if(ret < 0)
                {
                    strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "readDevice 0 failed!");
                    mylog->error(strlog);
                    continue;
                }
                ret = readDevice(&reader, &pair.device[1]);
                if(ret < 0)
                {
                    strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "readDevice 1 failed!");
                    mylog->error(strlog);
                    continue;
                }

                list->append(pair);
            }
        }

        file.close();
    }

    return 0;
}

typedef struct tagDataCollect{
    QString ip;
    QString port;
    QSet<QString> ids;
    QList<void *> listRtkProcess;
    int runFlag;
    int quitFlag;
    BSP_ThrHndl netDevThread;
}DataCollect;

typedef struct tagGemhoRtk{
    QList<void *> listRtkProcess;
    QList<DataCollect *> listDataCollect;
}GemhoRtk;

static void *process_DataCollect(void *args)
{
    DataCollect *handle = (DataCollect *)args;
    QString strlog;
    QTcpSocket client;
    int nPrintflag1 = 0;
    QTime timeRcvAlive, timeSendAlive;

    strlog.sprintf("[%s][%s][%d]process_DataCollect create:ip(%s),port(%s),ids:", __FILE__, __func__, __LINE__,
                   handle->ip.toStdString().c_str(), handle->port.toStdString().c_str());
    for(QSet<QString>::iterator iter=handle->ids.begin(); iter!=handle->ids.end(); iter++)
    {
        strlog += *iter + " ";
    }
    mylog->info(strlog);

    while(handle->runFlag == 1)
    {
        client.connectToHost(QHostAddress(handle->ip), handle->port.toUShort());

        if(client.waitForConnected(2000) == false)
        {
            if(nPrintflag1 == 0)
            {
                strlog.sprintf("[%s][%s][%d]ip(%s),port(%s):%s", __FILE__, __func__, __LINE__,
                       handle->ip.toStdString().c_str(), handle->port.toStdString().c_str(),
                       client.errorString().toStdString().c_str());
                mylog->info(strlog);
                nPrintflag1 = 1;
            }
            continue;
        }

        nPrintflag1 = 0;

        strlog.sprintf("[%s][%s][%d]connect to ip(%s),port(%s) success!", __FILE__, __func__, __LINE__,
                       handle->ip.toStdString().c_str(), handle->port.toStdString().c_str());
        mylog->info(strlog);


        timeRcvAlive.restart();
        timeSendAlive.restart();
        while(client.state() == QAbstractSocket::ConnectedState && handle->runFlag == 1)
        {
            upComDataHead head;
            qint64 sentsize = 0;
            qint64 retSize = 0;
            qint64 size = 0;

            if(timeRcvAlive.elapsed() > 10*1000)
            {
                client.disconnectFromHost();
                if (client.state() == QAbstractSocket::UnconnectedState ||
                    client.waitForDisconnected(1000))
                {
                    strlog.sprintf("[%s][%s][%d]ip(%s),port(%s):timeRcvAlive timeout,reconnect!!", __FILE__, __func__, __LINE__,
                                   handle->ip.toStdString().c_str(), handle->port.toStdString().c_str());
                    mylog->error(strlog);
                }

                break;
            }

            if(timeSendAlive.elapsed() >= 2*1000)
            {
                head.start[0] = 0x55;
                head.start[1] = 0xaa;
                head.type = 2;
                head.size = 0;

                sentsize = 0;
                size = sizeof(head);

                while(size != sentsize)
                {
                  retSize = client.write((char *)&head, size-sentsize);

                  if(retSize < 0)
                  {
                      strlog.sprintf("[%s][%s][%d]ip(%s),port(%s):send alive failed!!! error:%s", __FILE__, __func__, __LINE__,
                                     handle->ip.toStdString().c_str(), handle->port.toStdString().c_str(),
                                     client.errorString().toStdString().c_str());
                      mylog->error(strlog);
                      break;
                  }
                  sentsize += retSize;
                }
                timeSendAlive.restart();
            }

//            printf("%d\n", client.bytesAvailable());
            if(client.bytesAvailable()>0 || client.waitForReadyRead(1000))
            {
                if(client.bytesAvailable() >= sizeof(upComDataHead))
                {
                    qint64 readSize = 0;
                    char buffer[1024] = "";
                    readSize = client.read(buffer, sizeof(upComDataHead));
                    if(readSize != sizeof(upComDataHead))
                    {
                        strlog.sprintf("[%s][%s][%d]read upComDataHead fail!", __FILE__, __func__, __LINE__);
                        mylog->error(strlog);
                        continue;
                    }
                    memcpy(&head, buffer, sizeof(upComDataHead));

                    if(head.start[0]==0x55&&head.start[1]==0xaa&&head.type==1)
                    {
                        if(client.bytesAvailable()==0 && client.waitForReadyRead(300) == false)
                        {
                            strlog.sprintf("[%s][%s][%d]head.type==1 reacv data fail!", __FILE__, __func__, __LINE__);
                            mylog->error(strlog);
                            continue;
                        }

                        int timeout = 300;
                        while(timeout--)
                        {
                            if(client.waitForReadyRead(10) && client.bytesAvailable() >= head.size)
                            {
                                quint32 id[3] = {};
                                QString strId;

                                readSize = client.read(buffer, head.size);
                                if(readSize != head.size)
                                {
                                    strlog.sprintf("[%s][%s][%d]read DATA fail!", __FILE__, __func__, __LINE__);
                                    mylog->error(strlog);
                                    break;
                                }

                                memcpy(id, buffer, sizeof(id));
                                strId.sprintf("%08X%08X%08X", id[2], id[1], id[0]);

                                for(QList<void *>::iterator iter = handle->listRtkProcess.begin(); iter != handle->listRtkProcess.end(); iter++)
                                {
                                    rtkprocess_pushData(*iter, strId, buffer+sizeof(id), readSize-sizeof(id));
                                }
                                break;
                            }
                        }
                    }
                    else if(head.start[0]==0x55&&head.start[1]==0xaa&&head.type==2)
                    {
                        timeRcvAlive.restart();
//                        printf("alive\n");
                    }
                    else
                    {
                        if(client.waitForReadyRead(10) == true || client.bytesAvailable() > 0)
                        {
                            client.readAll();
                        }
                    }

                }
//                data = client.read(1024);
//                if(data.size() > 0)
//                {
//                    qDebug()<<data;


//                    for(QList<void *>::iterator iter = handle->listRtkProcess.begin(); iter != handle->listRtkProcess.end(); iter++)
//                    {
//                        rtkprocess_pushData(*iter, *(handle->ids.begin()), data.data(), data.length());
//                    }

//                    client.write(data);

//                    client.disconnectFromHost();
//                    client.waitForDisconnected();
//                }
            }
        }
    }

    strlog.sprintf("[%s][%s][%d]ip(%s),port(%s):%s", __FILE__, __func__, __LINE__,
           handle->ip.toStdString().c_str(), handle->port.toStdString().c_str(),
           "normally quit!");
    mylog->info(strlog);

    handle->quitFlag = 1;

    BSP_thrExit(NULL);
    return NULL;
}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
void *gemhoRtkStart()
{
    QList<pairInfo> list;
    QString strlog;
    QMultiMap <QList<QString>, QString> mapID;
    QSet<QString> set;
    GemhoRtk *handle = new GemhoRtk;
    RtkConfig rtkcfg;

    if(handle == NULL)
    {
        strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "gemhoRtkStart new failed!");
        mylog->error(strlog);
        return handle;
    }

    readxml(&list, &rtkcfg);

    for(QList<pairInfo>::iterator iter = list.begin(); iter != list.end(); iter++)
    {
        double p[3]={0};
        double pos[3]={0};
        pairInfo pi = *iter;
        prcopt_t prcopt = prcopt_default;
        QList<QString> strlist;
        void *prtkp = NULL;

        prcopt.mode = PMODE_KINEMA;
        p[0]=pi.device[1].lat.toDouble()*D2R;
        p[1]=pi.device[1].lon.toDouble()*D2R;
        p[2]=pi.device[1].height.toDouble();
        pos2ecef(p, pos);
        prcopt.rb[0] = pos[0];
        prcopt.rb[1] = pos[1];
        prcopt.rb[2] = pos[2];
        solopt_t solopt[2] = {solopt_default, solopt_default};

        prtkp = rtkprocess_create(&pi, &prcopt, solopt, &rtkcfg);
        if(prtkp == NULL)
        {
            strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "rtkprocess_create create failed!");
            mylog->error(strlog);
            continue;
        }
        handle->listRtkProcess.push_back(prtkp);


        strlist.clear();
        strlist.append(pi.device[0].ip);
        strlist.append(pi.device[0].port);
        mapID.insert(strlist, pi.device[0].id);
        strlist.clear();
        strlist.append(pi.device[1].ip);
        strlist.append(pi.device[1].port);
        mapID.insert(strlist, pi.device[1].id);
    }

    QList<QList<QString>> keys = mapID.uniqueKeys();
    keys > keys;
    for(QList<QList<QString>>::iterator iter=keys.begin(); iter != keys.end(); iter++)
    {
        DataCollect *dc = new DataCollect;
        int status = -1;

        if(dc == NULL)
        {
            strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "gemhoRtkStart new DataCollect failed!");
            mylog->error(strlog);
            continue;
        }

        dc->ip = (*iter)[0];
        dc->port = (*iter)[1];
        dc->ids = mapID.values(*iter).toSet();
        dc->runFlag = 1;
        dc->quitFlag = 0;
        dc->listRtkProcess = handle->listRtkProcess;

        status = BSP_thrCreate(&dc->netDevThread, process_DataCollect, BSP_THR_PRI_DEFAULT, BSP_THR_STACK_SIZE_DEFAULT, dc);
        if(status < 0)
        {
            strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "DataCollect thread create failed!");
            mylog->error(strlog);
            delete dc;
            continue;
        }

        handle->listDataCollect.push_back(dc);
    }

    return handle;
}
#else
void *gemhoRtkStart()
{
    QList<pairInfo> list;
    QString strlog;
    QMultiMap <QString, QString> mapID;
    QSet<QString> set;
    GemhoRtk *handle = new GemhoRtk;
    RtkConfig rtkcfg;

    if(handle == NULL)
    {
        strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "gemhoRtkStart new failed!");
        mylog->error(strlog);
        return handle;
    }

    readxml(&list, &rtkcfg);

    for(QList<pairInfo>::iterator iter = list.begin(); iter != list.end(); iter++)
    {
        double p[3]={0};
        double pos[3]={0};
        pairInfo pi = *iter;
        prcopt_t prcopt = prcopt_default;
        QString strlist;
        void *prtkp = NULL;

        prcopt.mode = PMODE_KINEMA;
        p[0]=pi.device[1].lat.toDouble()*D2R;
        p[1]=pi.device[1].lon.toDouble()*D2R;
        p[2]=pi.device[1].height.toDouble();
        pos2ecef(p, pos);
        prcopt.rb[0] = pos[0];
        prcopt.rb[1] = pos[1];
        prcopt.rb[2] = pos[2];
        solopt_t solopt[2] = {solopt_default, solopt_default};

        prtkp = rtkprocess_create(&pi, &prcopt, solopt, &rtkcfg);
        if(prtkp == NULL)
        {
            strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "rtkprocess_create create failed!");
            mylog->error(strlog);
            continue;
        }
        handle->listRtkProcess.push_back(prtkp);


//        strlist.clear();
//        strlist.append(pi.device[0].ip);
//        strlist.append(pi.device[0].port);
        strlist = pi.device[0].ip + ":" + pi.device[0].port;
        mapID.insert(strlist, pi.device[0].id);
//        strlist.clear();
//        strlist.append(pi.device[1].ip);
//        strlist.append(pi.device[1].port);
        strlist = pi.device[1].ip + ":" + pi.device[1].port;
        mapID.insert(strlist, pi.device[1].id);
    }

    QList<QString> keys = mapID.uniqueKeys();
    for(QList<QString>::iterator iter=keys.begin(); iter != keys.end(); iter++)
    {
        DataCollect *dc = new DataCollect;
        int status = -1;
        QStringList slist;

        if(dc == NULL)
        {
            strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "gemhoRtkStart new DataCollect failed!");
            mylog->error(strlog);
            continue;
        }

        slist = (*iter).split(":");

        dc->ip = slist[0];
        dc->port = slist[1];
        dc->ids = mapID.values(*iter).toSet();
        dc->runFlag = 1;
        dc->quitFlag = 0;
        dc->listRtkProcess = handle->listRtkProcess;

        status = BSP_thrCreate(&dc->netDevThread, process_DataCollect, BSP_THR_PRI_DEFAULT, BSP_THR_STACK_SIZE_DEFAULT, dc);
        if(status < 0)
        {
            strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "DataCollect thread create failed!");
            mylog->error(strlog);
            delete dc;
            continue;
        }

        handle->listDataCollect.push_back(dc);
    }

    return handle;
}
#endif

int gemhoRtkProcess(void *pGrtk)
{
    GemhoRtk *handle = (GemhoRtk *)pGrtk;
    QString strlog;

    if(handle == NULL)
    {
        strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "gemhoRtkStop handle NULL!");
        mylog->error(strlog);
        return -1;
    }

    QList<void *> &listrtkp = handle->listRtkProcess;
    for(QList<void *>::iterator iter=listrtkp.begin(); iter!=listrtkp.end(); iter++)
    {
        rtkprocess_process(*iter);
    }

    return 0;
}


int gemhoRtkStop(void *pGrtk)
{
    GemhoRtk *handle = (GemhoRtk *)pGrtk;
    QString strlog;

    if(handle == NULL)
    {
        strlog.sprintf("[%s][%s][%d]%s", __FILE__, __func__, __LINE__, "gemhoRtkStop handle NULL!");
        mylog->error(strlog);
        return -1;
    }

    QList<DataCollect *> &listdc = handle->listDataCollect;
    QTime time;
    time.start();

    while(time.elapsed()<=3000 && listdc.length()>0)
    {
        for(QList<DataCollect *>::iterator iter=listdc.begin(); iter!=listdc.end(); )
        {
            if((*iter)->runFlag != 0)
                (*iter)->runFlag = 0;

            if((*iter)->quitFlag == 1)
            {
                BSP_thrJoin(&(*iter)->netDevThread);
                delete (*iter);
                iter=listdc.erase(iter);
            }
            else
                iter++;
        }
    }

    if(listdc.length() > 0)
        return -2;

    QList<void *> &listrtkp = handle->listRtkProcess;
    for(QList<void *>::iterator iter=listrtkp.begin(); iter!=listrtkp.end(); iter++)
    {
        rtkprocess_destory(*iter);
    }

    delete handle;

    return 0;
}
