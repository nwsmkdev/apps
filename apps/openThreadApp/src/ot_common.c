#include <stdlib.h>

#include "sysinit/sysinit.h"
#include <dw1000/dw1000_mac.h>
#include <dw1000/dw1000_hal.h>
#include "console/console.h"

#include <ot_common.h>

#include <openthread/types.h>
#include <openthread/platform/uart.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/platform.h>
#include <openthread/platform/alarm.h>
#include <openthread/platform/usec-alarm.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/radio.h>
#include <openthread/tasklet.h>
#include <hal/hal_flash.h>

void __cxa_pure_virtual() { while (1); }

struct os_callout task_callout;
static bool taskletprocess = false;

ot_instance_t * ot_global_inst;

void PlatformInit(struct _dw1000_dev_instance_t* inst){
	inst->config.rx.phrMode = DWT_PHRMODE_EXT;
	sysinit();
    return ;
}

otError otPlatRandomGetTrue(uint8_t *aOutput, uint16_t aOutputLength){

#if MYNEWT_VAL(OT_DEBUG)
	printf("# %s #\n",__func__);
#endif
    otError error = OT_ERROR_NONE;

    return error;
}

uint32_t otPlatRandomGet(void){
    srand(os_cputime_get32()); 
    uint32_t rng  = (uint32_t)rand();
    return (uint32_t)rng;
}

uint32_t utilsFlashGetSize(void){

#if MYNEWT_VAL(OT_DEBUG)
	printf("# %s #\n",__func__);
#endif
	return (0x70000 - 0x8000);
}

uint32_t utilsFlashWrite(uint32_t aAddress, uint8_t *aData, uint32_t aSize){


#if MYNEWT_VAL(OT_DEBUG)
	printf("# %s #\n",__func__);
#endif
    int rc = hal_flash_write(0, aAddress, aData, aSize);
    return rc;
}

uint32_t utilsFlashRead(uint32_t aAddress, uint8_t *aData, uint32_t aSize){

#if MYNEWT_VAL(OT_DEBUG)
	printf("# %s : 0x%lX #\n",__func__, aAddress);
#endif
    int rc = hal_flash_read(0, aAddress, aData, aSize);
    return rc;
}

otError utilsFlashInit(void){
 
#if MYNEWT_VAL(OT_DEBUG)
	printf("# %s #\n",__func__);
#endif
    return OT_ERROR_NONE;
}

otError utilsFlashStatusWait(uint32_t aTimeout){

    otError error = OT_ERROR_NONE;

#if MYNEWT_VAL(OT_DEBUG)
	printf("# %s #\n",__func__);
#endif
    return error;
}

otError utilsFlashErasePage(uint32_t aAddress){

#if MYNEWT_VAL(OT_DEBUG)
	printf("# %s #\n",__func__);
#endif
    int rc = hal_flash_erase_sector(0, aAddress);
    return rc;
}

void otPlatReset(otInstance *aInstance){

#if MYNEWT_VAL(OT_DEBUG)
	printf("# %s #\n",__func__);
#endif
    (void)aInstance;
    NVIC_SystemReset();
}

void otPlatRadioSetDefaultTxPower(otInstance *aInstance, int8_t aPower){

    // TBD for Setting Default transmit power
#if MYNEWT_VAL(OT_DEBUG)
	printf("# %s #\n",__func__);
#endif
    (void)aInstance;
    (void)aPower;
}

static void tasklet_sched(struct os_event* ev){
    ot_instance_t* ot = (ot_instance_t*)ev->ev_arg;
    otInstance* aInstance = ot->sInstance;
    if(taskletprocess == true){
        taskletprocess = false;
        otTaskletsProcess(aInstance);
    }
}

void otTaskletsSignalPending(otInstance *aInstance)
{
    taskletprocess = true;
    if(ot_global_inst->status.initialized == 1)
        os_eventq_put(&ot_global_inst->eventq, &task_callout.c_ev);
    else
        otTaskletsProcess(aInstance);
}
#if 1
static void ot_task(void *arg)
{
    ot_instance_t *ot = (ot_instance_t*)arg;
    while (1) {
        os_eventq_run(&ot->eventq);
    }
}
#endif

ot_instance_t *
ot_init(dw1000_dev_instance_t * inst){

	assert(inst);

	if (inst->ot == NULL ){
		inst->ot  = (ot_instance_t *) malloc(sizeof(ot_instance_t));
		assert(inst->ot);
        memset(inst->ot, 0x00, sizeof(ot_instance_t));
		inst->ot->status.selfmalloc = 1;
	}
	inst->ot->dev = inst;
    inst->ot->task_prio = inst->task_prio + 0x7;

	os_error_t err = os_sem_init(&inst->ot->sem, 0x01);
	assert(err == OS_OK);
	
    ot_global_inst = inst->ot;
    
    nrf5AlarmInit(inst);
    RadioInit(inst);
	
    return inst->ot;
}

void ot_post_init(dw1000_dev_instance_t* inst, otInstance *aInstance){
    ot_global_inst = inst->ot;
    inst->ot->sInstance = aInstance;
    os_eventq_init(&inst->ot->eventq);
    os_task_init(&inst->ot->task_str, "ot_task",
            ot_task,
            (void *)inst->ot,
            inst->ot->task_prio,
            OS_WAIT_FOREVER,
            inst->ot->task_stack,
            DW1000_DEV_TASK_STACK_SZ * 2);
    os_callout_init(&task_callout, &inst->ot->eventq, tasklet_sched , (void*)inst->ot);

	inst->ot->status.initialized = 1;
}

