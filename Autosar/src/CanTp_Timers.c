#include "CanTp_Timers.h"

#ifndef UINT32_MAX
#define UINT32_MAX 0xFFFFFFFF
#endif

void CanTp_TStart(CanTp_Timer_type *timer){

    timer->state = TIMER_ACTIVE;
}

void CanTp_TReset(CanTp_Timer_type *timer){
    timer->state = TIMER_NOT_ACTIVE;
    timer->counter = 0;
}

Std_ReturnType CanTp_Timer_Incr(CanTp_Timer_type *timer){
    Std_ReturnType ret = E_OK;
    if(timer->state == TIMER_ACTIVE){
        if(timer->counter < UINT32_MAX){
            timer->counter ++;
        }
        else{
            ret = E_NOT_OK;
        }
    }
    else{
    	ret = E_NOT_OK;
    }
    return ret;
}

Std_ReturnType CanTp_Timer_Timeout(const CanTp_Timer_type *timer){
    if(timer->counter >= timer->timeout){
        return E_NOT_OK;
    }
    else{
        return E_OK;
    }
}
