/*
 * BSP_thr.h
 *
 *  Created on: 2015-1-6
 *      Author: ltp
 */

#ifndef BSP_THR_H_
#define BSP_THR_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <pthread.h>

#define BSP_THR_PRI_MAX                 sched_get_priority_max(SCHED_FIFO)
#define BSP_THR_PRI_MIN                 sched_get_priority_min(SCHED_FIFO)

#define BSP_THR_PRI_DEFAULT             ( BSP_THR_PRI_MIN + (BSP_THR_PRI_MAX-BSP_THR_PRI_MIN)/2 )

#define BSP_THR_STACK_SIZE_DEFAULT      0

typedef void * (*BSP_ThrEntryFunc)(void *);

typedef struct {

  pthread_t      hndl;

} BSP_ThrHndl;

int BSP_thrCreate(BSP_ThrHndl *hndl, BSP_ThrEntryFunc entryFunc, int pri, unsigned int stackSize, void *prm);
int BSP_thrDelete(BSP_ThrHndl *hndl);
int BSP_thrJoin(BSP_ThrHndl *hndl);
int BSP_thrChangePri(BSP_ThrHndl *hndl, int pri);
int BSP_thrExit(void *returnVal);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BSP_THR_H_ */







