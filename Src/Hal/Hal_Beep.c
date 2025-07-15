#include "stm32F10x.h"
#include "hal_timer.h"
#include "hal_beep.h"

#define BEEP_PORT			GPIOB
#define BEEP_PIN			GPIO_Pin_4

#define BEEP_EN_PORT		GPIOA
#define BEEP_EN_PIN			GPIO_Pin_0

#define BEEP_110_STEP_FREQ 	58

//#define BP_FRQ1	500
//#define BP_FRQ2	BP_FRQ1+33
//#define BP_FRQ3	BP_FRQ2+33
//#define BP_FRQ4	BP_FRQ3+33
//#define BP_FRQ5	BP_FRQ4+33
//#define BP_FRQ6	BP_FRQ5+33
//#define BP_FRQ7	BP_FRQ6+33
//#define BP_FRQ8	BP_FRQ7+33
//#define BP_FRQ9	BP_FRQ8+33
//#define BP_FRQ10	BP_FRQ9+33
//#define BP_FRQ11	BP_FRQ10+33
//#define BP_FRQ12	BP_FRQ11+33
//#define BP_FRQ13	BP_FRQ12+33
//#define BP_FRQ14	BP_FRQ13+33
//#define BP_FRQ15	BP_FRQ14+33

#define BP_FRQ1		260
#define BP_FRQ2		BP_FRQ1+49
#define BP_FRQ3		BP_FRQ2+49
#define BP_FRQ4		BP_FRQ3+49
#define BP_FRQ5		BP_FRQ4+49
#define BP_FRQ6		BP_FRQ5+49
#define BP_FRQ7		BP_FRQ6+49
#define BP_FRQ8		BP_FRQ7+49
#define BP_FRQ9		BP_FRQ8+49
#define BP_FRQ10	BP_FRQ9+49
#define BP_FRQ11	BP_FRQ10+49
#define BP_FRQ12	BP_FRQ11+49
#define BP_FRQ13	BP_FRQ12+49
#define BP_FRQ14	BP_FRQ13+49
#define BP_FRQ15	BP_FRQ14+49

//unsigned short NoteFreqAry[12] = {955,901,851,803,758,715,675,637,601,568,536,506};
//unsigned short NoteFreqAry[12] = {253,268,284,300,318,337,357,379,401,425,450,477};

//unsigned short NoteFreqAry[2] = {253,268,284,300,318,337,357,379,401,425,450,477};
unsigned short NoteFreqAry[30] = {
		
		BP_FRQ1,BP_FRQ2,BP_FRQ3,BP_FRQ4,BP_FRQ5,BP_FRQ6,BP_FRQ7,BP_FRQ8,BP_FRQ9,BP_FRQ10,BP_FRQ11,BP_FRQ12,BP_FRQ13,BP_FRQ14,BP_FRQ15,
		BP_FRQ15,BP_FRQ14,BP_FRQ13,BP_FRQ12,BP_FRQ11,BP_FRQ10,BP_FRQ9,BP_FRQ8,BP_FRQ7,BP_FRQ6,BP_FRQ5,BP_FRQ4,BP_FRQ3,BP_FRQ2
};

static void Hal_Beep_Config(void);
 
static void Hal_Beep_PWMHandler(void);


static void Hal_Beep_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOA , ENABLE); 	
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_AFIO, ENABLE);
	
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE); 

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);	 
	
	GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE);   
 
	GPIO_InitStructure.GPIO_Pin = BEEP_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; ; 
	GPIO_Init(BEEP_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = BEEP_EN_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; ; 
	GPIO_Init(BEEP_EN_PORT, &GPIO_InitStructure);
	GPIO_SetBits(BEEP_EN_PORT,BEEP_EN_PIN);
	
 
	TIM_TimeBaseStructure.TIM_Period = 100 - 1; 					   	// ARR = 100
	TIM_TimeBaseStructure.TIM_Prescaler = SystemCoreClock/1000000 - 1; 	// PSC = 72
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; 
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); 
	
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2; 				
 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; 	
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; 

	TIM_OCInitStructure.TIM_Pulse = 50;									// duty = 50/100
	TIM_OC1Init(TIM3, &TIM_OCInitStructure);  
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);  

	TIM_Cmd(TIM3, ENABLE);  

}


void Hal_Beep_Init(void)
{
	Hal_Beep_Config();
	Hal_Timer_CreatTimer(T_BEEP, Hal_Beep_PWMHandler, 120, T_STATE_START); // TimeBase: 50us, period = 6ms
}

void Hal_Beep_Pro(void)
{
	
}

void Hal_Beep_PWMCtrl(unsigned char cmd)
{
 
	if(cmd)
	{
		GPIO_ResetBits(BEEP_EN_PORT,BEEP_EN_PIN);
	}
	else
	{
		GPIO_SetBits(BEEP_EN_PORT,BEEP_EN_PIN);
	}

}

static void Hal_Beep_PWMHandler(void)
{
	static unsigned char i=0;

	TIM_SetAutoreload(TIM3,NoteFreqAry[i]);
	TIM_SetCompare1(TIM3,NoteFreqAry[i]/2);
	TIM_SetCounter(TIM3,0);

	i++;
	if(i>28)
	{
		i=0;
	}
	
	Hal_Timer_ResetTimer(T_BEEP,T_STATE_START);
	 
}
 
