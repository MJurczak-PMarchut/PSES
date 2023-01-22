#ifndef CAN_TP_TIMERS_H
#define CAN_TP_TIMERS_H

#include "Std_Types.h"

#define N_AR_TIMEOUT_VAL 100
#define N_BR_TIMEOUT_VAL 100
#define N_CR_TIMEOUT_VAL 100

#define N_AS_TIMEOUT_VAL 100
#define N_BS_TIMEOUT_VAL 100
#define N_CS_TIMEOUT_VAL 100

typedef enum{
    TIMER_ACTIVE,
    TIMER_NOT_ACTIVE
} timer_state_t;

typedef struct{
    timer_state_t state;
    uint32        counter;
    const uint32   timeout;
} CanTp_Timer_type;

void CanTp_TStart(CanTp_Timer_type *timer);
void CanTp_TReset(CanTp_Timer_type *timer);
Std_ReturnType CanTp_Timer_Incr(CanTp_Timer_type *timer);
Std_ReturnType CanTp_Timer_Timeout(const CanTp_Timer_type *timer);

#endif /*CAN_TP_TIMERS_H*/
