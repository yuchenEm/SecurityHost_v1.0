#ifndef __DEVICE_H_
#define __DEVICE_H_

#define DTC_SUM					20						

#define STRU_DTC_SIZE			sizeof(Stru_DTC)

#define STRU_SYSTEMPARA_SIZE	sizeof(SystemPara_InitTypeDef)	

#define STRU_DEVICEPARA_OFFSET	0


typedef enum
{
	DTC_DOOR,			// doorsensor
	DTC_PIR_MOTION,		// IRsensor
	DTC_REMOTE,			// remote
	
	DTC_TYP_SUM,
}DTC_TYPE_TYPEDEF;		

typedef enum
{
	ZONE_TYP_24HOURS,	
	ZONE_TYP_1ST,		
	ZONE_TYP_2ND,		
	
	STG_DEV_AT_SUM
}ZONE_TYPED_TYPEDEF;

typedef struct
{
	unsigned char ID;				// device ID
	unsigned char Mark;		 		// 0-unpaired 1-paired
	unsigned char NameNum;			
	unsigned char DeviceName[16];	// device name
	DTC_TYPE_TYPEDEF DTCType;		// device type
	ZONE_TYPED_TYPEDEF ZoneType;	

	unsigned char Code[3];			// ev1527/2262  24Bit
}Stru_DTC;

void Device_Init(void);
void Device_FactoryReset(void);

unsigned char Device_AddDTC(Stru_DTC *pDTC);
void Device_DeleteDTC(Stru_DTC *pDTC);

unsigned char Device_DTCMatching(unsigned char *pCode);
unsigned char Device_CheckDTCExisting(unsigned char Index);
unsigned char Device_GetDTCID(unsigned char Index);
unsigned char Device_GetDTCNum(void);

void Device_GetDTCStructure(Stru_DTC *psBuffer, unsigned char index);
void Device_SetDTCAttribute(unsigned char index, Stru_DTC *psDevicePara);


#endif
