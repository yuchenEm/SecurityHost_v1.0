/****************************************************************************
* Module: Hal_LED										 				 	*
* Function: Implementing an LED effect matrix: 								*
* 		 @ Configure hardware parameters of the LED object					*
*		 @ Polling LED commands 									 		*
*		 @ Select the corresponding LED command based on external events	*
*		 @ Drive the specified object to output the specified effect		*
* Description: 																*
*		@To add a new object: 												*
*		--> Add the corresponding LED_TARGET_TYPEDEF in Hal_LED.h		 	*
*		@To add a new effect: 												*
*		--> Add the corresponding LED_EFFECT_TYPEDEF in Hal_LED.h and 		*
*			add the corresponding Led_CMD[] LED command in Hal_LED.c		*
*****************************************************************************/

#include "stm32f10x.h"
#include "hal_led.h"
#include "hal_timer.h"
#include "os_system.h"

static void Hal_LED_1_Drive(unsigned char sta);
static void Hal_Buz_Drive(unsigned char sta);
static void Hal_LED_Handler(void);
static void Hal_LED_Config(void);

Queue4	LED_CMDBuffer[LED_TARGET_SUM]; 

unsigned short Led_Off[] = {0,10,LED_EFFECT_END};
unsigned short Led_On[] = {1,10,LED_EFFECT_END};
unsigned short Led_On_100ms[] = {1,10,0,10,LED_EFFECT_END};	
unsigned short Led_Blink1[] = {1,10,0,10,LED_EFFECT_AGN,2};
unsigned short Led_Blink2[] = {1,10,0,10,1,10,0,10,1,10,0,200,LED_EFFECT_AGN,6};
unsigned short Led_Blink3[] = {1,30,0,30,LED_EFFECT_AGN,2};
unsigned short Led_Blink4[] = {1,50,0,50,LED_EFFECT_AGN,2};

unsigned char LED_LoadFlag[LED_TARGET_SUM]; 

unsigned short *pLED[LED_TARGET_SUM];
unsigned short LED_Timer[LED_TARGET_SUM];

void (*Hal_LED_Drive[LED_TARGET_SUM])(unsigned char) = { Hal_LED_1_Drive,Hal_Buz_Drive,
};

/*----------------------------------------------------------------------------
@Name		: Hal_LED_Init()
@Function	: LED module initialization
		--> LED GPIO init
		--> creat LED Timer T_LED， Hal_LED_Handler as the Interrupt handler， period 10ms
		--> clear idle-flag for all registered devices 
		--> empty LED_CMDBuffer
@Parameter	: Null
------------------------------------------------------------------------------*/
void Hal_LED_Init(void)
{
	unsigned char i;
	Hal_LED_Config();
	Hal_Timer_CreatTimer(T_LED, Hal_LED_Handler, 200, T_STATE_START); // timebase=50us;
	
	for(i=0; i<LED_TARGET_SUM; i++)
	{
		LED_LoadFlag[i] = 0;
		pLED[i] = (unsigned short *)Led_Off;
		LED_Timer[i] = *(pLED[i]+1);
		QueueEmpty(LED_CMDBuffer[i]);
	}
	
	Hal_LED_MsgInput(LED_1,LED_OFF,1);
	Hal_LED_MsgInput(BUZ,LED_OFF,1);
}

