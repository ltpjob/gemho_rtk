#ifndef DATAFIFO_H
#define DATAFIFO_H
#include <QMutex>
#include <QByteArray>

class DataFifo
{
public:
    DataFifo();
    virtual ~DataFifo();


    qint64 popData(char *data, qint64 maxSize);
    QByteArray popData(qint64 maxSize);
    QByteArray popAllData();


    qint64 pushData(const char *data, qint64 maxSize);
    qint64 pushData(const QByteArray &byteArray);

    qint64 getDataSize();

protected:
    QMutex m_lock;
    QByteArray m_buffer;
};

#endif // DATAFIFO_H
