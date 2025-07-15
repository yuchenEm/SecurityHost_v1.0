#include "stm32f10x.h"
#include "hal_i2c_eeprom.h"
#include "device.h"

static void Device_CreatDTC(unsigned char n);
static unsigned char Device_ParaCheck(void);

Stru_DTC	sDevice[DTC_SUM];	


/*----------------------------------------------------------------------------
@Name		: Device_Init()
@Function	: Device module initial
@Parameter	: Null
------------------------------------------------------------------------------*/
void Device_Init(void)
{
	Hal_I2C_EEPROM_SequentialRead(STRU_DEVICEPARA_OFFSET, (unsigned char*)(&sDevice), sizeof(sDevice));
	
	if(Device_ParaCheck())
	{
		Device_FactoryReset();
	}
}

/*----------------------------------------------------------------------------
@Name		: Device_FactoryReset()
@Function	: reset all device parameters
@Parameter	: Null
------------------------------------------------------------------------------*/
void Device_FactoryReset(void)
{
	unsigned char i, j;
	
	for(i=0; i<DTC_SUM; i++)
	{
		sDevice[i].ID = 0;
		sDevice[i].Mark = 0;
		sDevice[i].NameNum = 0;
		for(j=0; j<16; j++)
		{
			sDevice[i].DeviceName[j] = 0;
		}
		sDevice[i].DTCType = DTC_DOOR;
		sDevice[i].ZoneType = ZONE_TYP_1ST;
		sDevice[i].Code[0] = 0;
		sDevice[i].Code[1] = 0;
		sDevice[i].Code[2] = 0;
	}
	
	Hal_I2C_EEPROM_PageWrite(STRU_DEVICEPARA_OFFSET, (unsigned char*)(&sDevice), sizeof(sDevice));
	Hal_I2C_EEPROM_SequentialRead(STRU_DEVICEPARA_OFFSET, (unsigned char*)(&sDevice), sizeof(sDevice));
}


/*----------------------------------------------------------------------------
@Name		: Device_DeleteDTC(pDTC)
@Function	: delete the selected device
@Parameter	: 
		--> pDTC: point to the target device
------------------------------------------------------------------------------*/
void Device_DeleteDTC(Stru_DTC *pDTC)
{
	unsigned char i;
	unsigned char realID;
	
	realID = pDTC->ID - 1;
	pDTC->ID = 0;
	pDTC->Mark = 0;
	pDTC->NameNum = 0;
	for(i=0; i<16; i++)
	{
		pDTC->DeviceName[i] = 0;
	}
	pDTC->DTCType = DTC_DOOR;
	pDTC->ZoneType = ZONE_TYP_1ST;
	pDTC->Code[0] = 0;
	pDTC->Code[1] = 0;
	pDTC->Code[2] = 0;
	
	Hal_I2C_EEPROM_PageWrite(STRU_DEVICEPARA_OFFSET + realID * STRU_DTC_SIZE, (unsigned char*)(pDTC), STRU_DTC_SIZE);
	Hal_I2C_EEPROM_SequentialRead(STRU_DEVICEPARA_OFFSET + realID * STRU_DTC_SIZE, (unsigned char*)(&sDevice[realID]), STRU_DTC_SIZE);
}


/*----------------------------------------------------------------------------
@Name		: Device_GetDTCNum()
@Function	: obtain the number of paired devices
@Parameter	: Null
------------------------------------------------------------------------------*/
unsigned char Device_GetDTCNum(void)
{
	unsigned char i;
	unsigned char count = 0;
	
	for(i=0; i<DTC_SUM; i++)
	{
		if(sDevice[i].Mark)
		{
			count++;
		}
	}
	return count;
}


