#ifndef __HAL_LED_H_
#define __HAL_LED_H_

#define LED1_PORT			GPIOA
#define LED1_PIN			GPIO_Pin_1

#define BUZ_PORT			GPIOB
#define BUZ_PIN				GPIO_Pin_0

#define LED_EFFECT_END	0xFFFE
#define LED_EFFECT_AGN	0xFFFF

typedef enum
{
	LED_OFF,
	LED_ON,
	LED_ON_100MS,
	LED_BLINK1,
	LED_BLINK2,
	LED_BLINK3,
	LED_BLINK4,
	
}LED_EFFECT_TYPEDEF;

typedef enum
{
	LED_1,
	BUZ,
	
	LED_TARGET_SUM
}LED_TARGET_TYPEDEF;

void Hal_LED_Init(void);
void Hal_LED_Pro(void);
void Hal_LED_MsgInput(unsigned char target, LED_EFFECT_TYPEDEF cmd, unsigned char CutIn);


#endif
