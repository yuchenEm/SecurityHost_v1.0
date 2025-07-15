/************************************************************************
* Module: Hal_USART
* Functionality: Implements USART1 and USART2 communication with host computer for reception, transmission, debugging, and serial data transparent transmission
*                @ Configures USART1 and USART2 GPIO pins, USART parameters, NVIC priority
*                @ USART1 polling to receive host computer debug data and echo (using queue buffer)
*                @ USART2 sends a single byte to NBIOT
*                @ USART2 sends multiple bytes of data to NBIOT
*                @ USART2 sends string data to NBIOT
* Notes:
*       @ Enable/disable USART debug functionality --> Comment/uncomment corresponding macros in Hal_USART.h
*************************************************************************/

#include "stm32f10x.h"
#include "hal_usart.h"
#include "os_system.h"

static void Hal_USART_Config(void);
static void Hal_USART_DebugPro(void);
static void Hal_Usart2_SendByte(unsigned char dat);

volatile unsigned char DebugIsBusy; // 1-USART1 is busy，0->idle

volatile Queue256 DebugTxMsg; 		

USART_RxDat_CallBack_t	USART2_RxDatCBF;  

/*----------------------------------------------------------------------------
@Name		: Hal_USART_Init()
@Function	: USART Initialization
		--> Configures GPIO pins, USART parameters, NVIC
		--> Clears send queue DebugTxMsg
		--> Sets UART send status flag DebugIsBusy to idle
@Parameter	: Null
------------------------------------------------------------------------------*/
void Hal_USART_Init(void)
{
	Hal_USART_Config();
	
	QueueEmpty(DebugTxMsg);
	
	DebugIsBusy = 0; 		
	USART2_RxDatCBF =0;		
}

/*----------------------------------------------------------------------------
@Name		: Hal_USART_Pro()
@Function	: UART debug polling function
@Parameter	: Null
------------------------------------------------------------------------------*/
void Hal_USART_Pro(void)
{
	Hal_USART_DebugPro();
}

/*----------------------------------------------------------------------------
@Name		: Hal_USART_DebugDataQueue(buf, len)
@Function	: Enqueues received debug data into the send queue DebugTxMsg
@Parameter	: 
		buf: Pointer to data buffer
		len: Data length to send
------------------------------------------------------------------------------*/
void Hal_USART_DebugDataQueue(unsigned char *buf, unsigned int len) 
{
    QueueDataIn(DebugTxMsg, &buf[0], len);
}

/*------------------------------------------------------------------------------
@Name			: Hal_USART2_RxDatCBSRegister(Usart_RxDat_CallBack_t pCBS)
@Function		: This function registers the callback function to ensure user-defined operations can be executed when data is received
@Parameter		: 
		--> pCBS  Pointer to user-defined callback function
-------------------------------------------------------------------------------*/
void Hal_USART2_RxDatCBSRegister(USART_RxDat_CallBack_t pCBF)
{
	if(USART2_RxDatCBF == 0)		
    {
        USART2_RxDatCBF = pCBF; 		
	}
}   


/*----------------------------------------------------------------------------
@Name		: Hal_USART_DebugPro()
@Function	: UART debug data processing function, as the interface of the polling function
@Parameter	: Null
------------------------------------------------------------------------------*/
static void Hal_USART_DebugPro(void)
{
    unsigned char dat;
        
    if(!DebugIsBusy)	
    {
        if(QueueDataLen(DebugTxMsg))
        {
            QueueDataOut(DebugTxMsg,&dat);
            
            DebugIsBusy = 1; 			
            
            USART_SendData(USART1,dat); 

            USART_ITConfig(DEBUG_USART_PORT, USART_IT_TXE, ENABLE); 
        }
    } 
}

/*----------------------------------------------------------------------------
@Name		: Hal_Usart2_SendByte(dat)
@Function	: USART2 sends a single byte
@Parameter	: 
		--> dat: Data byte to send
------------------------------------------------------------------------------*/
static void Hal_Usart2_SendByte(unsigned char dat)
{
    while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);

    USART_SendData(USART2, dat);
    
}

