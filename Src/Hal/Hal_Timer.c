/********************************************************************************
* Module: Hal_Timer											 					*
* Function: Implementing a timer matrix: 										*
*		@ Configure timer parameters 									 		*
*		@ Create a timer for a specified object 								*
*		@ When the timer reaches the specified time, 							*
*		  execute the provided callback function					 			*
* Description:																	*
*		@ To add a new object: 													*
*		--> Add the corresponding TIMER_ID_TYPEDEF in Hal_Timer.h				*
*		@ To add a new TimeBase: 												*
*		--> Add the corresponding TimeBase define macro in Hal_Timer.h			*
*		@ To modify a TimeBase: 												*
*		--> Change the .TIM_Period value in Hal_Timer_Config()					*
*********************************************************************************/

#include "stm32f10x.h" 
#include "hal_timer.h"
#include "hal_led.h"

static void Hal_Timer_Config(void);
static void Hal_Timer_TimerHandler(void);

volatile Stu_TimerTypedef Stu_Timer[T_SUM];

/******************************************************************
	@Name		: Hal_Timer_Init
	@Function	: timer inital(API)
*******************************************************************/
void Hal_Timer_Init(void)
{
	unsigned char i;
	Hal_Timer_Config();
	
	for(i=0; i<T_SUM; i++)
	{
		Stu_Timer[i].state = T_STATE_STOP;
		Stu_Timer[i].CurrentCount = 0;
		Stu_Timer[i].func = 0;
		Stu_Timer[i].Period = 0;
	}
}

/*******************************************************************
	@Name		: Hal_Timer_CreatTimer
	@Function	: creat timer 
	@Parameters	: 
		* TIMER_ID_TYPEDEF ID
		* void (*proc)(void)
		* unsigned short Period
		* TIMER_STATE_TYPEDEF State
********************************************************************/
void Hal_Timer_CreatTimer(TIMER_ID_TYPEDEF ID, void (*proc)(void), unsigned short Period, TIMER_STATE_TYPEDEF State)
{
	Stu_Timer[ID].state = State;
	Stu_Timer[ID].CurrentCount = 0;
	Stu_Timer[ID].func = proc;
	Stu_Timer[ID].Period = Period;
}

/*******************************************************************
	@Name		: Hal_ResetTimer
	@Function	: Reset timer 
	@Parameters	: 
		* TIMER_ID_TYPEDEF ID
		* TIMER_STATE_TYPEDEF State
********************************************************************/
TIMER_RESULT_TYPEDEF Hal_Timer_ResetTimer(TIMER_ID_TYPEDEF ID, TIMER_STATE_TYPEDEF State)
{
	if(Stu_Timer[ID].func)
	{
		Stu_Timer[ID].state = State;
		Stu_Timer[ID].CurrentCount = 0;
		return T_SUCCESS;
	}
	else
	{
		return T_FAIL;
	}
}

/******************************************************************
	@Name		: Hal_TimerDelete
	@Function	: Delete timer 
	@Parameters	: 
		* TIMER_ID_TYPEDEF ID
*******************************************************************/
TIMER_RESULT_TYPEDEF Hal_Timer_TimerDelete(TIMER_ID_TYPEDEF ID)
{
	if(Stu_Timer[ID].func)
	{
		Stu_Timer[ID].state = T_STATE_STOP;
		Stu_Timer[ID].CurrentCount = 0;
		Stu_Timer[ID].func = 0;
		return T_SUCCESS;
	}
	else
	{
		return T_FAIL;
	}
}

/********************************************************************
	@Name		: Hal_Timer_StateControl
	@Function	: change timer state
	@Parameters	: 
		* TIMER_ID_TYPEDEF ID
		* TIMER_STATE_TYPEDEF State
*********************************************************************/
TIMER_RESULT_TYPEDEF Hal_Timer_StateControl(TIMER_ID_TYPEDEF ID,TIMER_STATE_TYPEDEF State)
{
	if(Stu_Timer[ID].func)
	{
		Stu_Timer[ID].state = State;
		return T_SUCCESS;
	}
	else
	{
		return T_FAIL;
	}
}

/********************************************************************
	@Name		: Hal_Timer_GetState
	@Function	: get timer state
	@Parameters	: 
		* TIMER_ID_TYPEDEF ID
*********************************************************************/
TIMER_STATE_TYPEDEF	Hal_Timer_GetState(TIMER_ID_TYPEDEF ID)
{
	if(Stu_Timer[ID].func)
	{
		return Stu_Timer[ID].state;
	 
	}
	else
	{
		return T_STATE_INVALID;
	}
}

/******************************************************************
	@Name		: Hal_Timer_Config(static)
	@Function	: config timer parameters
	@TimeBase	: TIM_Period
		* TimeBase_5us		: 5 us
		* TimeBase_10us		: 10 us
		* TimeBase_20us		: 20 us
		* TimeBase_50us		: 50 us
		* TimeBase_100us	: 100 us
		* TimeBase_500us	: 500 us
		* TimeBase_1ms		: 1 ms
		* TimeBase_5ms		: 5 ms
		* TimeBase_10ms		: 10 ms
		* TimeBase_20ms		: 20 ms
		* TimeBase_50ms		: 50 ms
*******************************************************************/
static void Hal_Timer_Config(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	
	TIM_DeInit(TIM4);
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1; 	// indicates the division ratio between the timer clock (CK_INT) frequency and sampling clock used by the digital filters (ETR, TIx)
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = TimeBase_50us-1; 		// Auto Reload Register ARR(16bits): 0-65535, ARR:1-65536
	TIM_TimeBaseInitStructure.TIM_Prescaler = 72-1; 				// 72MHz->1us, PSC:1-65536, PSC Register(16bits):0-65535
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0; 			// only for advacnced Timer1, Timer8
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStructure);
	
	TIM_ClearFlag(TIM4, TIM_FLAG_Update); 		
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE); 
	TIM_Cmd(TIM4, ENABLE);
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&NVIC_InitStructure);
}

/*************************************************************************
	@Name		: Hal_Timer_TimerHandler(static)
	@Function	: Timer handler for interrupt
**************************************************************************/
static void Hal_Timer_TimerHandler(void)
{
	unsigned char i;
	for(i=0; i<T_SUM; i++)
	{
		if((Stu_Timer[i].func) && (Stu_Timer[i].state == T_STATE_START))
		{
			Stu_Timer[i].CurrentCount++;
			if(Stu_Timer[i].CurrentCount >= Stu_Timer[i].Period)
			{
				Stu_Timer[i].state = T_STATE_STOP;
				Stu_Timer[i].CurrentCount = Stu_Timer[i].CurrentCount; 
				Stu_Timer[i].func(); 
			}
		}
	}
}


/******************************************************************
	@Name		: TIM4_IRQHandler
	@Function	: TIM4 Interrupt handler
*******************************************************************/
void TIM4_IRQHandler(void)
{
	TIM_ClearFlag(TIM4, TIM_FLAG_Update);
	Hal_Timer_TimerHandler(); // timebase: 1ms	
}
