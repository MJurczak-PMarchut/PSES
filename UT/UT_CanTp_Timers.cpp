#ifndef UT_CANTP_TIMERS_CPP_
#define UT_CANTP_TIMERS_CPP_
#include <stdlib.h>

#define TEST_NO_MAIN

#include "acutest.h"
#include "UT_CanTp_Timers.hpp"
#include "CanTp_Timers.c"


void test_CanTp_TStart(void)
{
	CanTp_Timer_type test_timer = {TIMER_NOT_ACTIVE, 0, 10};
	CanTp_TStart(&test_timer);
	TEST_CHECK(test_timer.state == TIMER_ACTIVE);
}

void test_CanTp_Timer_Incr(void)
{
	CanTp_Timer_type test_timer1 = {TIMER_NOT_ACTIVE, 4, 10};
	CanTp_Timer_type test_timer2 = {TIMER_ACTIVE, 4, 10};
	Std_ReturnType ret = E_OK;

	ret = CanTp_Timer_Incr(&test_timer1);
	TEST_CHECK(ret == E_NOT_OK);
	TEST_CHECK(test_timer1.counter == 4);


	ret = CanTp_Timer_Incr(&test_timer2);
	TEST_CHECK(ret == E_OK);
	TEST_CHECK(test_timer2.counter == 5);
}

void test_CanTp_Timer_Timeout(void)
{
	uint32 timeout = 10;
	CanTp_Timer_type test_timer = {TIMER_ACTIVE, 0, timeout};
	Std_ReturnType ret = E_OK;

	test_timer.counter = timeout - 1;
	ret = CanTp_Timer_Timeout(&test_timer);
	TEST_CHECK(ret == E_OK);

	test_timer.counter = timeout;
	ret = CanTp_Timer_Timeout(&test_timer);
	TEST_CHECK(ret == E_NOT_OK);

	test_timer.counter = timeout + 1;
	ret = CanTp_Timer_Timeout(&test_timer);
	TEST_CHECK(ret == E_NOT_OK);
}


#endif /* UT_CANTP_TIMERS_CPP_ */