/*----------------------------------------------------------------------------
@Name		: Hal_Uart2_Send_Data(*buf, len)
@Function	: USART2 sends multiple bytes of data
@Parameter	: 
		--> buf: Pointer to data to be sent
		--> len: Data length to send
------------------------------------------------------------------------------*/
void Hal_USART2_Send_Data(unsigned char *buf, unsigned int len) 
{
  
	#ifndef DEBUG_PRINT_USART1RX_TO_USART2TX
    unsigned int t; 
        
    for(t=0; t < len; t++)
    {
        Hal_Usart2_SendByte(buf[t]);
    } 
	#endif
}

/*****************************************************************************************************************
* @brief  This function is used to send string data through USART2.
* @param  buf: Starting address of the string to be sent, pointer type unsigned char.
* @retval No return value.
*****************************************************************************************************************/
void Hal_USART2_Send_String(const unsigned char *buf) 
{
	#ifndef DEBUG_PRINT_USART1RX_TO_USART2TX
    unsigned char i; 
    
    while(*buf) 
    {
        Hal_Usart2_SendByte(*buf);
        
        i = *buf;
        buf++;
    }
	#endif
}

/*----------------------------------------------------------------------------
@Name		: Hal_USART_Config()
@Function	: Configures GPIO pins, USART, and interrupt settings
@Parameter	: Null
------------------------------------------------------------------------------*/
static void Hal_USART_Config(void) 
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
	
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	
	// USART1 GPIO configuration
	// Configure USART1 TX pin
	GPIO_InitStructure.GPIO_Pin = DEBUG_TX_PIN; 			// Set the pin to be initialized
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 		// Set to alternate function push-pull output, for TX
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 		// Set pin speed to 50MHz
	GPIO_Init(DEBUG_TX_PORT, &GPIO_InitStructure);
    
    // Configure USART1 RX pin
    GPIO_InitStructure.GPIO_Pin = DEBUG_RX_PIN; 			// Set the pin to be initialized
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 	// Set to floating input, for RX   
	GPIO_Init(DEBUG_RX_PORT, &GPIO_InitStructure);
	
	// USART2 GPIO configuration
    // Configure USART2 TX pin 
    GPIO_InitStructure.GPIO_Pin = NBIOT_TX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 		// Push-pull output
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(NBIOT_TX_PORT, &GPIO_InitStructure);

    // Configure USART2 RX pin
    GPIO_InitStructure.GPIO_Pin = NBIOT_RX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 	// Floating input   
	GPIO_Init(NBIOT_RX_PORT, &GPIO_InitStructure);
	
	
    USART_InitStructure.USART_BaudRate = 9600; 							// Baud rate (Baud Rate) is 9600    
    USART_InitStructure.USART_WordLength = USART_WordLength_8b; 		// Data bits (Word Length) is 8 bits      
    USART_InitStructure.USART_StopBits = USART_StopBits_1; 				// Stop bits (Stop Bits) is 1 bit      
    USART_InitStructure.USART_Parity = USART_Parity_No; 				// Parity bit (Parity) is no parity 
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // Hardware flow control (Hardware Flow Control) is none  
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; 	// Operation mode, enable receive (Rx) and transmit (Tx)    
    
	USART_Init(DEBUG_USART_PORT, &USART_InitStructure);   				// Initialize USART1     
    USART_Init(NBIOT_PORT, &USART_InitStructure);    					// Initialize USART2
	
	USART_ITConfig(DEBUG_USART_PORT, USART_IT_RXNE, ENABLE); 	// Enable USART1 receive interrupt (RXNE), when USART receives new data, it will trigger an interrupt      
	// !!! Disable USART1 transmit interrupt (TXE): TXE is set to 1 after reset, and can only be cleared by writing to the TDR register
	USART_ITConfig(DEBUG_USART_PORT, USART_IT_TXE, DISABLE); 	
	USART_Cmd(DEBUG_USART_PORT, ENABLE); 						// Enable USART1 port, i.e., enable USART1 functionality, allowing data transmission and reception
	
	
    USART_ITConfig(NBIOT_PORT, USART_IT_RXNE, ENABLE);    		// Enable USART2 receive interrupt (RXNE). When USART receives new data, it will trigger an interrupt.
    // !!! Disable USART2 transmit interrupt (TXE): TXE is set to 1 after reset, and can only be cleared by writing to the TDR register
    USART_ITConfig(NBIOT_PORT, USART_IT_TXE, DISABLE); 
    USART_Cmd(NBIOT_PORT, ENABLE);  							// Enable USART2 port, i.e., enable USART2 functionality, allowing data transmission and reception.
	
	/* Set USART1 interrupt priority */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0); 			// Set NVIC priority grouping
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn; 			// Set interrupt channel to USART1 interrupt
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; 	// Set preemption priority to 0, i.e., highest priority
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; 			// Set sub-priority to 0, i.e., first to respond among same-priority preemption interrupts
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 			// Enable this interrupt channel
    NVIC_Init(&NVIC_InitStructure); 
	
	/* Set USART2 interrupt priority */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;			// Set interrupt channel to USART2 interrupt
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			// Set sub-priority to 1
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);   							// Enable this interrupt channel
}

