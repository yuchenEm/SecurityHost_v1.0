/************************************************************************************************
* Module: Hal_Key											 				 					*
* Function: Implementing key detection:													 		*
* 		 @ Configure the GPIO for keys and the pin state detection function for each key		*
*		 @ Polling, perform key detection according to the specified process					*
*		 @ Define different key values corresponding to different key behaviors					*
*		 @ Use the detected key value as a parameter to call an external callback function		*
* Description:																 					*
*		@To add a new key: --> Add the corresponding KEY_TYPEDEF in Hal_Key.h and 				*
*		update the key value list by adding the corresponding KEY_VALUE_TYPEDEF in Hal_Key.h	*
*************************************************************************************************/

#include "stm32f10x.h"
#include "hal_key.h"


static void Hal_Key_Config(void);

static unsigned char Hal_Key_GetKey_1_State(void);
static unsigned char Hal_Key_GetKey_2_State(void);
static unsigned char Hal_Key_GetKey_3_State(void);
static unsigned char Hal_Key_GetKey_4_State(void);
static unsigned char Hal_Key_GetKey_5_State(void);
static unsigned char Hal_Key_GetKey_6_State(void);


unsigned char (*GetKeyState[KEY_NUM])() = {  Hal_Key_GetKey_1_State,Hal_Key_GetKey_2_State,Hal_Key_GetKey_3_State,Hal_Key_GetKey_4_State,Hal_Key_GetKey_5_State,Hal_Key_GetKey_6_State
								};

KeyEvent_CallBack_t KeyScanCBF; // KeyScan call-back function

unsigned char KeyStep[KEY_NUM];								
unsigned short KeyScanTime[KEY_NUM];						
unsigned short KeyPressLongTimer[KEY_NUM];					
unsigned short KeyContPressTimer[KEY_NUM];					

/*----------------------------------------------------------------------------
@Name		: Hal_Key_Init()
@Function	: Key initialization
		--> Initialize key pins
		--> Set the callback function pointer KeyScanCBF to Null
		--> Traverse all keys and set the key detection process to KEY_STEP_WAIT
		--> Traverse all keys and set the debounce delay to KEY_SCANTIME
		--> Traverse all keys and set the long-press delay to KEY_PRESS_LONG_TIME
		-->  Traverse all keys and set the continuous long-press delay to KEY_PRESS_CONTINUE_TIME
@Parameter	: Null
------------------------------------------------------------------------------*/
void Hal_Key_Init(void)
{
	unsigned char i;
	KeyScanCBF = 0;
	Hal_Key_Config();
 
	for(i=0; i<KEY_NUM; i++)
	{
		KeyStep[i] = KEY_STEP_WAIT;
		KeyScanTime[i] = KEY_SCANTIME;
		KeyPressLongTimer[i] = KEY_PRESS_LONG_TIME;
		KeyContPressTimer[i] = KEY_PRESS_CONTINUE_TIME;
	}
}

/*----------------------------------------------------------------------------
@Name		: Hal_Key_KeyScanCBF_Register(pCBF)
@Function	: Function: Register the key scan callback function, 
			  which passes the externally provided function pointer to KeyScanCBF
@Parameter	: pCBF->the incoming callback function pointer
------------------------------------------------------------------------------*/
void Hal_Key_KeyScanCBF_Register(KeyEvent_CallBack_t pCBF)
{
	if(KeyScanCBF == 0)
	{
		KeyScanCBF = pCBF;
	}
}	
								
/*----------------------------------------------------------------------------
@Name		: Hal_Key_Pro()
@Function	: polling function, according to the process update the KeyValue
@Parameter	: Null
		KeyValue: captured key value
------------------------------------------------------------------------------*/
unsigned char KeyValue;
								
