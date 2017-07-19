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
    QString lat;
    QString lon;
    QString height;
}deviceInfo;

typedef struct tagPairInfo{
    deviceInfo device[2];
}pairInfo;

typedef struct tagRtkConfig{
    QString savePath;
    QString resultPath;
}RtkConfig;

void *gemhoRtkStart();
int gemhoRtkProcess(void *pGrtk);
int gemhoRtkStop(void *pGrtk);


#ifdef __cplusplus
}
#endif

#endif // GEMHO_RTK_H
