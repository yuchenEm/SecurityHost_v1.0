#ifndef __HAL_TIMER_H_
#define __HAL_TIMER_H_

/*************************************************************
	@TimeBase define:
**************************************************************/
#define TimeBase_5us	5
#define TimeBase_10us	10
#define TimeBase_20us	20
#define TimeBase_50us	50
#define TimeBase_100us	100
#define TimeBase_500us	500
#define TimeBase_1ms	1000
#define TimeBase_5ms	5000
#define TimeBase_10ms	10000
#define TimeBase_20ms	20000
#define TimeBase_50ms	50000

/************************************************************/

typedef enum
{
	T_LED,	
	T_RFD_PULSE_RX,		// RFD pulse collect timer
	T_RFD_RECODE_FLT,	// RFD re-code timer
	T_BEEP,
	
	T_SUM,
}TIMER_ID_TYPEDEF;

typedef enum
{
	T_SUCCESS,
	T_FAIL,	
}TIMER_RESULT_TYPEDEF;

typedef enum
{
	T_STATE_INVALID, //0
	T_STATE_STOP,	
	T_STATE_START,	
}TIMER_STATE_TYPEDEF;

typedef struct
{
	TIMER_STATE_TYPEDEF state; 	// INVALID: failed; STOP: timer idle; START: timer run
	unsigned char CompleteFlag; // 0: not complete; 1: complete
	unsigned short CurrentCount; 
	unsigned short Period; 
	void (*func)(void); 
}Stu_TimerTypedef;

void Hal_Timer_Init(void);
void Hal_Timer_CreatTimer(TIMER_ID_TYPEDEF ID, void (*proc)(void), unsigned short Period, TIMER_STATE_TYPEDEF State);
TIMER_RESULT_TYPEDEF Hal_Timer_ResetTimer(TIMER_ID_TYPEDEF ID, TIMER_STATE_TYPEDEF State);
TIMER_RESULT_TYPEDEF Hal_Timer_TimerDelete(TIMER_ID_TYPEDEF ID);
TIMER_RESULT_TYPEDEF Hal_Timer_StateControl(TIMER_ID_TYPEDEF ID,TIMER_STATE_TYPEDEF State);
TIMER_STATE_TYPEDEF	Hal_Timer_GetState(TIMER_ID_TYPEDEF ID);

#endif
