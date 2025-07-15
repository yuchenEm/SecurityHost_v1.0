/*************************************************************************************************************
* Module: Hal_RFD
* Functionality: Implements RF wireless data reception and decoding:
*       @ Configures GPIO for the RFD module
*       @ Creates an RFD sampling timer with a TimeBase of 50us for OOK signal sampling
*       @ Creates a repeat code filtering timer with a TimeBase of 1s to filter out duplicate signals
*       @ Polls and decodes received data pulse width to obtain a 2-byte address code and 1-byte data code
*       @ Transfers decoded data to the application layer via a callback function
* Notes:
*       @ To adjust the allowable error range for sync code pulse width: 
*		  modify RFD_TITLE_CLK_MINL and RFD_TITLE_CLK_MAXL in Hal_RFD.h
*       @ To adjust the allowable error range for data code pulse width: 
*		  modify RFD_DATA_CLK_MINL and RFD_DATA_CLK_MAXL in Hal_RFD.h
**************************************************************************************************************/

#include <string.h>
#include <stdbool.h>
#include "stm32f10x.h" 
#include "hal_rfd.h"
#include "hal_timer.h"
#include "os_system.h"

static void Hal_RFD_Config(void);
static unsigned char Hal_RFD_GetRFD_IOState(void);
static void Hal_PulseACQ_Handler(void);
static void Hal_RFD_DecodeFilter_Handler(void); 
static void Hal_RFD_CodeHandler(unsigned char *pCode);

/*-----------------------------------------------------------------------------*/
unsigned char RFD_DecodeSteps;	// RFD Decode Steps
enum {							
	RFD_DECODE_PULSEDATA, 		// ev1527 RFD decode header          
	RFD_DECODE_DATA       		// ev1527 RFD decode data      
};

volatile Queue32 RFD_RxBuffer;  // RFD data receive queue
Queue8 RFD_CodeBuffer;			// RFD message buffer

volatile unsigned char RFD_DecodeFilterTimerIdle; // receive repeat code timer flag

RFD_RxCallBack_t RFD_RxCBF;

/*----------------------------------------------------------------------------
@Name		: Hal_RFD_Init()
@Function	: RFD module initial
		--> RFD GPIO configure
		--> call-back function RFD_RxCBF point to Null
		--> clear RFD_DecodeFilterTimerIdle flag
		--> RFD_DecodeSteps set to RFD_DECODE_PULSEDATA （obtain pulse widths sets for decoding）
		--> empty RFD_RxBuffer
		--> empty RFD_CodeBuffer
@Parameter	: Null
------------------------------------------------------------------------------*/
void Hal_RFD_Init(void)
{
	Hal_RFD_Config();
	
	RFD_RxCBF = 0;
	RFD_DecodeFilterTimerIdle = 0;
	RFD_DecodeSteps = RFD_DECODE_PULSEDATA;
	
	QueueEmpty(RFD_RxBuffer);
	QueueEmpty(RFD_CodeBuffer);
	
	Hal_Timer_CreatTimer(T_RFD_PULSE_RX, Hal_PulseACQ_Handler, 1, T_STATE_START);				// TimeBase: 50us, Period: 50us
	Hal_Timer_CreatTimer(T_RFD_RECODE_FLT, Hal_RFD_DecodeFilter_Handler, 20000, T_STATE_STOP);	// TimeBase: 50us, Period: 1s
}