/*----------------------------------------------------------------------------
@Name		: Device_AddDTC(pDTC)
@Function	: add new detectors
@Parameter	: 
		--> pDTC: point to the new detector
@Note		: return the detector index, 0xFF->pair failed
------------------------------------------------------------------------------*/
unsigned char Device_AddDTC(Stru_DTC *pDTC)
{
	unsigned char i, j, Temp;
	unsigned char ID;
	unsigned char NameStrIndex = 0;
	
	Stru_DTC NewDTC;
	
	for(i=0; i<DTC_SUM; i++)
	{	
		if((sDevice[i].Mark) && (sDevice[i].Code[1] == pDTC->Code[1]) && (sDevice[i].Code[2] == pDTC->Code[2]))
		{
			ID = i;
			return ID; 
		}
	}
	
	for(i=0; i<DTC_SUM; i++)
	{
		if(!sDevice[i].Mark)
		{
			NewDTC.DeviceName[0] = 'Z';
			NewDTC.DeviceName[1] = 'o';
			NewDTC.DeviceName[2] = 'n';
			NewDTC.DeviceName[3] = 'e';
			NewDTC.DeviceName[4] = '-';
			
			NameStrIndex = 5;
			Temp = i + 1;
			
			NewDTC.DeviceName[NameStrIndex++] = '0' + (Temp / 100);
			NewDTC.DeviceName[NameStrIndex++] = '0' + ((Temp % 100) / 10);
			NewDTC.DeviceName[NameStrIndex++] = '0' + ((Temp % 100) % 10);
			
			for(j=NameStrIndex; j<16; j++)
			{
				NewDTC.DeviceName[j] = 0;
			}
			
			NewDTC.ID = i + 1;
			NewDTC.Mark = 1;
			NewDTC.NameNum = Temp;
			NewDTC.DTCType = pDTC->DTCType;
			NewDTC.ZoneType = pDTC->ZoneType;
			NewDTC.Code[0] = pDTC->Code[0];
			NewDTC.Code[1] = pDTC->Code[1];
			NewDTC.Code[2] = pDTC->Code[2];
			
			Hal_I2C_EEPROM_PageWrite(STRU_DEVICEPARA_OFFSET + i * STRU_DTC_SIZE, (unsigned char*)(&NewDTC), sizeof(NewDTC));
			Hal_I2C_EEPROM_SequentialRead(STRU_DEVICEPARA_OFFSET + i * STRU_DTC_SIZE, (unsigned char*)(&sDevice[i]), STRU_DTC_SIZE);
		
			ID = i;
			
			return ID;		
		}
	}
	return 0xFF;			
}


/*----------------------------------------------------------------------------
@Name		: Device_DTCMatching(pCode)
@Function	: RFD matching
@Parameter	: 
		--> pCode: point to the Code
@Note		: return the detector index, 0xFF->pair failed
------------------------------------------------------------------------------*/
unsigned char Device_DTCMatching(unsigned char *pCode)
{
	unsigned char i;
	
	for(i=0; i<DTC_SUM; i++)
	{
		if((sDevice[i].Mark) && (sDevice[i].Code[1] == pCode[1]) && (sDevice[i].Code[2] == pCode[2]))
		{
			return (sDevice[i].ID);
		}
	}
	return 0xFF;
}


/*----------------------------------------------------------------------------
@Name		: Device_CheckDTCExisting(Index)
@Function	: check the specific detector existing or not
@Parameter	: 
		--> Index: detector index
@Note: 0->not existing, 1->existing
------------------------------------------------------------------------------*/
unsigned char Device_CheckDTCExisting(unsigned char Index)
{
	unsigned char result = 0;
	
	if(Index < DTC_SUM)			
	{
		if(sDevice[Index].Mark)
		{
			result = 1;
		}
	}
	return result;
}

/*----------------------------------------------------------------------------
@Name		: Device_GetDTCID(Index)
@Function	: obtain the specific detector ID
@Parameter	: 
		--> Index: detector index
------------------------------------------------------------------------------*/
unsigned char Device_GetDTCID(unsigned char Index)
{
	unsigned char result = 0;
	
	if(Index < DTC_SUM)
	{
		result = sDevice[Index].ID;
	}
	return result;
}


/*----------------------------------------------------------------------------
@Name		: Device_GetDTCStructure(psBuffer, index)
@Function	: obtain the specific detector structure
@Parameter	: 
		--> psBuffer	: point to the buffer
		--> index		: the detector index
------------------------------------------------------------------------------*/
void Device_GetDTCStructure(Stru_DTC *psBuffer, unsigned char index)
{
	unsigned char i;
	
	if(index >= DTC_SUM)
	{
		return;			
	}
	
	psBuffer->ID = sDevice[index].ID;
	psBuffer->Mark = sDevice[index].Mark;
	psBuffer->NameNum = sDevice[index].NameNum;
	for(i=0; i<16; i++)
	{
		psBuffer->DeviceName[i] = sDevice[index].DeviceName[i];
	}
	psBuffer->DTCType = sDevice[index].DTCType;
	psBuffer->ZoneType = sDevice[index].ZoneType;
	
	psBuffer->Code[0] = sDevice[index].Code[0];
	psBuffer->Code[1] = sDevice[index].Code[1];
	psBuffer->Code[2] = sDevice[index].Code[2];
}


