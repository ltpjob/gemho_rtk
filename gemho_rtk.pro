#-------------------------------------------------
#
# Project created by QtCreator 2017-03-23T15:05:55
#
#-------------------------------------------------

QT       += core gui serialport
LIBS += -lpthread libwsock32 libws2_32 libwinmm

DEFINES += TRACE

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = gemho_rtk
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp\
        mainwindow.cpp \
    datafifo.cpp \
    rtklib/binex.c \
    rtklib/cmr.c \
    rtklib/convgpx.c \
    rtklib/convkml.c \
    rtklib/convrnx.c \
    rtklib/crescent.c \
    rtklib/datum.c \
    rtklib/download.c \
    rtklib/ephemeris.c \
    rtklib/geoid.c \
    rtklib/gis.c \
    rtklib/gw10.c \
    rtklib/ionex.c \
    rtklib/javad.c \
    rtklib/lambda.c \
    rtklib/novatel.c \
    rtklib/nvs.c \
    rtklib/options.c \
    rtklib/pntpos.c \
    rtklib/postpos.c \
    rtklib/ppp.c \
    rtklib/ppp_ar.c \
    rtklib/ppp_corr.c \
    rtklib/preceph.c \
    rtklib/qzslex.c \
    rtklib/rcvlex.c \
    rtklib/rcvraw.c \
    rtklib/rinex.c \
    rtklib/rt17.c \
    rtklib/rtcm.c \
    rtklib/rtcm2.c \
    rtklib/rtcm3.c \
    rtklib/rtcm3e.c \
    rtklib/rtkcmn.c \
    rtklib/rtkpos.c \
    rtklib/rtksvr.c \
    rtklib/sbas.c \
    rtklib/septentrio.c \
    rtklib/skytraq.c \
    rtklib/solution.c \
    rtklib/ss2.c \
    rtklib/stream.c \
    rtklib/streamsvr.c \
    rtklib/tides.c \
    rtklib/tle.c \
    rtklib/ublox.c \
    rtklib/userdef.c \
    rtkprocess.cpp \
    log4qt/helpers/classlogger.cpp \
    log4qt/helpers/configuratorhelper.cpp \
    log4qt/helpers/datetime.cpp \
    log4qt/helpers/factory.cpp \
    log4qt/helpers/initialisationhelper.cpp \
    log4qt/helpers/logerror.cpp \
    log4qt/helpers/logobject.cpp \
    log4qt/helpers/logobjectptr.cpp \
    log4qt/helpers/optionconverter.cpp \
    log4qt/helpers/patternformatter.cpp \
    log4qt/helpers/properties.cpp \
    log4qt/spi/filter.cpp \
    log4qt/varia/debugappender.cpp \
    log4qt/varia/denyallfilter.cpp \
    log4qt/varia/levelmatchfilter.cpp \
    log4qt/varia/levelrangefilter.cpp \
    log4qt/varia/listappender.cpp \
    log4qt/varia/nullappender.cpp \
    log4qt/varia/stringmatchfilter.cpp \
    log4qt/appenderskeleton.cpp \
    log4qt/basicconfigurator.cpp \
    log4qt/consoleappender.cpp \
    log4qt/dailyrollingfileappender.cpp \
    log4qt/fileappender.cpp \
    log4qt/hierarchy.cpp \
    log4qt/layout.cpp \
    log4qt/level.cpp \
    log4qt/log4qt.cpp \
    log4qt/logger.cpp \
    log4qt/loggerrepository.cpp \
    log4qt/loggingevent.cpp \
    log4qt/logmanager.cpp \
    log4qt/mdc.cpp \
    log4qt/ndc.cpp \
    log4qt/patternlayout.cpp \
    log4qt/propertyconfigurator.cpp \
    log4qt/rollingfileappender.cpp \
    log4qt/simplelayout.cpp \
    log4qt/ttcclayout.cpp \
    log4qt/writerappender.cpp \
    gemho_rtk.cpp \
    bsp_thr.c

HEADERS  += mainwindow.h \
    rtklib.h \
    datafifo.h \
    rtklib/rtklib.h \
    log4qt/helpers/classlogger.h \
    log4qt/helpers/configuratorhelper.h \
    log4qt/helpers/datetime.h \
    log4qt/helpers/factory.h \
    log4qt/helpers/initialisationhelper.h \
    log4qt/helpers/logerror.h \
    log4qt/helpers/logobject.h \
    log4qt/helpers/logobjectptr.h \
    log4qt/helpers/optionconverter.h \
    log4qt/helpers/patternformatter.h \
    log4qt/helpers/properties.h \
    log4qt/spi/filter.h \
    log4qt/varia/debugappender.h \
    log4qt/varia/denyallfilter.h \
    log4qt/varia/levelmatchfilter.h \
    log4qt/varia/levelrangefilter.h \
    log4qt/varia/listappender.h \
    log4qt/varia/nullappender.h \
    log4qt/varia/stringmatchfilter.h \
    log4qt/appender.h \
    log4qt/appenderskeleton.h \
    log4qt/basicconfigurator.h \
    log4qt/consoleappender.h \
    log4qt/dailyrollingfileappender.h \
    log4qt/fileappender.h \
    log4qt/hierarchy.h \
    log4qt/layout.h \
    log4qt/level.h \
    log4qt/log4qt.h \
    log4qt/logger.h \
    log4qt/loggerrepository.h \
    log4qt/loggingevent.h \
    log4qt/logmanager.h \
    log4qt/mdc.h \
    log4qt/ndc.h \
    log4qt/patternlayout.h \
    log4qt/propertyconfigurator.h \
    log4qt/rollingfileappender.h \
    log4qt/simplelayout.h \
    log4qt/ttcclayout.h \
    log4qt/writerappender.h \
    mylog.h \
    gemho_rtk.h \
    rtkprocess.h \
    bsp_debug.h \
    bsp_thr.h

FORMS    += mainwindow.ui

DISTFILES += \
    gemho_rtk.pro.user \
    log4j.properties
