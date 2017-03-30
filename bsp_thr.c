/*
 * BSP_thr.c
 *
 *  Created on: 2015-1-6
 *      Author: ltp
 */


#include "bsp_thr.h"
#include "bsp_debug.h"


int BSP_thrCreate(BSP_ThrHndl *hndl, BSP_ThrEntryFunc entryFunc, int pri, unsigned int stackSize, void *prm)
{
  int status=0;
  pthread_attr_t thread_attr;
  struct sched_param schedprm;

  // initialize thread attributes structure
  status = pthread_attr_init(&thread_attr);

  if(status!=0) {
    BSP_ERROR("BSP_thrCreate() - Could not initialize thread attributes\n");
    return status;
  }

  if(stackSize!=BSP_THR_STACK_SIZE_DEFAULT)
    pthread_attr_setstacksize(&thread_attr, stackSize);

  if(pri != BSP_THR_PRI_DEFAULT)
  {
	  status |= pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED);
	  status |= pthread_attr_setschedpolicy(&thread_attr, SCHED_FIFO);

	  if(pri>BSP_THR_PRI_MAX)
	    pri=BSP_THR_PRI_MAX;
	  else
	  if(pri<BSP_THR_PRI_MIN)
	    pri=BSP_THR_PRI_MIN;

	  schedprm.sched_priority = pri;
	  status |= pthread_attr_setschedparam(&thread_attr, &schedprm);

	  if(status!=0) {
	    BSP_ERROR("BSP_thrCreate() - Could not initialize thread attributes\n");
	    goto error_exit;
	  }
  }

  status = pthread_create(&hndl->hndl, &thread_attr, entryFunc, prm);

  if(status!=0) {
    BSP_ERROR("BSP_thrCreate() - Could not create thread [%d]\n", status);
    perror("pthread_create");
  }

error_exit:
  pthread_attr_destroy(&thread_attr);

  return status;
}

int BSP_thrJoin(BSP_ThrHndl *hndl)
{
  int status=0;
  void *returnVal;

  status = pthread_join(hndl->hndl, &returnVal);

  return status;
}

int BSP_thrDelete(BSP_ThrHndl *hndl)
{
  int status=0;

  status |= pthread_cancel(hndl->hndl);
  status |= BSP_thrJoin(hndl);

  return status;
}

int BSP_thrChangePri(BSP_ThrHndl *hndl, int pri)
{
  int status = 0;
  struct sched_param schedprm;

  if(pri>BSP_THR_PRI_MAX)
    pri=BSP_THR_PRI_MAX;
  else
  if(pri<BSP_THR_PRI_MIN)
    pri=BSP_THR_PRI_MIN;

  schedprm.sched_priority = pri;
  status |= pthread_setschedparam(hndl->hndl, SCHED_FIFO, &schedprm);

  return status;
}

int BSP_thrExit(void *returnVal)
{
  pthread_exit(returnVal);
  return 0;
}














