#-------------------------------------------------
#
# Project created by QtCreator 2017-03-23T15:05:55
#
#-------------------------------------------------

QT       += core gui
LIBS += -lpthread libwsock32 libws2_32 libwinmm

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
    rtklib/userdef.c

HEADERS  += mainwindow.h \
    rtklib.h \
    datafifo.h \
    rtklib/rtklib.h

FORMS    += mainwindow.ui

DISTFILES += \
    gemho_rtk.pro.user