/*----------------------------------------------------------------------------
@Name		: Hal_RFD_Pro()
@Function	: RFD polling function （receive and decode）
		--> get data(byte) from RFD_RxBuffer
		--> calculate the corresponding pulse width, and save to the width queue ClkTimeBuff
		--> decode the pulse widths data, capture the syn-header
		--> if syn-header detected, start decoding RF data, according pulse widths to decode <Bit '1'> and <Bit '0'>
		--> after receiving 24 Bit valid data, combine the data to Code[3], prepare the dataframe
		--> using call-back function to deliver data to application layer

			<syn-header> ：
			<Bit '1'> ：
			<Bit '0'> ：
			<Dataframe> ：
@Parameter	: Null
------------------------------------------------------------------------------*/
void Hal_RFD_Pro(void)
{
	Queue256 PulseTimeBuff;	// pulse width queue
							// dataformat： {1000 0010, 0111 1111, 0001 1111, 1111 1000, ...}
							//  		use 2byte to represent a set of data（1000 0010, 0111 1111） high-byte, low-byte
							// 			bit[7] of high-byte represent high/low volateg： 1-->high, 0-->low
							//			bit[0-6] of high-byte and bit[0-7] of low-byte represent the counts of pulse(0-32767) (0-1638.35ms)
	
	static unsigned short Time1 = 0; 		// high time
	static unsigned short Time2 = 0; 		// low time
	static unsigned char ReadDataFlag = 0;	// 1：syn-header captured, 0：syn-header not captured
	static unsigned char Len; 
	static unsigned char Code[3]; 			// save Hex data（2 byte address， 1 byte data）
	static unsigned char CodeTempBuff[3];
	
	/*-------------------------------------------------------*/	
	switch(RFD_DecodeSteps)		
	{
		case RFD_DECODE_PULSEDATA:	
		{
			unsigned char Temp; 
			unsigned char Num; 
			static unsigned char DataState = 0; 
			static unsigned short Count = 0;	// high/low voltage pulse counter （count * 50us = pulse width）
			
			QueueEmpty(PulseTimeBuff);
			
			while(QueueDataOut(RFD_RxBuffer, &Temp))
			{
				Num = 8; 
				
				while(Num--)
				{
					if(DataState) 
					{
						if(!(Temp & 0x80)) 
						{
							unsigned char Data; 	
							
							Data = Count / 256;		
							Data |= 0x80;			
							QueueDataIn(PulseTimeBuff, &Data, 1);
							Data = Count % 256;		
							QueueDataIn(PulseTimeBuff, &Data, 1);
							
							DataState = 0; 
							Count = 0; 	   
						}
					}
					
					else 		
					{
						if(Temp & 0x80)	
						{
							unsigned char Data;		
							
							Data = Count / 256;		
							Data &= 0x7F;			
							QueueDataIn(PulseTimeBuff, &Data, 1);
							Data = Count % 256;		
							QueueDataIn(PulseTimeBuff, &Data, 1);	
							
							DataState = 1; 
							Count = 0;	   
						}
					}
					Count++;
					Temp <<= 1;
				}
			}
		}
		
		case RFD_DECODE_DATA:  
		{
			while(QueueDataLen(PulseTimeBuff)) 
			{
				if(!ReadDataFlag) // waiting syn-header
				{
					unsigned char Temp;
					
					while(!Time1 || !Time2)
					{
						if(!Time1)	
						{
							while(QueueDataOut(PulseTimeBuff, &Temp)) 
							{
								if(Temp & 0x80)			
								{
									Temp &= 0xFF7F; 	// obtain high-byte bit[0-6]
									Time1 = Temp * 256; 
									
									QueueDataOut(PulseTimeBuff, &Temp); 
									Time1 += Temp;
									Time2 = 0;
									break;
								}
								else
								{
									QueueDataOut(PulseTimeBuff, &Temp);
								}									
							}
							
							if(!QueueDataLen(PulseTimeBuff))
							{
								break; 
							}									
						}
						
						if(!Time2) 
						{
							QueueDataOut(PulseTimeBuff, &Temp); 
							Time2 = Temp * 256;
							
							QueueDataOut(PulseTimeBuff, &Temp); 
							Time2 += Temp;
							
							// Design tolerence: RFD_TITLE_CLK_MINL < Ratio < RFD_TITLE_CLK_MAXL 
							// compare the high voltage/low voltage time ratio(1/31)
							if((Time2 >= (Time1 * RFD_TITLE_CLK_MINL)) && (Time2 <= (Time1 * RFD_TITLE_CLK_MAXL)))
							{
								Time1 = 0;
								Time2 = 0;
								Len = 0;
								
								ReadDataFlag = 1; 
								break;
							}		
							else
							{
								Time1 = 0;
								Time2 = 0;
							}
						}
					}
				}
				
				if(ReadDataFlag) // syn-header detected, start receiving data
				{
					unsigned char Temp;
								
					if(!Time1) 
					{
						if(QueueDataOut(PulseTimeBuff, &Temp)) 
						{
							Temp &= 0xFF7F;
							Time1 = Temp * 256;
							
							QueueDataOut(PulseTimeBuff, &Temp);
							Time1 += Temp;
							Time2 = 0;
						}
						else
						{
							break;
						}
					}
					
					
					if(!Time2) 
					{
						if(QueueDataOut(PulseTimeBuff, &Temp))
						{
							bool RecvSuccFlag; 
							
							Time2 = Temp * 256;
							QueueDataOut(PulseTimeBuff, &Temp);
							Time2 += Temp;
							
							// <Bit '1'>
							// Design torelence: RFD_DATA_CLK_MINL < Ratio < RFD_DATA_CLK_MAXL 
							// compare high voltage/low voltage time ratio(3/1)						
							if((Time1 > (Time2 * RFD_DATA_CLK_MINL)) && (Time1 <= (Time2 * RFD_DATA_CLK_MAXL)))
							{
								unsigned char i;
								unsigned char c = 0x80; 
								
								for(i = 0; i < Len%8; i++) 
								{
									c >>= 1;
									c &= 0x7F;
								}
								Code[Len/8] |= c; 
								RecvSuccFlag = 1; 
							}
							
							// <Bit '0'>
							// Design torelence: RFD_DATA_CLK_MINL < Ratio < RFD_DATA_CLK_MAXL 
							// compare high voltage/low voltage time ratio(1/3)	：	
							else if((Time2 > (Time1*RFD_DATA_CLK_MINL)) && (Time2 <= (Time1*RFD_DATA_CLK_MAXL)))
							{
								unsigned char i;
								unsigned char c = (unsigned char)0xFF7F; // 0x7F(0111 1111)
								for(i = 0; i < Len%8; i++)
								{
									c >>= 1;
									c |= 0x0080;
								}
								Code[Len/8] &= c;
								RecvSuccFlag = 1;
							}
							else //error
							{
								RecvSuccFlag = 0;
								ReadDataFlag = 0;
							}
							
							Time1 = 0;
							Time2 = 0;
							
							if((++Len ==24)  && RecvSuccFlag) // Len=24: 16bit address + 8bit data； RecvSuccFlag = 1 --> a set of Hex data(3byte) collected
							{
								ReadDataFlag = 0; // waiting for the next syn-header
								
								if((CodeTempBuff[0]==Code[0])&&(CodeTempBuff[1]==Code[1])&&(CodeTempBuff[2]==Code[2])) 
									{
										Hal_RFD_CodeHandler(Code); 
									}
									else
									{
										memcpy(CodeTempBuff, Code, 3); 
									}
							}
						}
						
						else
						{
							break; 
						}
						
					}
				}
			}
		}
	}
}

