#include "datafifo.h"

DataFifo::DataFifo()
{
    m_buffer.clear();
}


DataFifo::~DataFifo()
{

}

qint64 DataFifo::popData(char *data, qint64 maxSize)
{
    QByteArray tmp;

    m_lock.lock();

    if(maxSize > m_buffer.length())
        maxSize = m_buffer.length();

    tmp = m_buffer.left(maxSize);
    memcpy(data, tmp.data(), maxSize);
    m_buffer.remove(0, maxSize);

    m_lock.unlock();

    return maxSize;
}

QByteArray DataFifo::popData(qint64 maxSize)
{
    QByteArray tmp;

    m_lock.lock();

    if(maxSize > m_buffer.length())
        maxSize = m_buffer.length();

    tmp = m_buffer.left(maxSize);
    m_buffer.remove(0, maxSize);

    m_lock.unlock();

    return tmp;
}

QByteArray DataFifo::popAllData()
{
    QByteArray tmp;

    m_lock.lock();

    tmp = m_buffer;
    m_buffer.clear();

    m_lock.unlock();

    return tmp;
}

qint64 DataFifo::pushData(const char *data, qint64 maxSize)
{
    qint64 len = -1;

    m_lock.lock();

    len = m_buffer.length();
    m_buffer.append(data, maxSize);
    len = m_buffer.length()-len;

    m_lock.unlock();

    return len;
}

qint64 DataFifo::pushData(const QByteArray &byteArray)
{
    qint64 len = -1;

    m_lock.lock();

    len = m_buffer.length();
    m_buffer.append(byteArray);
    len = m_buffer.length()-len;

    m_lock.unlock();

    return len;
}

qint64 DataFifo::getDataSize()
{
    qint64 len = -1;

    m_lock.lock();

    len = m_buffer.length();

    m_lock.unlock();

    return len;
}






