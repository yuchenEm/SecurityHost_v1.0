#ifndef __HAL_RFD_H_
#define __HAL_RFD_H_

#define RFD_CLK_SENDLEN			400		// CLK timebase(us)

#define RFD_INT_FRQ				50

#define RFDCLKEND				0xFFFF

// RFD CLK tolerance
#define  RFD_TITLE_CLK_MINL  	20
#define  RFD_TITLE_CLK_MAXL  	44

#define  RFD_DATA_CLK_MINL   	2
#define  RFD_DATA_CLK_MAXL   	5

// RFD resend times
#define RFD_TX_NUM				15

#define RFD_RX_PORT				GPIOA
#define RFD_RX_PIN				GPIO_Pin_11

#define RFD_NORMAL_DELDOUBLE_TIME  (T500MS+T50MS)

typedef enum
{
	RFDT_CLKSTEP0 = 0,
	RFDT_CLKSTEP1,            
	RFDT_CLKSTEP2,            
}RFD_SENDCLKTypeDef;

 
typedef void (*RFD_RxCallBack_t)(unsigned char *pBuff);

void Hal_RFD_Init(void);
void Hal_RFD_Pro(void);
void Hal_RFD_RxCBF_Register(RFD_RxCallBack_t pCBF);

#endif