/*----------------------------------------------------------------------------
@Name		: Hal_RFD_CodeHandler(pCode)
@Function	: process the data to send
@Parameter	: 
		pCode: coming-in pointer of the Hex data
------------------------------------------------------------------------------*/
static void Hal_RFD_CodeHandler(unsigned char *pCode)
{
	static unsigned char tBuff[3];
	unsigned char temp;
 
	if((Hal_Timer_GetState(T_RFD_RECODE_FLT)==T_STATE_START) && (!RFD_DecodeFilterTimerIdle))
	{
		return;
	}

	Hal_Timer_ResetTimer(T_RFD_RECODE_FLT,T_STATE_START); 
	memcpy(tBuff, pCode, 3);
	RFD_DecodeFilterTimerIdle = 0;	
	
	temp = '#';
	QueueDataIn(RFD_CodeBuffer, &temp, 1);
	QueueDataIn(RFD_CodeBuffer, &tBuff[0], 3);	// Hex dataframe format("# 0x00 0x00 0x00")
	
	if(RFD_RxCBF) 		  
	{
		RFD_RxCBF(tBuff); 
	}
}

/*----------------------------------------------------------------------------
@Name		: Hal_RFD_RxCBF_Register(pCBF)
@Function	: RFD module call-back function register
@Parameter	: 
		pCBF: point to the call-back function defined by user 
------------------------------------------------------------------------------*/
void Hal_RFD_RxCBF_Register(RFD_RxCallBack_t pCBF)
{
	if(RFD_RxCBF == 0)
	{
		RFD_RxCBF = pCBF;
	}
}	

/*----------------------------------------------------------------------------
@Name		: Hal_PulseACQ_Handler
@Function	: RFD pulse acquisition handler， TimeBase = 50us RFD_PULSE_RX timer IRQ handler；
@Parameter	: Null
------------------------------------------------------------------------------*/
static void Hal_PulseACQ_Handler(void)
{
	static unsigned char Temp;
	static unsigned char Count = 0;
	Temp <<= 1;
	if(Hal_RFD_GetRFD_IOState()) 
		Temp |= 0x01;  
	else 
		Temp &= 0xFE;
	if(++Count == 8)
	{
		Count = 0;
		QueueDataIn(RFD_RxBuffer, &Temp, 1);
	}
	Hal_Timer_ResetTimer(T_RFD_PULSE_RX, T_STATE_START);
}

/*----------------------------------------------------------------------------
@Name		: Hal_RFD_GetRFD_IOState()
@Function	: Read the RFD IO state
@Parameter	: Null
------------------------------------------------------------------------------*/
static unsigned char Hal_RFD_GetRFD_IOState(void)
{
	return (GPIO_ReadInputDataBit(RFD_RX_PORT, RFD_RX_PIN));	
}

/*----------------------------------------------------------------------------
@Name		: Hal_RFD_DecodeFilter_Handler()
@Function	: skip the repetive code received in 1s
			  TimeBase = 1s RFD_RECODE_FLT timer IRQ handler；
				@ RFD_DecodeFilterTimerIdle = 1 --> accept repeat code
			    @ RFD_DecodeFilterTimerIdle = 0 --> not accept repeat code
@Parameter	: Null
------------------------------------------------------------------------------*/
static void Hal_RFD_DecodeFilter_Handler(void)
{
	RFD_DecodeFilterTimerIdle = 1;
}

/*----------------------------------------------------------------------------
@Name		: Hal_RFD_Config()
@Function	: config RFD GPIO
@Parameter	: Null
------------------------------------------------------------------------------*/
static void Hal_RFD_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	 
	GPIO_InitStructure.GPIO_Pin = RFD_RX_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_Init(RFD_RX_PORT, &GPIO_InitStructure);	
}
