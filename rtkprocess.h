#ifndef RTKPROCESS_H
#define RTKPROCESS_H

#include "gemho_rtk.h"
#include "rtklib/rtklib.h"
#include <QDateTime>
#include <QString>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagRtkOutSolution{
    int status;
    QString id;
    QDateTime GPST;
    QString latitude;
    QString longitude;
    QString height;
    QString Q;
    QString ns;
    QString sdn;
    QString sde;
    QString sdu;
    QString sdne;
    QString sdeu;
    QString sdun;
    QString age;
    QString ratio;
    QString baseline;
}RtkOutSolution;

void *rtkprocess_create(pairInfo *pInfo, prcopt_t *prcopt, solopt_t *solopt, RtkConfig *pRtkcfg);

int rtkprocess_destory(void *hRtk);

int rtkprocess_saveData(void *hRtk, QString id);

int rtkprocess_pushData(void *hRtk, QString id, char *data, int size, int isSave);

int rtkprocess_process(void *hRtk);

int rtkprocess_getSolAll(void *hRtk, RtkOutSolution *now, RtkOutSolution *best);

int rtkprocess_getSolBest(void *hRtk, RtkOutSolution *best);

int rtkprocess_resetBestSol(void *hRtk);

int rtkprocess_getLastProcTime(void *hRtk, QDateTime *lastPtime);

int rtkprocess_setLastProcTime(void *hRtk, QDateTime Ptime);

#ifdef __cplusplus
}
#endif

#endif // RTKPROCESS_H
