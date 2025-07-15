#ifndef __HAL_USART_H_
#define __HAL_USART_H_

/*------------------------------------------------------------------------------------------
  Disable USART debugging functions by commenting out the corresponding macro definitions:
      (1) USART1 receives data echo              DEBUG_PRINT_USART1_RX
      (2) USART1 receives data and transmits it to USART2    DEBUG_PRINT_USART1RX_TO_USART2TX
      (3) USART2 receives data and transmits it to USART1    DEBUG_PRINT_USART2RX_TO_USART1TX
  USART1 cannot use the transparent transmission function in monitoring mode: 
            to prevent USART1 data from interfering with the interaction data of USART2 and NB module
/*------------------------------------------------------------------------------------------*/
//#define DEBUG_PRINT_USART1_RX   			// USART1_RX echo enable			  	    
//#define DEBUG_PRINT_USART1RX_TO_USART2TX 	// USART1_RX unvanished transmission to USART2_TX enable  
#define DEBUG_PRINT_USART2RX_TO_USART1TX	// USART1 printout USART2_RX data
/*------------------------------------------------------------------------------------------*/

/* USART1 TX/RX port: */
// DEBUG_TX_PORT
#define DEBUG_TX_PORT       GPIOA

#define DEBUG_TX_PIN        GPIO_Pin_9

#define DEBUG_RX_PORT       GPIOA

#define DEBUG_RX_PIN        GPIO_Pin_10

#define DEBUG_USART_PORT    USART1

/* USART2 TX/RX port: */
#define NBIOT_TX_PORT       GPIOA
#define NBIOT_TX_PIN        GPIO_Pin_2

#define NBIOT_RX_PORT       GPIOA
#define NBIOT_RX_PIN        GPIO_Pin_3

#define NBIOT_PORT          USART2

typedef void (*USART_RxDat_CallBack_t)(unsigned char dat);

void Hal_USART_Init(void);
void Hal_USART_Pro(void);
void Hal_USART_DebugDataQueue(unsigned char *buf, unsigned int len);
void Hal_USART2_Send_String(const unsigned char *buf);
void Hal_USART2_Send_Data(unsigned char *buf, unsigned int len);
void Hal_USART2_RxDatCBSRegister(USART_RxDat_CallBack_t pCBF);

#endif
