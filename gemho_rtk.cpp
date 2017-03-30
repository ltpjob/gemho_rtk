#include <QApplication>
#include <QXmlStreamReader>
#include <QFile>
#include <QList>
#include <QMultiMap>
#include <QStringList>
#include <QSet>
#include <QDebug>
#include "mylog.h"
#include "gemho_rtk.h"
#include "rtkprocess.h"
#include "bsp_thr.h"
#include <unistd.h>


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

static int readxml(QList<pairInfo> *list)
{
    QString filename = QApplication::applicationDirPath() + "\\config.xml";
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

            if (reader.isStartElement() && reader.name() == "pair")
            {
                int ret = -1;

                ret = readDevice(&reader, &pair.device[0]);
                if(ret < 0)
                {
                    strlog.sprintf("%s][%s][%d]%s", __FILE__, __func__, __LINE__, "readDevice 0 failed!");
                    mylog->error(strlog);
                    continue;
                }
                ret = readDevice(&reader, &pair.device[1]);
                if(ret < 0)
                {
                    strlog.sprintf("%s][%s][%d]%s", __FILE__, __func__, __LINE__, "readDevice 1 failed!");
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

    strlog.sprintf("%s][%s][%d]process_DataCollect create:ip(%s),port(%s),ids:", __FILE__, __func__, __LINE__,
                   handle->ip.toStdString().c_str(), handle->port.toStdString().c_str());
    for(QSet<QString>::iterator iter=handle->ids.begin(); iter!=handle->ids.end(); iter++)
    {
        strlog += *iter + " ";
    }
    mylog->info(strlog);


    while(handle->runFlag == 1)
    {
        sleep(1);
    }

    return NULL;
}

void *gemhoRtkStart()
{
    QList<pairInfo> list;
    QString strlog;
    QMultiMap <QStringList, QString> mapID;
    QSet<QString> set;
    GemhoRtk *handle = new GemhoRtk;

    if(handle == NULL)
    {
        strlog.sprintf("%s][%s][%d]%s", __FILE__, __func__, __LINE__, "gemhoRtkStart new failed!");
        mylog->error(strlog);
        return handle;
    }

//    strlist.append("192.168.100.2");
//    strlist.append("6688");
//    mapID.insert(strlist, "1");
//    set = mapID.values(strlist).toSet();

    readxml(&list);

    for(QList<pairInfo>::iterator iter = list.begin(); iter != list.end(); iter++)
    {
        double p[3]={0};
        double pos[3]={0};
        pairInfo pi = *iter;
        prcopt_t prcopt = prcopt_default;
        QStringList strlist;
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

        prtkp = rtkprocess_create(&pi, &prcopt, solopt);
        if(prtkp == NULL)
        {
            strlog.sprintf("%s][%s][%d]%s", __FILE__, __func__, __LINE__, "rtkprocess_create create failed!");
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

    QList<QStringList> keys = mapID.uniqueKeys();
    for(QList<QStringList>::iterator iter=keys.begin(); iter != keys.end(); iter++)
    {
        DataCollect *dc = new DataCollect;
        int status = -1;

        if(dc == NULL)
        {
            strlog.sprintf("%s][%s][%d]%s", __FILE__, __func__, __LINE__, "gemhoRtkStart new DataCollect failed!");
            mylog->error(strlog);
            continue;
        }

        dc->ip = (*iter)[0];
        dc->port = (*iter)[1];
        dc->ids = mapID.values(*iter).toSet();
        dc->runFlag = 1;
        dc->listRtkProcess = handle->listRtkProcess;

        status = BSP_thrCreate(&dc->netDevThread, process_DataCollect, BSP_THR_PRI_DEFAULT, BSP_THR_STACK_SIZE_DEFAULT, dc);
        if(status < 0)
        {
            strlog.sprintf("%s][%s][%d]%s", __FILE__, __func__, __LINE__, "DataCollect thread create failed!");
            mylog->error(strlog);
            delete dc;
            continue;
        }

        handle->listDataCollect.push_back(dc);
    }

    return handle;
}


int gemhoRtkStop()
{
    return 0;
}