/*----------------------------------------------------------------------------
@Name		: USART1_IRQHandler()
@Function	: USART1 interrupt handler
@Parameter	: Null
------------------------------------------------------------------------------*/
void USART1_IRQHandler(void)
{
    unsigned char dat; // Variable to temporarily store data received from USART1

    // USART1 serial port reception: Check USART1 receive interrupt flag (RXNE)
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) 
    {
        dat = USART_ReceiveData(USART1);
        USART_ClearITPendingBit(USART1, USART_IT_RXNE); // Clear USART1 receive interrupt flag
    
		// If DEBUG_PRINT_USART1_RX macro is defined, send the received data through USART1 TX pin
        #ifdef DEBUG_PRINT_USART1_RX
			Hal_USART_DebugDataQueue(&dat, 1);
        #endif     
        // If DEBUG_PRINT_USART1RX_TO_USART2TX macro is defined, send the received data through USART2
        #ifdef DEBUG_PRINT_USART1RX_TO_USART2TX 
			USART_SendData(USART2, dat);
        #endif 
		
	}

    // USART1 serial port transmission: Check USART1 transmit interrupt flag (TXE)
    if (USART_GetITStatus(USART1, USART_IT_TXE) != RESET) // USART1 transmit buffer is empty
    {   
        if(QueueDataLen(DebugTxMsg)) 
        {
            QueueDataOut(DebugTxMsg,&dat); 
			
            USART_SendData(USART1,dat); 	// Write data to TDR, hardware clears TXE flag
        }
        else
        {
            DebugIsBusy = 0; // USART1 idle
            	
            USART_ITConfig(DEBUG_USART_PORT, USART_IT_TXE, DISABLE); // Queue is empty, disable USART1 transmit interrupt (TXE)
        }
		
		// TXE pending bit is cleared only by a write to the USART_DR register (USART_SendData()).
		// USART_ClearITPendingBit(USART1, USART_IT_TXE); // If transmit buffer is empty, clear transmit interrupt flag
    }
}

/*----------------------------------------------------------------------------
@Name		: USART2_IRQHandler()
@Function	: USART2 interrupt handler
@Parameter	: Null
------------------------------------------------------------------------------*/
void USART2_IRQHandler(void)
{
    unsigned char dat; // Define a variable to temporarily store received data
    
	// USART2 serial port reception: Check USART2 receive interrupt flag (RXNE)
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {   
        dat = USART_ReceiveData(USART2);  
        
		if(USART2_RxDatCBF)			// Check if a callback function has been registered
        {
            USART2_RxDatCBF(dat);   // Call the function, passing dat data to the application layer
        } 
		
		#ifdef DEBUG_PRINT_USART2RX_TO_USART1TX
             //Hal_DebugDataQueue(&dat,1);
        #endif 
		
        USART_ClearITPendingBit(USART2, USART_IT_RXNE); // Clear receive interrupt flag, prepare for next interrupt
    }
	
    // USART2 serial port transmission: Check USART2 receive interrupt flag (TXE)
    if(USART_GetITStatus(USART2, USART_IT_TXE) != RESET)
    {
		
		
//        // If transmit buffer is empty (transmit interrupt flag), clear transmit interrupt flag
//        USART_ClearITPendingBit(USART2, USART_IT_TXE);
    }
}
