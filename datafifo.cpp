#include "datafifo.h"

DataFifo::DataFifo()
{

}


DataFifo::~DataFifo()
{

}

int DataFifo::pushData(char *data, int len)
{
    int ret = -1;

    m_lock.lock();

    m_buffer.open(QIODevice::WriteOnly|QIODevice::Append);
    ret = m_buffer.write(data, len);
    m_buffer.close();

    m_lock.unlock();

    return ret;
}

int DataFifo::popData(char *data, int len)
{
    int ret = -1;

    m_lock.lock();

    m_buffer.open(QIODevice::ReadOnly);
    ret = m_buffer.read(data, len);
    m_buffer.close();

    m_lock.unlock();

    return ret;
}

int DataFifo::getDataSize()
{
    int ret = -1;

    m_lock.lock();

    ret = m_buffer.bytesAvailable();

    m_lock.unlock();

    return ret;
}
