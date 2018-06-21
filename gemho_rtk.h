#ifndef GEMHO_RTK_H
#define GEMHO_RTK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <QString>

typedef struct tagDeviceInfo{
    QString id;
    QString ip;
    QString port;
    QString type;
    QString mode;
    QString lat;
    QString lon;
    QString height;
    QString save;
}deviceInfo;

typedef struct tagPairInfo{
    deviceInfo device[2];
}pairInfo;

typedef struct tagRtkConfig{
    QString savePath;
    QString resultPath;
    QString sendInterval;
    QString isSaveDatabase;
    QString serialName;
}RtkConfig;

void *gemhoRtkStart();
int gemhoRtkSendInit(void *pGrtk);
int gemhoRtkProcess(void *pGrtk);
int gemhoRtkStop(void *pGrtk);


#ifdef __cplusplus
}
#endif

#endif // GEMHO_RTK_H
