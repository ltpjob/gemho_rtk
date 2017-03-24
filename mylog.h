#ifndef MYLOG_H
#define MYLOG_H

#include <QString>
#include "log4qt/basicconfigurator.h"
#include "log4qt/propertyconfigurator.h"
#include "log4qt/logmanager.h"


Log4Qt::Logger *mylog = Log4Qt::Logger::logger("R1");

void log_init(QString path)
{
    Log4Qt::BasicConfigurator::configure();
    Log4Qt::PropertyConfigurator::configure(path);
}



#endif // MYLOG_H
