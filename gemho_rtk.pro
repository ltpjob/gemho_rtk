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
    binex.c \
    cmr.c \
    convgpx.c \
    convkml.c \
    convrnx.c \
    crescent.c \
    datum.c \
    download.c \
    ephemeris.c \
    geoid.c \
    gis.c \
    gw10.c \
    ionex.c \
    javad.c \
    lambda.c \
    novatel.c \
    nvs.c \
    options.c \
    pntpos.c \
    postpos.c \
    ppp.c \
    ppp_ar.c \
    ppp_corr.c \
    preceph.c \
    qzslex.c \
    rcvlex.c \
    rcvraw.c \
    rinex.c \
    rt17.c \
    rtcm.c \
    rtcm2.c \
    rtcm3.c \
    rtcm3e.c \
    rtkcmn.c \
    rtkpos.c \
    rtksvr.c \
    sbas.c \
    septentrio.c \
    skytraq.c \
    solution.c \
    ss2.c \
    stream.c \
    streamsvr.c \
    tides.c \
    tle.c \
    ublox.c \
    userdef.c

HEADERS  += mainwindow.h \
    rtklib.h

FORMS    += mainwindow.ui

DISTFILES += \
    gemho_rtk.pro.user
