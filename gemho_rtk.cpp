#include <QApplication>
#include <QXmlStreamReader>
#include <QFile>
#include <QList>
#include <QMultiMap>
#include <QStringList>
#include <QSet>
#include "mylog.h"
#include "gemho_rtk.h"


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

typedef struct tagGemhoRtk{
    QList<void *> listRtkProcess;


}GemhoRtk;

void *gemhoRtkStart()
{
    QList<pairInfo> list;
    QString strlog;
    QMultiMap <QStringList, QString> mapID;
    QSet<QString> set;

    QStringList strlist;

//    strlist.append("192.168.100.2");
//    strlist.append("6688");
//    mapID.insert(strlist, "1");
//    set = mapID.values(strlist).toSet();



    readxml(&list);

    mylog_console->debug(strlog.sprintf("%d", list.length()));



    return NULL;
}


int gemhoRtkStop()
{
    return 0;
}
