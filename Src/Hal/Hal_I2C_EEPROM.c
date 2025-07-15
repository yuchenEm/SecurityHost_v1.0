/************************************************************************
* Module: Hal_I2C_EEPROM
* Function: Implement software-simulated I2C timing:
        EEPROM read/write one byte; cross-page read/write multiple bytes
*       @ Configure I2C pins
*       @ Use I2C protocol to communicate with EEPROM (AT24C128)
*       @ EEPROM read/write one byte
*       @ EEPROM cross-page read/write (64KB)
* Description:
*       @ To use other types of EEPROM:
*           --> Modify EEPROM_PAGE_SIZE
*           --> Modify Device address
*************************************************************************/

#include "stm32f10x.h" 
#include "hal_i2c_eeprom.h"

#define EEPROM_PAGE_SIZE 64	

static void Hal_I2C_Config(void);
static void Hal_I2C_Delay(unsigned short t);
static void Hal_I2C_SDA_Write(unsigned char BitValue);
static void Hal_I2C_SCL_Write(unsigned char BitValue);
static unsigned char Hal_I2C_SDA_Read(void);
static void Hal_I2C_Start(void);
static void Hal_I2C_Stop(void);
static void Hal_I2C_SendByte(unsigned char Byte);
static unsigned char Hal_I2C_ReceiveByte(void);
static void Hal_I2C_SendACK(unsigned char ACKbit);
static unsigned char Hal_I2C_RecACK(void);

/*----------------------------------------------------------------------------
@Name		: Hal_I2C_EEPROM_Init()
@Function	: I2C_EEPROM module initialize
@Parameter	: Null
------------------------------------------------------------------------------*/
void Hal_I2C_EEPROM_Init(void)
{
	Hal_I2C_Config();
}

/*----------------------------------------------------------------------------
@Name		: Hal_I2C_Config()
@Function	: I2C GPIO config
@Parameter	: Null
------------------------------------------------------------------------------*/
static void Hal_I2C_Config(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure; 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin =  I2C_SCL_PIN | I2C_SDA_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;  
	GPIO_Init(I2C_SCL_PORT, &GPIO_InitStructure);
  
	GPIO_SetBits(I2C_SDA_PORT, I2C_SDA_PIN);
	GPIO_SetBits(I2C_SCL_PORT, I2C_SCL_PIN);
	
}


/*----------------------------- I2C Driver functions ----------------------------------*/
/*----------------------------------------------------------------------------
@Name		: Hal_I2C_SDA_Write(BitValue)
@Function	: I2C SDA（1：high，0：low）

@Name		: Hal_I2C_SCL_Write(BitValue)
@Function	: I2C SCL（1：high，0：low）

@Name		: Hal_I2C_SDA_Read()
@Function	: I2C SDA readout（Return Value： 1：high，0：low）

@Name		: Hal_I2C_Start()
@Function	: I2C start signal

@Name		: Hal_I2C_Stop()
@Function	: I2C stop signal

@Name		: Hal_I2C_SendByte(Byte)
@Function	: I2C send 1byte

@Name		: Hal_I2C_ReceiveByte()
@Function	: I2C receive 1byte

@Name		: Hal_I2C_SendACK(ACKbit)
@Function	: I2C send ACK to slave （1：NACK，0：ACK）

@Name		: Hal_I2C_RecACK()
@Function	: I2C waiting ACK from slave （Return Value： 1：NACK，0：ACK）

@Name		: Hal_I2C_Delay(t)
@Function	: I2C delay
------------------------------------------------------------------------------*/
static void Hal_I2C_SDA_Write(unsigned char BitValue)
{
	GPIO_WriteBit(I2C_SDA_PORT, I2C_SDA_PIN, (BitAction)BitValue);
	Hal_I2C_Delay(1);
}

static void Hal_I2C_SCL_Write(unsigned char BitValue)
{
	GPIO_WriteBit(I2C_SCL_PORT, I2C_SCL_PIN, (BitAction)BitValue);
	Hal_I2C_Delay(1);
}

static unsigned char Hal_I2C_SDA_Read(void)
{
	unsigned char BitValue;
	
	BitValue = GPIO_ReadInputDataBit(I2C_SDA_PORT, I2C_SDA_PIN);
	Hal_I2C_Delay(1);
	return BitValue;
}

static void Hal_I2C_Start(void)
{
	Hal_I2C_SDA_Write(1);
	Hal_I2C_SCL_Write(1);
	Hal_I2C_SDA_Write(0);
	Hal_I2C_SCL_Write(0);
}

static void Hal_I2C_Stop(void)
{
	Hal_I2C_SDA_Write(0);
	Hal_I2C_SCL_Write(1);
	Hal_I2C_SDA_Write(1);
}

static void Hal_I2C_SendByte(unsigned char Byte)
{
	unsigned char i;
	
	for(i=0; i<8; i++)
	{
		Hal_I2C_SDA_Write(Byte & (0x80>>i));
		Hal_I2C_SCL_Write(1);
		Hal_I2C_SCL_Write(0);
	}
	
	Hal_I2C_SCL_Write(0);
	Hal_I2C_SDA_Write(1);
}

