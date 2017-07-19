#ifndef RTKPROCESS_H
#define RTKPROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "gemho_rtk.h"
#include "rtklib/rtklib.h"

void *rtkprocess_create(pairInfo *pInfo, prcopt_t *prcopt, solopt_t *solopt, RtkConfig *pRtkcfg);

int rtkprocess_destory(void *hRtk);

int rtkprocess_pushData(void *hRtk, QString id, char *data, int size);

int rtkprocess_process(void *hRtk);



#ifdef __cplusplus
}
#endif

#endif // RTKPROCESS_H
