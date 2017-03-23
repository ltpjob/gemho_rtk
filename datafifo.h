#ifndef DATAFIFO_H
#define DATAFIFO_H
#include <QMutex>
#include <QBuffer>

class DataFifo
{
public:
    DataFifo();
    virtual ~DataFifo();
    int pushData(char *data, int len);
    int popData(char *data, int len);
    int getDataSize();

protected:
    QMutex m_lock;
    QBuffer m_buffer;
};

#endif // DATAFIFO_H