static unsigned char Hal_I2C_ReceiveByte(void)
{
	unsigned char Byte = 0x00;
	unsigned char i;
	
	Hal_I2C_SDA_Write(1);
	
	for(i=0; i<8; i++)
	{
		Hal_I2C_SCL_Write(1);
		if(Hal_I2C_SDA_Read() == 1)
		{
			Byte |= (0x80>>i);
		}
		Hal_I2C_SCL_Write(0);
	}
	
	Hal_I2C_SCL_Write(0);
	Hal_I2C_SDA_Write(1);
	
	return Byte;
}

static void Hal_I2C_SendACK(unsigned char ACKbit)
{
	Hal_I2C_SDA_Write(ACKbit);
	Hal_I2C_SCL_Write(1);
	Hal_I2C_SCL_Write(0);
}

static unsigned char Hal_I2C_RecACK(void)
{
	unsigned char ACKbit;
	
	Hal_I2C_SDA_Write(1);
	Hal_I2C_SCL_Write(1);			
	ACKbit = Hal_I2C_SDA_Read();
	Hal_I2C_SCL_Write(0);
	
	return ACKbit;
}

static void Hal_I2C_Delay(unsigned short t)
{
	unsigned short i = 50;
	unsigned short j, k;
	k = t;
	for(j=0; j<k; j++)
	{
		while(i)
		{
			i--;
		}
	}
}

/*-------------------------- EEPROM functions --------------------------------------*/
/* Device: AT24C128, Device address: (MSB->LSB) 1 0 1 0 0 A1 A0 R/W
/* A1, A0 grounded --> Device address: (MSB->LSB) 1 0 1 0 0 0 0 R/W
/* Write to EEPROM --> Device address: (R/W = 0) 1 0 1 0 0 0 0 0
/* Read from EEPROM --> Device address: (R/W = 1) 1 0 1 0 0 0 0 1

/*----------------------------------------------------------------------------
@Name		: Hal_I2C_EEPROM_ByteWrite(address, Byte)
@Function	: Write 1 byte of data to EEPROM
@Parameter	: Device address: (R/W = 0) 1 0 1 0 0 0 0 0 (0xA0)
		--> address	: EEPROM address where the data will be written
		--> Byte	: The byte of data to be written
@Description:
	!!! After writing data, sufficient time must be given for the EEPROM to complete the programming process. 
		During programming, the written address data cannot be accessed normally (will display as 0xFF)
------------------------------------------------------------------------------*/
void Hal_I2C_EEPROM_ByteWrite(unsigned short address, unsigned char Data)
{
	Hal_I2C_Start();
	
	Hal_I2C_SendByte(0xA0);
	Hal_I2C_RecACK();
	
	Hal_I2C_SendByte((address >> 8) & 0xFF); 
	Hal_I2C_RecACK();
	
	Hal_I2C_SendByte(address & 0xFF); 		
	Hal_I2C_RecACK();
	
	Hal_I2C_SendByte(Data);
	Hal_I2C_RecACK();
	
	Hal_I2C_Stop();
	
	Hal_I2C_Delay(20000);		
}

/*----------------------------------------------------------------------------
@Name		: Hal_I2C_EEPROM_RandomRead(address)
@Function	: Read 1 byte of data from any address in the EEPROM
                @Random mode: A random read requires a "dummy" byte write sequence
                    to load in the data word address.
@Parameter	: Device address: (R/W = 1) 1 0 1 0 0 0 0 1 (0xA1)
        --> address: The address from which to read the data
------------------------------------------------------------------------------*/
unsigned char Hal_I2C_EEPROM_RandomRead(unsigned short address)
{
	unsigned char RxByte;
	
	Hal_I2C_Start();
	
	Hal_I2C_SendByte(0xA0);
	Hal_I2C_RecACK();
	
	Hal_I2C_SendByte((address >> 8) & 0xFF); 
	Hal_I2C_RecACK();
	
	Hal_I2C_SendByte(address & 0xFF); 		
	Hal_I2C_RecACK();
	
	Hal_I2C_Start();
	
	Hal_I2C_SendByte(0xA1);
	Hal_I2C_RecACK();
	
	RxByte = Hal_I2C_ReceiveByte();
	Hal_I2C_SendACK(1);			
	
	Hal_I2C_Stop();
	
	return RxByte;
}