void Hal_Key_Pro(void)
{
	unsigned char i;
	unsigned char KeyState[KEY_NUM];
	
	for(i=0; i<KEY_NUM; i++) 
	{	
		KeyValue = 0; 
 
		KeyState[i] = GetKeyState[i]();
		switch(KeyStep[i]) 
		{
			case KEY_STEP_WAIT:		
				if(KeyState[i])
				{
					KeyStep[i] = KEY_STEP_CLICK;	
				}
			break;
				
			case KEY_STEP_CLICK:			 	
				if(KeyState[i])
				{
					if(!(--KeyScanTime[i]))
					{
						KeyScanTime[i] = KEY_SCANTIME;
						KeyStep[i] = KEY_STEP_LONG_PRESS;
						KeyValue = (i*5)+1;		
					}
				}
				else
				{
					KeyScanTime[i] = KEY_SCANTIME;
					KeyStep[i] = KEY_STEP_WAIT;
				}
			break;
				
			case KEY_STEP_LONG_PRESS:			
				if(KeyState[i])
				{	
					if(!(--KeyPressLongTimer[i]))
					{
						KeyPressLongTimer[i] = KEY_PRESS_LONG_TIME;
						KeyStep[i] = KEY_STEP_CONTINUOUS_PRESS;
						KeyValue = (i*5)+3;		
					 
					}
				}
				else
				{
					KeyPressLongTimer[i] = KEY_PRESS_LONG_TIME;
					KeyStep[i] = KEY_STEP_WAIT;
					KeyValue = (i*5)+2;			
				}
			break;
				
			case KEY_STEP_CONTINUOUS_PRESS:
				if(KeyState[i])
				{
					if(!(--KeyContPressTimer[i]))
					{
						KeyContPressTimer[i] = KEY_PRESS_CONTINUE_TIME;
						KeyValue = (i*5)+4;		
					}
				}
				else
				{
					KeyStep[i] = KEY_STEP_WAIT;
					KeyContPressTimer[i] = KEY_PRESS_CONTINUE_TIME;
					KeyValue = (i*5)+5;			
				}				 
			break;	 		
		}
		
		if(KeyValue)
		{
			if(KeyScanCBF)
			{	 
				KeyScanCBF((KEY_VALUE_TYPEDEF)KeyValue); 
			}
		}
	 
	}
}

/*----------------------------------------------------------------------------
@Name		: Hal_Key_Config()
@Function	: Key config 
@Parameter	: Null
------------------------------------------------------------------------------*/
static void Hal_Key_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB |RCC_APB2Periph_AFIO, ENABLE);
	
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = K1_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(K1_PORT, &GPIO_InitStructure);
	
	 
	GPIO_InitStructure.GPIO_Pin = K2_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(K2_PORT, &GPIO_InitStructure);
	
	 
	GPIO_InitStructure.GPIO_Pin = K3_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(K3_PORT, &GPIO_InitStructure);
	 
	GPIO_InitStructure.GPIO_Pin = K4_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(K4_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = K5_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(K5_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = K6_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(K6_PORT, &GPIO_InitStructure);
}

/*--------------------------- GPIO Pinout state ----------------------------------------*/
static unsigned char Hal_Key_GetKey_1_State(void)
{
	return (!GPIO_ReadInputDataBit(K1_PORT, K1_PIN));		
} 

static unsigned char Hal_Key_GetKey_2_State(void)
{
	return (!GPIO_ReadInputDataBit(K2_PORT, K2_PIN));		
}

 
static unsigned char Hal_Key_GetKey_3_State(void)
{
	return (!GPIO_ReadInputDataBit(K3_PORT, K3_PIN));		
}

static unsigned char Hal_Key_GetKey_4_State(void)
{
	return (!GPIO_ReadInputDataBit(K4_PORT, K4_PIN));		
}

static unsigned char Hal_Key_GetKey_5_State(void)
{
	return (!GPIO_ReadInputDataBit(K5_PORT, K5_PIN));		
}

static unsigned char Hal_Key_GetKey_6_State(void)
{
	return (!GPIO_ReadInputDataBit(K6_PORT, K6_PIN));		
}