/*----------------------------------------------------------------------------
@Name		: Device_SetDTCAttribute(index, psDevicePara)
@Function	: edit the detector attribute
@Parameter	: 
	--> index : detector index
	--> psDevicePara : point to the detectorPara structure
------------------------------------------------------------------------------*/
void Device_SetDTCAttribute(unsigned char index, Stru_DTC *psDevicePara)
{
	unsigned char i;
	
	if(index >= DTC_SUM)
	{
		return;			
	}
	
	sDevice[index].ID = psDevicePara->ID;
	sDevice[index].Mark = psDevicePara->Mark;
	for(i=0; i<16; i++)
	{
		sDevice[index].DeviceName[i] = psDevicePara->DeviceName[i];
	}
	sDevice[index].DTCType = psDevicePara->DTCType;
	sDevice[index].ZoneType = psDevicePara->ZoneType;
	
	sDevice[index].Code[0] = psDevicePara->Code[0];
	sDevice[index].Code[1] = psDevicePara->Code[1];
	sDevice[index].Code[2] = psDevicePara->Code[2];
	
	Hal_I2C_EEPROM_PageWrite(STRU_DEVICEPARA_OFFSET + index * STRU_DTC_SIZE, (unsigned char*)(psDevicePara), STRU_DTC_SIZE);
	Hal_I2C_EEPROM_SequentialRead(STRU_DEVICEPARA_OFFSET + index * STRU_DTC_SIZE, (unsigned char*)(&sDevice[index]), STRU_DTC_SIZE);
}


/*----------------------------------------------------------------------------
@Name		: Device_CreatDTC(n)
@Function	: creat the specific number of detectors
@Parameter	: 
		--> n : number
@Note		:  Debug use
------------------------------------------------------------------------------*/
static void Device_CreatDTC(unsigned char n)
{
	unsigned char i, j, Temp;
	unsigned char NameStrIndex;
	
	for(i=0; i<n; i++)
	{
		sDevice[i].ID = i + 1;
		sDevice[i].Mark = 1;
		
		sDevice[i].DeviceName[0] = 'Z';
		sDevice[i].DeviceName[1] = 'o';
		sDevice[i].DeviceName[2] = 'n';
		sDevice[i].DeviceName[3] = 'e';
		sDevice[i].DeviceName[4] = '-';
		
		NameStrIndex = 5;
		Temp = i + 1;
		
		sDevice[i].NameNum = Temp;
		sDevice[i].DeviceName[NameStrIndex++] = '0' + (Temp / 100);
		sDevice[i].DeviceName[NameStrIndex++] = '0' + ((Temp % 100) / 10);
		sDevice[i].DeviceName[NameStrIndex++] = '0' + ((Temp % 100) % 10);
		
		for(j=NameStrIndex; j<16; j++)
		{
			sDevice[i].DeviceName[j] = 0;
		}
		
		sDevice[i].DTCType = DTC_DOOR;
		sDevice[i].ZoneType = ZONE_TYP_1ST;
		
		sDevice[i].Code[0] = 0x0C;
		sDevice[i].Code[1] = 0xBB;
		sDevice[i].Code[2] = 0xAA;
	}
}


/*----------------------------------------------------------------------------
@Name		: Device_ParaCheck()
@Function	: check the parameters of detectors
@Parameter	: Null
@Note		: error = 1 any error detected
------------------------------------------------------------------------------*/
static unsigned char Device_ParaCheck(void)
{
	unsigned char i;
	unsigned char error = 0;
	
	if(sDevice[0].ID != 1)
	{
		error = 1;
	}
	
	for(i=0; i<DTC_SUM; i++)
	{
		if(sDevice[i].ID > DTC_SUM)
		{
			error = 1;
		}
		if(sDevice[i].Mark > 1)
		{
			error = 1;
		}
		if(sDevice[i].NameNum > DTC_SUM)
		{
			error = 1;
		}
		if(sDevice[i].DTCType >= DTC_TYP_SUM)
		{
			error = 1;
		}
		if(sDevice[i].ZoneType >= STG_DEV_AT_SUM)
		{
			error = 1;
		}
	}
	
	return error;
}