/*----------------------------------------------------------------------------
@Name		: Hal_I2C_EEPROM_PageWrite(address, pDat, Num)
@Function	: Write data page by page to the specified address in EEPROM (automatic page turning)
@Parameter	: Device address: (R/W = 0) 1 0 1 0 0 0 0 0 (0xA0)
        --> address     : The address where data will be written
        --> pDat        : Pointer to the continuous data to be written
        --> Num         : Number of bytes to write (1-65536(64KB))
@Description:
        EEPROM page write function. AT24C128 supports writing up to 64 bytes per page. 
        Writing more than 64 bytes will overwrite existing data.
        Automatic page turning: Determine whether to turn page based on the starting address and number of bytes to write.

    *** Note: short type for Num allows writing up to 64KB of data at once. For larger amounts of data, use int type ***
------------------------------------------------------------------------------*/
void Hal_I2C_EEPROM_PageWrite(unsigned short address, unsigned char *pDat, unsigned short Num)
{
	unsigned short i, j;
	unsigned short temp = 0;
	unsigned short RemainedBytes; 
	unsigned short Page;
	unsigned char* pBuffer;
	pBuffer = pDat;
	
	if(address % EEPROM_PAGE_SIZE)
	{
		temp = EEPROM_PAGE_SIZE - (address % EEPROM_PAGE_SIZE); 
		if(temp >= Num)
		{
			temp = Num;			
		}
	}	
	
	if(temp) 		
	{
		Hal_I2C_Start();
		
		Hal_I2C_SendByte(0xA0);
		Hal_I2C_RecACK();
		
		Hal_I2C_SendByte((address >> 8) & 0xFF); 
		Hal_I2C_RecACK();
		
		Hal_I2C_SendByte(address & 0xFF); 		
		Hal_I2C_RecACK();
		
		for(i=0; i<temp; i++)
		{
			Hal_I2C_SendByte(pBuffer[i]);
			Hal_I2C_RecACK();
			
		}
		
		Hal_I2C_Stop();
		
		Hal_I2C_Delay(20000);				
	}
	
	Num -= temp; 					
	address += temp;				
	
	Page = Num / EEPROM_PAGE_SIZE;			
	RemainedBytes = Num % EEPROM_PAGE_SIZE;	
	
	for(i=0; i<Page; i++)
	{
		Hal_I2C_Start();
		
		Hal_I2C_SendByte(0xA0);
		Hal_I2C_RecACK();
		
		Hal_I2C_SendByte((address >> 8) & 0xFF); 
		Hal_I2C_RecACK();
		
		Hal_I2C_SendByte(address & 0xFF); 		
		Hal_I2C_RecACK();
		
		for(j=0; j<EEPROM_PAGE_SIZE; j++)
		{
			Hal_I2C_SendByte(pBuffer[temp + j]);
			Hal_I2C_RecACK();
			
		}
		
		Hal_I2C_Stop();
		
		Hal_I2C_Delay(20000);				
		
		address += EEPROM_PAGE_SIZE;
		temp += EEPROM_PAGE_SIZE;
	}
	
	if(RemainedBytes)
	{
		Hal_I2C_Start();
		
		Hal_I2C_SendByte(0xA0);
		Hal_I2C_RecACK();
		
		Hal_I2C_SendByte((address >> 8) & 0xFF); 
		Hal_I2C_RecACK();
		
		Hal_I2C_SendByte(address & 0xFF); 		
		Hal_I2C_RecACK();
		
		for(j=0; j<RemainedBytes; j++)
		{
			Hal_I2C_SendByte(pBuffer[temp + j]);
			Hal_I2C_RecACK();
			
		}
		
		Hal_I2C_Stop();
		
		Hal_I2C_Delay(20000);				
	}
}


/*----------------------------------------------------------------------------
@Name		: Hal_I2C_EEPROM_SequentialRead(address, pBuffer, Num)
@Function	: Read a continuous block of data starting from the specified EEPROM address
@Parameter	: Device address: (R/W = 1) 1 0 1 0 0 0 0 1 (0xA1)
        --> address     : The starting address from which to read data
        --> pBuffer     : Pointer to the buffer where the read data will be stored
        --> Num         : Number of bytes to read

    *** Note: short type for Num allows reading up to 64KB of data at once. For larger amounts of data, use int type ***
------------------------------------------------------------------------------*/
// EEPROM sequential read function. When the data address reaches the storage address limit, it rolls over until the host sends a stop signal.
void Hal_I2C_EEPROM_SequentialRead(unsigned short address, unsigned char *pBuffer, unsigned short Num)
{
	unsigned short len;
	len = Num;
	
	Hal_I2C_Start();
	
	Hal_I2C_SendByte(0xA0);
	Hal_I2C_RecACK();
	
	Hal_I2C_SendByte((address >> 8) & 0xFF); 
	Hal_I2C_RecACK();
	
	Hal_I2C_SendByte(address & 0xFF); 		
	Hal_I2C_RecACK();
	
	Hal_I2C_Start();
	
	Hal_I2C_SendByte(0xA1);
	Hal_I2C_RecACK();
	
	while(len)
	{
		*pBuffer = Hal_I2C_ReceiveByte();
		
		if(len == 1)
		{
			Hal_I2C_SendACK(1);
		}
		else
		{
			Hal_I2C_SendACK(0);
		}
		pBuffer++;
		len--;
	}
	
	Hal_I2C_Stop();
}
