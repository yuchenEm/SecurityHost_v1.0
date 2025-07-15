#ifndef __HAL_KEY_H_
#define __HAL_KEY_H_

// up
#define K1_PORT	GPIOB
#define K1_PIN	GPIO_Pin_3

// down
#define K2_PORT	GPIOB
#define K2_PIN	GPIO_Pin_5

// left
#define K3_PORT	GPIOB
#define K3_PIN	GPIO_Pin_6

// right
#define K4_PORT	GPIOB
#define K4_PIN	GPIO_Pin_7

// cancel/return
#define K5_PORT GPIOB	
#define K5_PIN	GPIO_Pin_10

// confirm/menu
#define K6_PORT GPIOB
#define K6_PIN	GPIO_Pin_11

#define KEY_SCANT_TICK				10	//10ms

#define KEY_SCANTIME				2	//20ms

#define	KEY_PRESS_LONG_TIME			200	//2s

#define KEY_PRESS_CONTINUE_TIME		15	//150ms 

typedef enum
{
	KEY_S1,		// up
	KEY_S2,		// down
	KEY_S3,		// left
	KEY_S4,		// right
	KEY_S5,		// cancel
	KEY_S6,		// confirm/menu
	
	KEY_NUM
}KEY_TYPEDEF;	

typedef enum
{	
	KEY_STEP_WAIT,					
	KEY_STEP_CLICK,					
	KEY_STEP_LONG_PRESS,			
	KEY_STEP_CONTINUOUS_PRESS,  	
}KEY_STEP_TYPEDEF;

typedef enum
{	
	KEY_IDLE,       	 		 		
	KEY_CLICK,          				
	KEY_CLICK_RELEASE,            		
	KEY_LONG_PRESS,			   			
	KEY_LONG_PRESS_CONTINUOUS,			
	KEY_LONG_PRESS_RELEASE				
	 
}KEY_EVENT_TYPEDEF;

typedef enum
{
	KEY_IDLE_VAL,						//0
	
	KEY1_CLICK,							//1
	KEY1_CLICK_RELEASE,					//2
	KEY1_LONG_PRESS,					//3
	KEY1_LONG_PRESS_CONTINUOUS,			//4
	KEY1_LONG_PRESS_RELEASE,			//5
	
	KEY2_CLICK,							//6
	KEY2_CLICK_RELEASE,
	KEY2_LONG_PRESS,
	KEY2_LONG_PRESS_CONTINUOUS,
	KEY2_LONG_PRESS_RELEASE,
	
	KEY3_CLICK,							//11
	KEY3_CLICK_RELEASE,
	KEY3_LONG_PRESS,
	KEY3_LONG_PRESS_CONTINUOUS,
	KEY3_LONG_PRESS_RELEASE,
	
	KEY4_CLICK,							//16
	KEY4_CLICK_RELEASE,
	KEY4_LONG_PRESS,
	KEY4_LONG_PRESS_CONTINUOUS,
	KEY4_LONG_PRESS_RELEASE,
	
	KEY5_CLICK,							//21
	KEY5_CLICK_RELEASE,
	KEY5_LONG_PRESS,
	KEY5_LONG_PRESS_CONTINUOUS,
	KEY5_LONG_PRESS_RELEASE,
	
	KEY6_CLICK,							//26
	KEY6_CLICK_RELEASE,
	KEY6_LONG_PRESS,
	KEY6_LONG_PRESS_CONTINUOUS,
	KEY6_LONG_PRESS_RELEASE,
	
	
}KEY_VALUE_TYPEDEF;


typedef void (*KeyEvent_CallBack_t)(KEY_VALUE_TYPEDEF KeyValue);

void Hal_Key_Init(void);
void Hal_Key_Pro(void);
void Hal_Key_KeyScanCBF_Register(KeyEvent_CallBack_t pCBF);

#endif