/*----------------------------------------------------------------------------
@Name		: Hal_LED_Pro()
@Function	: LED polling function
@Parameter	: Null
------------------------------------------------------------------------------*/
void Hal_LED_Pro(void)
{
	unsigned char i;
	unsigned char cmd;
	
	for(i=0; i<LED_TARGET_SUM; i++)
	{
		if((QueueDataLen(LED_CMDBuffer[i]) > 0) && (LED_LoadFlag[i] == 0))
		{
			QueueDataOut(LED_CMDBuffer[i], &cmd);
			LED_LoadFlag[i] = 1;
			switch(cmd)
			{
				case LED_OFF:
					pLED[i] = (unsigned short *)Led_Off; 
					LED_Timer[i] = *(pLED[i]+1); 		 
					break;
					
				case LED_ON:
					pLED[i] = (unsigned short *)Led_On;
					LED_Timer[i] = *(pLED[i]+1);
					break;
					
				case LED_ON_100MS:
					pLED[i] = (unsigned short *)Led_On_100ms;
					LED_Timer[i] = *(pLED[i]+1);
					break;
					
				case LED_BLINK1:
					pLED[i] = (unsigned short *)Led_Blink1;
					LED_Timer[i] = *(pLED[i]+1);
					break;
					
				case LED_BLINK2:
					pLED[i] = (unsigned short *)Led_Blink2;
					LED_Timer[i] = *(pLED[i]+1);
					break;
					
				case LED_BLINK3:
					pLED[i] = (unsigned short *)Led_Blink3;
					LED_Timer[i] = *(pLED[i]+1);
					break;
					
				case LED_BLINK4:
					pLED[i] = (unsigned short *)Led_Blink4;
					LED_Timer[i] = *(pLED[i]+1);
					break;
			}
		}
	}
}

/*----------------------------------------------------------------------------
@Name		: Hal_LED_MsgInput(target, cmd, CutIn)
@Function	: LED effect input
@Parameter	: 
	target	: target
	cmd		: effect type
	CutIn	: 1->cut-in, 0->waiting
------------------------------------------------------------------------------*/
void Hal_LED_MsgInput(unsigned char target, LED_EFFECT_TYPEDEF cmd, unsigned char CutIn)
{
	unsigned char LED_CMD;
	
	if(target >= LED_TARGET_SUM)
	{
		return;
	}
	LED_CMD = cmd;
	if(CutIn) 
	{
		QueueEmpty(LED_CMDBuffer[target]);
		LED_LoadFlag[target] = 0; 
	}
	QueueDataIn(LED_CMDBuffer[target], &LED_CMD, 1);
}

/*----------------------------------------------------------------------------
@Name		: Hal_LED_Handler()
@Function	: LED command handler
@Parameter	: Null
------------------------------------------------------------------------------*/
static void Hal_LED_Handler(void)
{
	unsigned char i;

	for(i=0; i<LED_TARGET_SUM; i++)
	{
		if(LED_Timer[i])
		{
			LED_Timer[i]--;
		}
		if(!LED_Timer[i]) 
		{
			if(*(pLED[i]+2) == LED_EFFECT_END)
			{
					LED_LoadFlag[i] = 0;
			}
			else
			{
				pLED[i] += 2;
				if(*pLED[i] == LED_EFFECT_AGN)
				{
					pLED[i] = pLED[i] - (*(pLED[i]+1) * 2);
				}
				LED_Timer[i] = *(pLED[i]+1);
			}
		}
		Hal_LED_Drive[i](*pLED[i]);
	}
	Hal_Timer_ResetTimer(T_LED,T_STATE_START);
}

/*----------------------------------------------------------------------------
@Name		: Hal_LED_Config()
@Function	: LED GPIO config	
@Parameter	: Null
------------------------------------------------------------------------------*/
static void Hal_LED_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA , ENABLE); 						 
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB , ENABLE); 	


	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; ; 
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA,GPIO_Pin_1);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; ; 
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOB,GPIO_Pin_0);	
	
}

/*--------------------------------LED GPIO Driver----------------------------------------------*/
static void Hal_LED_1_Drive(unsigned char sta)
{
	if(sta)
	{
		GPIO_SetBits(LED1_PORT,LED1_PIN);
	}
	else
	{
		GPIO_ResetBits(LED1_PORT,LED1_PIN);
	}
}

static void Hal_Buz_Drive(unsigned char sta)
{
	if(sta)
	{
		GPIO_SetBits(BUZ_PORT,BUZ_PIN);
	}
	else
	{
		GPIO_ResetBits(BUZ_PORT,BUZ_PIN);
	}
}

