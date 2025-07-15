#include "stm32f10x.h"
#include "app.h"
#include "device.h"
#include "hal_key.h"
#include "hal_led.h"
#include "hal_oled.h"
#include "hal_rfd.h"
#include "hal_i2c_eeprom.h"
#include "hal_beep.h"
#include "hal_nbiot.h"
#include "os_system.h"

static void menuInit(void);
static void showSystemTime(void);

static void gnlMenu_DesktopCBS(void);
static void stgMenu_MainMenuCBS(void);
static void stgMenu_LearnSensorCBS(void);
static void stgMenu_DTCListCBS(void);
static void stgMenu_MachineInfoCBS(void);
static void stgMenu_FactorySettingsCBS(void);

static void stgMenu_dl_ReviewMainCBS(void);
static void stgMenu_dl_ReviewCBS(void);
static void stgMenu_dl_EditCBS(void);
static void stgMenu_dl_DeleteCBS(void);

static void S_ENArmModeProc(void);
static void S_DisArmModeProc(void);
static void S_HomeArmModeProc(void);
static void S_AlarmModeProc(void);
static void SystemMode_Change(SYSTEMMODE_TYPEDEF sysMode);

static void HexToAscii(unsigned char *pHex, unsigned char *pAscii, int nLen);

static void KeyEventHandler(KEY_VALUE_TYPEDEF keys);
static void RFDRxHandler(unsigned char *pBuff);
static void ServerEventHandle(en_NBIot_MSG_TYPE type, unsigned char *pData);

static void ScreenControl(unsigned char cmd);

unsigned char *pMcuVersions = "v2.8";                        // MCU Firmware version
unsigned char *pHardVersions = "v7.0";                       // Hardware version

unsigned char NbIotWorkState;  // NBIot module work state
unsigned char bNbIotWorkState; // NBIot module work state backup
unsigned char NbIotCsQ;        // NBIot module CSQ
unsigned char bNbIotCsQ;       // NBIot module CSQ backup

unsigned short PutoutScreenTiemr;   // Timer for Screen-Turnoff
unsigned char ScreenState;          // 0->off, 1->on
unsigned short SetupMenuTimeOutCnt;

stu_mode_menu *pModeMenu;   	
stu_system_time stuSystemtime; 	

// Initialize the GeneralModeMenu
/**********************************************************************
		unsigned char ID;   			// = GNL_MENU_DESKTOP  
        MENU_POS menuPos;   			// = DESKTOP_MENU_POS
        unsigned char *pModeType; 		// = "Desktop"
        void (*action)(void); 			// = gnlMenu_DesktopCBS
        SCREEN_CMD refreshScreenCMD;  	// = SCREEN_CMD_RESET
        unsigned char reserved;  		// = 0
        unsigned char keyVal;    		// = 0xFF
        struct MODE_MENU *pLase;  		// = 0
        struct MODE_MENU *pNext;  		// = 0
        struct MODE_MENU *pParent; 		// = 0                     
        struct MODE_MENU *pChild;   	// = 0
*/
stu_mode_menu generalModeMenu[GNL_MENU_SUM] =
{
     {GNL_MENU_DESKTOP, DESKTOP_MENU_POS, "Desktop", gnlMenu_DesktopCBS, SCREEN_CMD_RESET, 0, 0xFF, 0, 0, 0, 0},
};

// Initialize the SettingModeMenu
stu_mode_menu settingModeMenu[STG_MENU_SUM] = 
{
    {STG_MENU_MAIN_SETTING,STG_MENU_POS,(unsigned char *)"Main Menu",stgMenu_MainMenuCBS,SCREEN_CMD_RESET,0,0xFF,0,0,0,0},                			 // 设置主界面
    {STG_MENU_LEARNING_SENSOR,STG_SUB_2_MENU_POS,(unsigned char *)"1. Learning Dtc",stgMenu_LearnSensorCBS,SCREEN_CMD_RESET,0,0xFF,0,0,0,0},         // 探测器配对界面
    {STG_MENU_DTC_LIST,STG_SUB_2_MENU_POS,(unsigned char *)"2. Dtc List",stgMenu_DTCListCBS,SCREEN_CMD_RESET,0,0xFF,0,0,0,0},                        // 探测器列表界面
    {STG_MENU_MACHINE_INFO,STG_SUB_2_MENU_POS,(unsigned char *)"3. Mac Info",stgMenu_MachineInfoCBS,SCREEN_CMD_RESET,0,0xFF,0,0,0,0},                // 设备信息界面
    {STG_MENU_FACTORY_SETTINGS,STG_SUB_2_MENU_POS,(unsigned char *)"4. Default Setting",stgMenu_FactorySettingsCBS,SCREEN_CMD_RESET,0,0xFF,0,0,0,0}, // 恢复出厂设置界面
};

// Initialize the DetectorListMenu
stu_mode_menu DL_ZX_Review[STG_MENU_DL_ZX_SUM] = 
{   
    {STG_MENU_DL_ZX_REVIEW_MAIN,STG_SUB_2_MENU_POS,(unsigned char *)"View",stgMenu_dl_ReviewMainCBS,SCREEN_CMD_RESET,0,0xFF,0,0,0,0},        
    {STG_MENU_DL_ZX_REVIEW,STG_SUB_3_MENU_POS,(unsigned char *)"View",stgMenu_dl_ReviewCBS,SCREEN_CMD_RESET,0,0xFF,0,0,0,0},                 
    {STG_MENU_DL_ZX_EDIT,STG_SUB_3_MENU_POS,(unsigned char *)"Edit",stgMenu_dl_EditCBS,SCREEN_CMD_RESET,0,0xFF,0,0,0,0},
    {STG_MENU_DL_ZX_DELETE,STG_SUB_3_MENU_POS,(unsigned char *)"Delete",stgMenu_dl_DeleteCBS,SCREEN_CMD_RESET,0,0xFF,0,0,0,0},
};


stu_system_mode *pStuSystemMode; 

stu_system_mode stu_Sysmode[SYSTEM_MODE_SUM] =            
{
    {SYSTEM_MODE_ENARM, SCREEN_CMD_RESET, 0xFF, S_ENArmModeProc},
    {SYSTEM_MODE_HOMEARM, SCREEN_CMD_RESET, 0xFF, S_HomeArmModeProc},
    {SYSTEM_MODE_DISARM, SCREEN_CMD_RESET, 0xFF, S_DisArmModeProc},
    {SYSTEM_MODE_ALARM, SCREEN_CMD_RESET, 0xFF, S_AlarmModeProc},
};


Queue8 RFD_RxMsg;	        // RFD Receiver Queue
Queue8 DtcTriggerIDMsg;     // Triggered Detector ID Queue

/*----------------------------------------------------------------------------
@Name		: App_Init()
@Function	: App Module Init
@Parameter	: Null
------------------------------------------------------------------------------*/
void App_Init(void)
{
	Hal_Key_KeyScanCBF_Register(KeyEventHandler);
	Hal_RFD_RxCBF_Register(RFDRxHandler);
    ServerEventCBFRegister(ServerEventHandle);
	
	QueueEmpty(RFD_RxMsg);
    QueueEmpty(DtcTriggerIDMsg);
    
    pStuSystemMode = &stu_Sysmode[SYSTEM_MODE_ENARM];  

	
	Hal_I2C_EEPROM_Init();
	Device_Init();
	Hal_Beep_Init();
	
	menuInit(); 

    NbIotWorkState = NBIOT_STA_INIT;     
    bNbIotWorkState = NBIOT_STA_INIT;   
    NbIotCsQ = 0xFF;                    // 0xFF invalid value as default

    PutoutScreenTiemr = 0;  
    ScreenState = 1;        
    SetupMenuTimeOutCnt= 0;
	
	hal_OledInit();
	hal_Oled_Refresh();

}


/*----------------------------------------------------------------------------
@Name		: menuInit()
@Function	: Menu Initialize
@Parameter	: Null
------------------------------------------------------------------------------*/
static void menuInit(void)
{
	unsigned char i;
	
    pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP]; 
    pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET; 
	stuSystemtime.year = 2024;
    stuSystemtime.mon = 3;
    stuSystemtime.day = 17;
    stuSystemtime.hour = 18;
    stuSystemtime.min = 0;
    stuSystemtime.week = 3;
		

    settingModeMenu[1].pLast = &settingModeMenu[STG_MENU_SUM-1]; 

    settingModeMenu[1].pNext = &settingModeMenu[2]; 
    
    settingModeMenu[1].pParent = &settingModeMenu[STG_MENU_MAIN_SETTING]; 

    for(i=2; i<STG_MENU_SUM-1; i++)
    {
        settingModeMenu[i].pLast = &settingModeMenu[i-1]; 

        settingModeMenu[i].pNext = &settingModeMenu[i+1]; 

        settingModeMenu[i].pParent = &settingModeMenu[STG_MENU_MAIN_SETTING];
    }

    settingModeMenu[STG_MENU_SUM-1].pLast = &settingModeMenu[i-1]; 
    
    settingModeMenu[STG_MENU_SUM-1].pNext = &settingModeMenu[1]; 
    
    settingModeMenu[STG_MENU_SUM-1].pParent = &settingModeMenu[STG_MENU_MAIN_SETTING];

    DL_ZX_Review[1].pLast = &DL_ZX_Review[STG_MENU_DL_ZX_SUM-1];
    DL_ZX_Review[1].pNext = &DL_ZX_Review[2];
    DL_ZX_Review[1].pParent = &DL_ZX_Review[STG_MENU_DL_ZX_REVIEW_MAIN];
    for(i=2; i<STG_MENU_DL_ZX_SUM-1; i++)
    {
        DL_ZX_Review[i].pLast = &DL_ZX_Review[i-1];
        DL_ZX_Review[i].pNext = &DL_ZX_Review[i+1];
        DL_ZX_Review[i].pParent = &DL_ZX_Review[STG_MENU_DL_ZX_REVIEW_MAIN];
    }

    DL_ZX_Review[STG_MENU_DL_ZX_SUM-1].pLast = &DL_ZX_Review[i-1];
    DL_ZX_Review[STG_MENU_DL_ZX_SUM-1].pNext = &DL_ZX_Review[1];
    DL_ZX_Review[STG_MENU_DL_ZX_SUM-1].pParent = &DL_ZX_Review[STG_MENU_DL_ZX_REVIEW_MAIN];
}


/*----------------------------------------------------------------------------
@Name		: showSystemTime()
@Function	: Display System Time
@Parameter	: Null
------------------------------------------------------------------------------*/
static void showSystemTime(void)
{
    //2024-04-12 19:00
    hal_Oled_ShowChar(14,55,(stuSystemtime.year/1000)+'0',8,1);
    hal_Oled_ShowChar(20,55,((stuSystemtime.year%1000)/100)+'0',8,1);
    hal_Oled_ShowChar(26,55,((stuSystemtime.year%1000%100)/10)+'0',8,1);
    hal_Oled_ShowChar(32,55,(stuSystemtime.year%1000%100%10)+'0',8,1);
  
    hal_Oled_ShowString(38,55,"-",8,1);
    
    hal_Oled_ShowChar(44,55,(stuSystemtime.mon/10)+'0',8,1);
    hal_Oled_ShowChar(50,55,(stuSystemtime.mon%10)+'0',8,1);

    hal_Oled_ShowString(56,55,"-",8,1);

    hal_Oled_ShowChar(62,55,(stuSystemtime.day/10)+'0',8,1);
    hal_Oled_ShowChar(68,55,(stuSystemtime.day%10)+'0',8,1);

    hal_Oled_ShowString(74,55," ",8,1);

    hal_Oled_ShowChar(80,55,(stuSystemtime.hour/10)+'0',8,1);
    hal_Oled_ShowChar(86,55,(stuSystemtime.hour%10)+'0',8,1);

    hal_Oled_ShowString(92,55,":",8,1);
    
    hal_Oled_ShowChar(98,55,(stuSystemtime.min/10)+'0',8,1);
    hal_Oled_ShowChar(104,55,(stuSystemtime.min%10)+'0',8,1);
    
    hal_Oled_Refresh();
}


/*----------------------------------------------------------------------------
@Name		: gnlMenu_DesktopCBS()
@Function	: Callback function for desktop menu
@Parameter	: Null
------------------------------------------------------------------------------*/
static void gnlMenu_DesktopCBS(void)
{
	unsigned char keys;
    static unsigned short timer = 0;
	
    if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
    {
        pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
        pStuSystemMode->refreshScreenCmd = SCREEN_CMD_RESET;
        
        pModeMenu->keyVal = 0xFF;
        
        hal_Oled_Clear();

        switch (NbIotWorkState) 
        {
            case NBIOT_STA_INIT:
            {
                hal_Oled_inco(0, ICON_NO_SIM); 
                hal_Oled_inco(1, ICON_ONENET_STA_BREAK); 
            }
            break;

            case NBIOT_SATE_GET_SIM: 
            {
                hal_Oled_inco(0, ICON_SIGNAL_L04); 
                hal_Oled_inco(1, ICON_ONENET_STA_BREAK); 
            }
            break; 

            case NBIOT_SATE_CONN_ONENET:
            {
                hal_Oled_inco(1, ICON_ONENET_STA_LINK); 
            } 
            break; 
        }
        
        bNbIotCsQ = 0xFF;
        bNbIotWorkState = 0xFF;
        
        showSystemTime();
        
        QueueEmpty(RFD_RxMsg);
        
        hal_Oled_Refresh();
    }

    if (bNbIotWorkState != NbIotWorkState) 
    {
        bNbIotWorkState = NbIotWorkState; 

        switch (NbIotWorkState) 
        {
            case NBIOT_STA_INIT: 
            {
                hal_Oled_inco(0, ICON_NO_SIM);             
                hal_Oled_inco(1, ICON_ONENET_STA_BREAK);   
            }
            break;

            case NBIOT_SATE_GET_SIM: 
            {
                hal_Oled_inco(0, ICON_SIGNAL_L03);      
                hal_Oled_inco(1, ICON_ONENET_STA_BREAK);   
                Hal_LED_MsgInput(LED_1, LED_BLINK4, 1);   
            }
            break;

            case NBIOT_SATE_CONN_ONENET: 
            {
                hal_Oled_inco(1, ICON_ONENET_STA_LINK);
                Hal_LED_MsgInput(LED_1, LED_ON, 1);   
            }   
            break;
        }
    }            
    
    if((NbIotWorkState == NBIOT_SATE_GET_SIM) || (NbIotWorkState == NBIOT_SATE_CONN_ONENET))
    {
        timer++; 
        
        if(timer > 200)
        {
            timer = 0; 
            showSystemTime(); 
        }

        if(bNbIotCsQ != NbIotCsQ)
        {
            bNbIotCsQ = NbIotCsQ; 
            
            switch(NbIotCsQ)
            {
                case NBIOT_CSQL_0: 
                    hal_Oled_inco(0, ICON_SIGNAL_L00); 
                break;

                case NBIOT_CSQL_1: 
                    hal_Oled_inco(0, ICON_SIGNAL_L01);  
                break;  

                case NBIOT_CSQL_2: 
                    hal_Oled_inco(0, ICON_SIGNAL_L02);  
                break;  

                case NBIOT_CSQL_3: 
                    hal_Oled_inco(0, ICON_SIGNAL_L03);  
                break;   

                case NBIOT_CSQL_4: 
                    hal_Oled_inco(0, ICON_SIGNAL_L04);  
                break;  

                default:           
                    hal_Oled_inco(0, ICON_NO_SIM); 
                break;
            }
        }
    } 
	
    if(pModeMenu->keyVal != 0xFF) 
    {
        keys = pModeMenu->keyVal;

        pModeMenu->keyVal = 0xFF;

        switch(keys)
        {
            case KEY6_LONG_PRESS: 

                pModeMenu = &settingModeMenu[0];

                pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET; 
                break;  
        }
    }
    
    timer++;

    if(timer > 200)
    {   
        timer = 0;
        showSystemTime();
    }

    pStuSystemMode->action();
}


/*----------------------------------------------------------------------------
@Name		: stgMenu_MainMenuCBS()
@Function	: callback function for main menu
@Parameter	: Null
------------------------------------------------------------------------------*/
static void stgMenu_MainMenuCBS(void)
{
    unsigned char i; 	
	unsigned char keys; 
	
	static stu_mode_menu *pMenu; 		
    static stu_mode_menu *bpMenu = 0; 	
    static unsigned char stgMainMenuSelectedPos = 0; 	
	
    if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
    {
        pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		hal_Oled_Clear(); 	
    
		hal_Oled_ShowString(37, 0, settingModeMenu[0].pModeType, 12, 1);
		hal_Oled_Refresh(); 
		
		for(i = 1; i < 5; i++)
		{
			hal_Oled_ShowString(0, 14 * i, settingModeMenu[i].pModeType, 8, 1);
			hal_Oled_Refresh();
		}
		
		pMenu = &settingModeMenu[1]; 	
        bpMenu = 0; 					
        
        stgMainMenuSelectedPos = 1; 	
		
		keys = 0xFF;   					
	}
	else if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RECOVER) 
    {
    
        pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;

        hal_Oled_Clear();
    
        hal_Oled_ShowString(37,0,settingModeMenu[0].pModeType,12,1);

        for(i = 1; i < 5; i++)
        {
            hal_Oled_ShowString(0, 14 * i, settingModeMenu[i].pModeType, 8, 1);
        }
                
        hal_Oled_Refresh();
    
        keys = 0xFF;
    
        bpMenu = 0;
    }
	
    if(pModeMenu->keyVal != 0xFF)
    {
        keys = pModeMenu->keyVal; 
        pModeMenu->keyVal = 0xFF; 

        switch(keys)
        {
			case KEY1_CLICK: 
                if(stgMainMenuSelectedPos == 1)
                {
                    hal_Oled_ShowString(0, 14 * stgMainMenuSelectedPos, pMenu->pModeType, 8, 1); 
                    hal_Oled_Refresh();
                    pMenu = pMenu->pLast; 
                    stgMainMenuSelectedPos = 4;
                }
                else
                {
                    hal_Oled_ShowString(0, 14 * stgMainMenuSelectedPos, pMenu->pModeType, 8, 1);
                    hal_Oled_Refresh();
                    pMenu = pMenu->pLast;
                    stgMainMenuSelectedPos--;
                }
                break;
				
            case KEY2_CLICK: 
                if(stgMainMenuSelectedPos == 4)
                {
                    hal_Oled_ShowString(0, 14 * stgMainMenuSelectedPos, pMenu->pModeType, 8, 1);
                    pMenu = pMenu->pNext;
                    stgMainMenuSelectedPos = 1;
                }
                else
                {
                    hal_Oled_ShowString(0, 14 * stgMainMenuSelectedPos, pMenu->pModeType, 8, 1);
                    hal_Oled_Refresh();
                    pMenu = pMenu->pNext;
                    stgMainMenuSelectedPos++;
                }
                break;
				
            case KEY5_CLICK_RELEASE: 
                pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];
                
                pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
                break;
			
			case KEY6_CLICK_RELEASE: 
                pModeMenu->pChild = pMenu; 
             
                pModeMenu = pModeMenu->pChild; 
                
                pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET; 
                break; 
        }
    }
	
    if(bpMenu != pMenu)
    {
        bpMenu = pMenu;
        hal_Oled_ShowString(0, 14 * stgMainMenuSelectedPos, pMenu->pModeType, 8, 0); 
        hal_Oled_Refresh();
    }
}

/*----------------------------------------------------------------------------
@Name		: stgMenu_LearnSensorCBS()
@Function	: Sensor Learning Menu
@Parameter	: Null
------------------------------------------------------------------------------*/
static void stgMenu_LearnSensorCBS(void)
{
	unsigned char keys;
	unsigned char dat;
	unsigned char tBuff[3];
	
	static unsigned char PairingComplete = 0; 	// learning flag，1->learning successfully
    static unsigned short Timer = 0;        	
    Stru_DTC stuTempDevice;  
    
    if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
    {
        pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
    
        hal_Oled_Clear();
    
        hal_Oled_ShowString(28,0,"Learning DTC",12,1);

        hal_Oled_ShowString(43,28,"Pairing...",8,1);
    
        hal_Oled_Refresh();

        keys = 0xFF;
		
		PairingComplete = 0; 
        Timer = 0;
    }  
	
    if(QueueDataLen(RFD_RxMsg) && (!PairingComplete))
    {
        QueueDataOut(RFD_RxMsg, &dat);

        if(dat == '#')
        {
            QueueDataOut(RFD_RxMsg,&tBuff[2]);      // address 
            QueueDataOut(RFD_RxMsg,&tBuff[1]);      // address 
            QueueDataOut(RFD_RxMsg,&tBuff[0]);     	// code 
    
            hal_Oled_ClearArea(0,28,128,36);
    
            stuTempDevice.Code[2] = tBuff[2];   
            stuTempDevice.Code[1] = tBuff[1];  
            stuTempDevice.Code[0] = tBuff[0];  
    
            if((stuTempDevice.Code[0]==SENSOR_CODE_DOOR_OPEN) ||
               (stuTempDevice.Code[0]==SENSOR_CODE_DOOR_CLOSE) ||
               (stuTempDevice.Code[0]==SENSOR_CODE_DOOR_TAMPER)||
               (stuTempDevice.Code[0]==SENSOR_CODE_DOOR_LOWPWR))
            {
                stuTempDevice.DTCType = DTC_DOOR;               
            }
			else if((stuTempDevice.Code[0]==SENSOR_CODE_REMOTE_ENARM) ||
                    (stuTempDevice.Code[0]==SENSOR_CODE_REMOTE_DISARM) ||
                    (stuTempDevice.Code[0]==SENSOR_CODE_REMOTE_HOMEARM) ||
                    (stuTempDevice.Code[0]==SENSOR_CODE_REMOTE_SOS))
            {
                stuTempDevice.DTCType = DTC_REMOTE;
            }  
            
            stuTempDevice.ZoneType = ZONE_TYP_1ST;
    
            if(Device_AddDTC(&stuTempDevice) != 0xFF)
            {
                switch(stuTempDevice.DTCType)
                {
                    case DTC_DOOR:    
                        hal_Oled_ShowString(34,28,"Success!",8,1);
                        hal_Oled_ShowString(16,36,"Added door dtc..",8,1);
                    break;
                    
                    case DTC_REMOTE: 
                        hal_Oled_ShowString(34,28,"Success!",8,1);
                        hal_Oled_ShowString(7,36,"Added remote dtc..",8,1);
                    break;
    
               
                }
    
                hal_Oled_Refresh();
    
                PairingComplete = 1;

                Timer = 0;
            }
            else
            {
                hal_Oled_ShowString(34,28,"Fail...",8,1);
                hal_Oled_Refresh();
            }
        }
    }
    
    if(PairingComplete)
    {
        Timer++;
    
        if(Timer > 150)
        {
            Timer = 0;

            pModeMenu = pModeMenu->pParent;

            pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
        }
    }

    if(pModeMenu->keyVal != 0xff)
    {
        keys = pModeMenu->keyVal;
    
        pModeMenu->keyVal = 0xFF;
      
        switch(keys)
        {
            
            case KEY5_CLICK_RELEASE:    
                pModeMenu = pModeMenu->pParent;
                
                pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
                break; 
    
            case KEY5_LONG_PRESS:    
                pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];
              
                pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
                break; 
        }
    }
}

/*----------------------------------------------------------------------------
@Name		: stgMenu_DTCListCBS()
@Function	: Detector List Menu
@Parameter	: Null
------------------------------------------------------------------------------*/
static void stgMenu_DTCListCBS(void)
{	
    unsigned char keys; 			
	unsigned char ClrScreenFlag; 	// Screen_Clear flag, when the menu list need to roll-over, set this flag to 1
    unsigned char i,j;
    
    Stru_DTC tStuDtc;
    
    static unsigned char DtcNameBuff[DTC_SUM][16];

    static stu_mode_menu settingMode_DTCList_Sub_Menu[DTC_SUM];

    static stu_mode_menu *pMenu;

    static stu_mode_menu *bpMenu = 0;

    static unsigned char stgMainMenuSelectedPos = 0;
        
    static stu_mode_menu *MHead;
  
    static unsigned char pMenuIdx = 0;
    
    if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
    {
        pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;

        pMenuIdx = 0;
        stgMainMenuSelectedPos = 1;
        bpMenu = 0;
        ClrScreenFlag = 1;
        pMenu = settingMode_DTCList_Sub_Menu; 
        
        keys = 0xFF;
        
        hal_Oled_Clear();
        hal_Oled_ShowString(40,0,"Dtc List",12,1);
        hal_Oled_Refresh();

        for(i=0; i<DTC_SUM; i++)
        {
            if(Device_CheckDTCExisting(i))
            {
                Device_GetDTCStructure(&tStuDtc,i); 

                (pMenu+pMenuIdx)->ID = pMenuIdx;   
                
                (pMenu+pMenuIdx)->menuPos = STG_SUB_3_MENU_POS;

                (pMenu+pMenuIdx)->reserved = tStuDtc.ID-1; 
               
                 
                for(j=0; j<16; j++)
                {
                    DtcNameBuff[pMenuIdx][j] = tStuDtc.DeviceName[j];
                }
         
                (pMenu+pMenuIdx)->pModeType = DtcNameBuff[pMenuIdx];

                pMenuIdx++; 
            }
        }

        // at least one detector exists
        if(pMenuIdx != 0)
        {
            if(pMenuIdx > 1)
            {
                pMenu->pLast =  pMenu+(pMenuIdx-1);
                pMenu->pNext =  pMenu+1;
                pMenu->pParent = &settingModeMenu[STG_MENU_MAIN_SETTING];
                for(i=1; i<pMenuIdx-1; i++)
                {
                    (pMenu+i)->pLast =  pMenu+(i-1);
                    (pMenu+i)->pNext = pMenu+(i+1);
                    (pMenu+i)->pParent = &settingModeMenu[STG_MENU_MAIN_SETTING];
                }
                (pMenu+(pMenuIdx-1))->pLast =  pMenu+(i-1);
                (pMenu+(pMenuIdx-1))->pNext = pMenu;
                (pMenu+(pMenuIdx-1))->pParent = &settingModeMenu[STG_MENU_MAIN_SETTING];
            }
            else if(pMenuIdx == 1) 
            {
                pMenu->pLast = pMenu;
                pMenu->pNext = pMenu;
                pMenu->pParent = &settingModeMenu[STG_MENU_MAIN_SETTING];
            }
        }
        else
        {
            bpMenu = pMenu;
            hal_Oled_ShowString(0,14," No detectors.",8,1);
            hal_Oled_Refresh();
        }
                
        MHead = pMenu;
    }

    else if(pModeMenu->refreshScreenCmd==SCREEN_CMD_RECOVER)
    {        
        pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
        hal_Oled_Clear();
        hal_Oled_ShowString(40,0,"Dtc List",12,1);
        hal_Oled_Refresh();
        keys = 0xFF;
        ClrScreenFlag = 1;
        bpMenu = 0;
    }

    if(pModeMenu->keyVal != 0xFF)
    {
        keys = pModeMenu->keyVal;

        pModeMenu->keyVal = 0xFF;
        
        switch(keys)
        {
            case KEY1_CLICK_RELEASE:
                if(pMenuIdx < 2)
                {

                }
                else if(pMenuIdx < 5)
                {
                    if(stgMainMenuSelectedPos == 1)
                    {
                        stgMainMenuSelectedPos = pMenuIdx; 
                        ClrScreenFlag = 1;
                        pMenu = pMenu->pLast; 
                    }
                    else
                    {
                        hal_Oled_ShowString(0, 14 * stgMainMenuSelectedPos, pMenu->pModeType, 8, 1); 
                        hal_Oled_Refresh(); 
                        pMenu = pMenu->pLast; 
                        stgMainMenuSelectedPos--; 
                    }
                }
                else if(pMenuIdx > 4)
                {
                    // multiple lines more than one page, need to scroll
                    if(stgMainMenuSelectedPos == 1) 
                    { 
                        MHead = MHead->pLast;
                        pMenu = pMenu->pLast; 
                        stgMainMenuSelectedPos = 1; 
                        ClrScreenFlag = 1; 
                    }
                    else
                    {
                        hal_Oled_ShowString(0, 14 * stgMainMenuSelectedPos, pMenu->pModeType, 8, 1); 
                        hal_Oled_Refresh();
                        pMenu = pMenu->pLast; 
                        stgMainMenuSelectedPos--; 
                    }
                }
            break;
            
            case KEY2_CLICK_RELEASE: 
                if(pMenuIdx < 2)
                {

                }
                else if(pMenuIdx < 5)
                {
                    // only one page
                    if(stgMainMenuSelectedPos == pMenuIdx) 
                    {
                        pMenu = pMenu->pNext; 
                        stgMainMenuSelectedPos = 1;
                        ClrScreenFlag = 1; 
                    }
                    else
                    {
                        hal_Oled_ShowString(0, 14 * stgMainMenuSelectedPos, pMenu->pModeType, 8, 1); 
                        hal_Oled_Refresh(); 
                        pMenu = pMenu->pNext; 
                        stgMainMenuSelectedPos++;
                    }
                }
                else if(pMenuIdx > 4)
                {
                    if(stgMainMenuSelectedPos == 4) 
                    {
                        MHead = MHead->pNext;
                        pMenu = pMenu->pNext; 
                        stgMainMenuSelectedPos = 4; 
                        ClrScreenFlag = 1; 
                    }
                    else
                    {
                        hal_Oled_ShowString(0, 14 * stgMainMenuSelectedPos, pMenu->pModeType, 8, 1); 
                        hal_Oled_Refresh(); 
                        pMenu = pMenu->pNext; 
                        stgMainMenuSelectedPos++;
                    }
                }
            break;

            case KEY5_CLICK_RELEASE:
                pModeMenu = pModeMenu->pParent;

                pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
            break;
			
            case KEY5_LONG_PRESS:
                pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];

                pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
            break;

            case KEY6_CLICK_RELEASE:
                if(pMenuIdx>0)
                {
                    pModeMenu = &DL_ZX_Review[STG_MENU_DL_ZX_REVIEW_MAIN]; 

                    pModeMenu->reserved = pMenu->reserved;       
                    pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
                }
            break;
        }
    }

    if(bpMenu != pMenu)
    {
        bpMenu = pMenu;
        
        if(ClrScreenFlag)    
        {
            ClrScreenFlag = 0;  
            pMenu = MHead;      
            hal_Oled_ClearArea(0,14,128,50);
            hal_Oled_Refresh();
        
            if(pMenuIdx < 4)
            {
                for(i=0; i<pMenuIdx; i++)
                {
                    hal_Oled_ShowString(0,14*(i+1),pMenu->pModeType,8,1);
                    hal_Oled_Refresh();
                    pMenu = pMenu->pNext; 
                }
            }
            else
            {
                for(i=1; i<5; i++)
                {
                    hal_Oled_ShowString(0,14*i,pMenu->pModeType,8,1);
                    hal_Oled_Refresh();
                    pMenu = pMenu->pNext; 
                } 
            }
        
            pMenu = bpMenu; 
            hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,0);
            hal_Oled_Refresh(); 
        }
        else
        { 
            hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,0);        
            hal_Oled_Refresh(); 
        }     
    } 

}


/*----------------------------------------------------------------------------
@Name		: stgMenu_dl_ReviewMainCBS()
@Function	: Detector List Review Menu
@Parameter	: Null
------------------------------------------------------------------------------*/
static void stgMenu_dl_ReviewMainCBS(void)
{
    unsigned char keys = 0xFF;
    unsigned char i,ClrScreenFlag=0;
    Stru_DTC tStuDtc;
     
    static stu_mode_menu *MHead;                    
    static stu_mode_menu *pMenu,*bpMenu=0;        
    
    static unsigned char stgMainMenuSelectedPos=0;                                 

    if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
    {  
        pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
            
        if(Device_CheckDTCExisting(pModeMenu->reserved))
        {
            Device_GetDTCStructure(&tStuDtc,pModeMenu->reserved); 
        }
             
             
        hal_Oled_Clear();
        hal_Oled_ShowString(40,0,tStuDtc.DeviceName,12,1);
            
        hal_Oled_Refresh();
            
        pMenu = &DL_ZX_Review[1];
        stgMainMenuSelectedPos = 1;
        MHead = pMenu;               
        ClrScreenFlag = 1;
        bpMenu = 0;
        keys = 0xFF;
    } 

    else if(pModeMenu->refreshScreenCmd==SCREEN_CMD_RECOVER)
    {
        pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;

        if(Device_CheckDTCExisting(pModeMenu->reserved))
        {
            Device_GetDTCStructure(&tStuDtc,pModeMenu->reserved);  
        }
         
        hal_Oled_Clear();
        hal_Oled_ShowString(40,0,tStuDtc.DeviceName,12,1);
        
        hal_Oled_Refresh();
        keys = 0xFF;
        ClrScreenFlag = 1;
        bpMenu = 0;
    }

    if(pModeMenu->keyVal != 0xFF)
    {
        keys = pModeMenu->keyVal;
        pModeMenu->keyVal = 0xFF;        
        switch(keys)
        {
            case KEY5_CLICK_RELEASE:
                pModeMenu = &settingModeMenu[STG_MENU_DTC_LIST];     

                pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
            break;

            case KEY5_LONG_PRESS:
                pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];

                pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
            break;

            case KEY1_CLICK:  
                if(stgMainMenuSelectedPos ==1)
                {
                    pMenu = pMenu->pLast;
                    stgMainMenuSelectedPos = 3;
                    ClrScreenFlag = 1;
                }
                else
                {
                    hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,1);
                    hal_Oled_Refresh();

                    pMenu = pMenu->pLast;
                    stgMainMenuSelectedPos--;
                }
            break;

            case KEY2_CLICK:  
                if(stgMainMenuSelectedPos ==3)
                {
                    pMenu = pMenu->pNext;
                    stgMainMenuSelectedPos = 1;
                    ClrScreenFlag = 1;
                }
                else
                {
                    hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,1);
                    hal_Oled_Refresh();

                    pMenu = pMenu->pNext;                                                                                                                                                        //切换下一个选项
                    stgMainMenuSelectedPos++;
                }
            break;
            
            case KEY6_CLICK_RELEASE:
                pMenu->reserved = pModeMenu->reserved;

                pModeMenu = pMenu;

                pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
            break;
        }
    }
    
    if(bpMenu != pMenu)
    {
        bpMenu = pMenu;
        if(ClrScreenFlag)
        {
            ClrScreenFlag = 0;
            pMenu = MHead;
            hal_Oled_ClearArea(0,14,128,50);
            hal_Oled_Refresh();
            for(i=0; i<3; i++)
            {
                hal_Oled_ShowString(0,14*(i+1),pMenu->pModeType,8,1);
                hal_Oled_Refresh();

                pMenu = pMenu->pNext;
            } 

            pMenu = bpMenu;
            
            hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,0);
            hal_Oled_Refresh();
                     
        }
        else
        { 
            hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,0);        
            hal_Oled_Refresh();
        }                 
    }
}

/*----------------------------------------------------------------------------
@Name		: stgMenu_dl_ReviewCBS()
@Function	: Detector Review Menu
@Parameter	: Null
------------------------------------------------------------------------------*/
static void stgMenu_dl_ReviewCBS(void)
{
    unsigned char keys = 0xFF;
    Stru_DTC tStuDtc;

    // 2262 protocol (3bytes): 2byte->address, 1byte->function code
    unsigned char temp[6];
    
    if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
    {  
        pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
         
        if(Device_CheckDTCExisting(pModeMenu->reserved))
        {
            Device_GetDTCStructure(&tStuDtc,pModeMenu->reserved);
        }
         

        hal_Oled_Clear();
        hal_Oled_ShowString(40,0,pModeMenu->pModeType,12,1);
        
        hal_Oled_ShowString(0,16,"<Name>: ",8,1); 
        
        hal_Oled_ShowString(48,16,tStuDtc.DeviceName,8,1);
        
        hal_Oled_ShowString(0,28,"<Type>: ",8,1);
        
        if(tStuDtc.DTCType == DTC_DOOR)
        {
            hal_Oled_ShowString(48,28,"door dtc",8,1);
        }
        else if(tStuDtc.DTCType == DTC_PIR_MOTION)
        {
            hal_Oled_ShowString(48,28,"pir dtc",8,1);
        }
        else if(tStuDtc.DTCType == DTC_REMOTE)
        {
            hal_Oled_ShowString(48,28,"remote",8,1);
        }
        
        hal_Oled_ShowString(0,40,"<ZoneType>: ",8,1);
        
        if(tStuDtc.ZoneType == ZONE_TYP_24HOURS)
        {
            hal_Oled_ShowString(72,40,"24 hrs",8,1);
        }
        else if(tStuDtc.ZoneType == ZONE_TYP_1ST)
        {
            hal_Oled_ShowString(72,40,"1ST",8,1);
        }
        else if(tStuDtc.ZoneType == ZONE_TYP_2ND)
        {
            hal_Oled_ShowString(72,40,"2ND",8,1);
        }
        
        hal_Oled_ShowString(0,52,"<RFCode>: ",8,1);
        
        HexToAscii(tStuDtc.Code,temp,3);
        
        
        hal_Oled_ShowChar(60,52,temp[4],8,1);
        hal_Oled_ShowChar(66,52,temp[5],8,1);
        
        hal_Oled_ShowChar(72,52,' ',8,1);
        
        hal_Oled_ShowChar(80,52,temp[2],8,1);
        hal_Oled_ShowChar(86,52,temp[3],8,1);
        
        hal_Oled_Refresh();
            
    }
    
    if(pModeMenu->keyVal != 0xFF)
    {
        keys = pModeMenu->keyVal;
        pModeMenu->keyVal = 0xFF;
        switch(keys)
        {
            case KEY5_CLICK_RELEASE:
                pModeMenu = &DL_ZX_Review[STG_MENU_DL_ZX_REVIEW_MAIN];
                pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;  
            break;

            case KEY6_CLICK_RELEASE:
                pModeMenu = &DL_ZX_Review[STG_MENU_DL_ZX_REVIEW_MAIN];
                pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
            break;

            case KEY5_LONG_PRESS:
                pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];
                pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
            break;          
        }
    }
}


/*----------------------------------------------------------------------------
@Name		: stgMenu_dl_EditCBS()
@Function	: Detector attribute Edit Menu
@Parameter	: Null
------------------------------------------------------------------------------*/
static void stgMenu_dl_EditCBS(void)
{
    unsigned char keys = 0xFF;
    static Stru_DTC tStuDtc;
    static unsigned short timer = 0;
    static unsigned char editComplete = 0;
    static unsigned char stgMainMenuSelectedPos=0;
    static unsigned char setValue = DTC_DOOR;
    static unsigned char *pDL_ZX_Edit_DTCType_Val[DTC_TYP_SUM] =
    {
        "door dtc",
        "pir dtc",
        "remote",
    };

    static unsigned char *pDL_ZX_Edit_ZoneType_Val[STG_DEV_AT_SUM] =
    {
        "24 hrs",
        "1ST",
        "2ND",
    };
    

    if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
    {   
        pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
        stgMainMenuSelectedPos = 0; 
        if(Device_CheckDTCExisting(pModeMenu->reserved))
        {
            Device_GetDTCStructure(&tStuDtc,pModeMenu->reserved);
        }
         
        stgMainMenuSelectedPos = 0;
        setValue = tStuDtc.DTCType;
        hal_Oled_Clear();
        hal_Oled_ShowString(40,0,pModeMenu->pModeType,12,1);
        hal_Oled_Refresh();
        
        hal_Oled_ShowString(0,16,"<Name>: ",8,1); 
        hal_Oled_ShowString(48,16,tStuDtc.DeviceName,8,1);
        
        hal_Oled_ShowString(0,28,"<Type>: ",8,1);
        hal_Oled_Refresh();
        
        if(tStuDtc.DTCType == DTC_DOOR)
        {
                hal_Oled_ShowString(48,28,"door dtc",8,0);
        }else if(tStuDtc.DTCType == DTC_PIR_MOTION)
        {
                hal_Oled_ShowString(48,28,"pir dtc",8,0);
        }else if(tStuDtc.DTCType == DTC_REMOTE)
        {
                hal_Oled_ShowString(48,28,"remote",8,0);
        }
        hal_Oled_Refresh();
        
        hal_Oled_ShowString(0,40,"<ZoneType>: ",8,1);
        hal_Oled_Refresh();
        
        if(tStuDtc.ZoneType == ZONE_TYP_24HOURS)
        {
                hal_Oled_ShowString(72,40,"24 hrs",8,1);
        }else if(tStuDtc.ZoneType == ZONE_TYP_1ST)
        {
                hal_Oled_ShowString(72,40,"1ST",8,1);
        }else if(tStuDtc.ZoneType == ZONE_TYP_2ND)
        {
                hal_Oled_ShowString(72,40,"2ND",8,1);
        }
        
        hal_Oled_Refresh();
         
        editComplete = 0;
        timer = 0;
    }
    
    if(pModeMenu->keyVal != 0xFF)
    {
        keys = pModeMenu->keyVal;
        pModeMenu->keyVal = 0xFF;        
        
        if(keys == KEY3_CLICK_RELEASE)    
        {
            if(stgMainMenuSelectedPos == 0)                
            {
                if(setValue == DTC_DOOR)
                {
                        setValue = DTC_TYP_SUM-1;
                }else
                {
                        setValue--;
                }
                tStuDtc.DTCType = (DTC_TYPE_TYPEDEF)setValue;  
                hal_Oled_ClearArea(48,28,80,8);               
                hal_Oled_ShowString(48,28,pDL_ZX_Edit_DTCType_Val[setValue],8,0);
                hal_Oled_Refresh();

            }
            else if(stgMainMenuSelectedPos == 1)
            {
                if(setValue == ZONE_TYP_24HOURS)
                {
                        setValue = STG_DEV_AT_SUM-1;
                }else
                {
                        setValue--;
                }
                tStuDtc.ZoneType = (ZONE_TYPED_TYPEDEF)setValue;  
                hal_Oled_ClearArea(72,40,56,8);                   
                hal_Oled_ShowString(72,40,pDL_ZX_Edit_ZoneType_Val[setValue],8,0);
                hal_Oled_Refresh();
            }
        }
        else if(keys == KEY4_CLICK_RELEASE)    
        {
            if(stgMainMenuSelectedPos == 0)
            {
                if(setValue == (DTC_TYP_SUM-1))
                {
                    setValue = 0;
                }else
                {
                    setValue++;
                }
                tStuDtc.DTCType = (DTC_TYPE_TYPEDEF)setValue;  
                hal_Oled_ClearArea(48,28,80,8);                
                hal_Oled_ShowString(48,28,pDL_ZX_Edit_DTCType_Val[setValue],8,0);
                hal_Oled_Refresh();
            }
            else if(stgMainMenuSelectedPos == 1)
            {
                if(setValue == (STG_DEV_AT_SUM-1))
                {
                    setValue = 0;
                }else
                {
                    setValue++;
                }
                tStuDtc.ZoneType = (ZONE_TYPED_TYPEDEF)setValue; 

                hal_Oled_ClearArea(72,40,56,8);                  
                hal_Oled_ShowString(72,40,pDL_ZX_Edit_ZoneType_Val[setValue],8,0);
                hal_Oled_Refresh();
            }
                
        }
        else if((keys==KEY1_CLICK_RELEASE) || (keys==KEY2_CLICK_RELEASE))
        {
            if(stgMainMenuSelectedPos == 0)
            {
                stgMainMenuSelectedPos = 1;
                setValue = tStuDtc.ZoneType;
                hal_Oled_ClearArea(48,28,80,8);        

                hal_Oled_ShowString(48,28,pDL_ZX_Edit_DTCType_Val[tStuDtc.DTCType],8,1);       

                hal_Oled_ClearArea(72,40,56,8);   

                hal_Oled_ShowString(72,40,pDL_ZX_Edit_ZoneType_Val[setValue],8,0);           
                hal_Oled_Refresh();
            }
            else
            {
                stgMainMenuSelectedPos = 0;
                setValue = tStuDtc.DTCType;
                hal_Oled_ClearArea(48,28,80,8);
                        
                hal_Oled_ShowString(48,28,pDL_ZX_Edit_DTCType_Val[setValue],8,0);               
                        
                        
                hal_Oled_ClearArea(72,40,56,8); 

                hal_Oled_ShowString(72,40,pDL_ZX_Edit_ZoneType_Val[tStuDtc.ZoneType],8,1);       
                        
                hal_Oled_Refresh();
            }
        }
        else if(keys == KEY5_CLICK_RELEASE)   
        {
            pModeMenu = &DL_ZX_Review[STG_MENU_DL_ZX_REVIEW_MAIN];
            pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
                         
        }
        else if(keys == KEY6_CLICK_RELEASE)  
        {
            timer = 0;
            Device_SetDTCAttribute(tStuDtc.ID-1,&tStuDtc); 
            editComplete = 1;
            hal_Oled_Clear();
            hal_Oled_ShowString(16,20,"Update..",24,1);
            hal_Oled_Refresh();         
        }
        else if(keys == KEY5_LONG_PRESS)  
        {
            pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];
            pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
        }
    }
    if(editComplete)
    {
        timer++;
        if(timer > 150) 
        {
            timer = 0;
            editComplete = 0;
            pModeMenu = &DL_ZX_Review[STG_MENU_DL_ZX_REVIEW_MAIN];
            pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
        }
    }
}

/*----------------------------------------------------------------------------
@Name		: stgMenu_dl_DeleteCBS()
@Function	: delete detectors Menu
@Parameter	: Null
------------------------------------------------------------------------------*/
static void stgMenu_dl_DeleteCBS(void)
{
    unsigned char keys = 0xFF;
    static Stru_DTC tStuDtc;
    static unsigned short timer = 0;
    static unsigned char DelComplete = 0;
    
    static unsigned char stgMainMenuSelectedPos=0;

    if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
    {   
        pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
        stgMainMenuSelectedPos = 0; 
        if(Device_CheckDTCExisting(pModeMenu->reserved))
        {
            Device_GetDTCStructure(&tStuDtc,pModeMenu->reserved);
        }
             
        stgMainMenuSelectedPos = 0;
            
        hal_Oled_Clear();
        hal_Oled_ShowString(46,0,pModeMenu->pModeType,12,1);
            
        //del zone-001
        hal_Oled_ShowString(28,14,"Del ",12,1); 
        hal_Oled_ShowString(52,14,tStuDtc.DeviceName,12,1); 
            
            
        hal_Oled_ShowString(25,28,"Are you sure?",12,1); 
        //yes   no
        hal_Oled_ShowString(40,48,"Yes",12,1); 
        hal_Oled_ShowString(88,48,"No",12,0); 
               
        hal_Oled_Refresh();
 
        DelComplete = 0;
        timer = 0;
    }
    
    if(pModeMenu->keyVal != 0xFF)
    {
        keys = pModeMenu->keyVal;
        pModeMenu->keyVal = 0xFF;        
            
        if((keys == KEY3_CLICK_RELEASE) || (keys == KEY4_CLICK_RELEASE))
        {
            if(stgMainMenuSelectedPos == 0)                
            {
                stgMainMenuSelectedPos = 1;
                hal_Oled_ClearArea(40,48,88,16);     
                hal_Oled_ShowString(40,48,"Yes",12,0); 
                hal_Oled_ShowString(88,48,"No",12,1); 
                hal_Oled_Refresh();

            }
            else if(stgMainMenuSelectedPos == 1)
            {
                stgMainMenuSelectedPos = 0;
                hal_Oled_ClearArea(40,48,88,16);   
                hal_Oled_ShowString(40,48,"Yes",12,1); 
                hal_Oled_ShowString(88,48,"No",12,0); 
                hal_Oled_Refresh();

            }
        }
        else if(keys == KEY6_CLICK_RELEASE)
        {
            if(stgMainMenuSelectedPos)
            {
                DelComplete = 1;
                timer = 0;
                tStuDtc.Mark = 0;

                Device_SetDTCAttribute(tStuDtc.ID-1,&tStuDtc);  
                hal_Oled_Clear();
                hal_Oled_ShowString(16,20,"Update..",24,1);
                hal_Oled_Refresh();        
            }
            else
            {
                pModeMenu = pModeMenu->pParent;

                pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET; 
            }
        }
        else if(keys == KEY5_CLICK_RELEASE)
        {
            pModeMenu = &DL_ZX_Review[STG_MENU_DL_ZX_REVIEW_MAIN];

            pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
        }
        else if(keys == KEY5_LONG_PRESS)
        {
            pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];

            pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
        }
    }

    if(DelComplete)
    {
        timer++;
        if(timer > 150)     
        {
            timer = 0;
            DelComplete = 0;
            pModeMenu = &settingModeMenu[STG_MENU_DTC_LIST];
            pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
        }
    }
}

/*----------------------------------------------------------------------------
@Name		: stgMenu_MachineInfoCBS()
@Function	: Host info Menu
@Parameter	: Null
------------------------------------------------------------------------------*/
static void stgMenu_MachineInfoCBS(void)
{
    unsigned char keys;
    
    if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
    {
        pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
  
        hal_Oled_Clear();
        
        hal_Oled_ShowString(40, 0, "Mac info", 12, 1);
        
        hal_Oled_ShowString(0, 16, "<mcu ver>: ", 12, 1);

        hal_Oled_ShowString(66, 16, pMcuVersions, 12, 1);

        hal_Oled_ShowString(0, 32, "<hard ver>: ", 12, 1);

        hal_Oled_ShowString(72, 32, pHardVersions, 12, 1);
        
        hal_Oled_Refresh();

        keys = 0xFF;
    }
    
    if(pModeMenu->keyVal != 0xFF)
    {
        keys = pModeMenu->keyVal;
        
        pModeMenu->keyVal = 0xFF;
        
        if(keys == KEY5_CLICK_RELEASE)
        {
            pModeMenu = pModeMenu->pParent;
            
            pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
        }
        else if(keys == KEY5_LONG_PRESS)
        {
            pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];
            
            pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
        }
    }
}

/*----------------------------------------------------------------------------
@Name		: stgMenu_FactorySettingsCBS()
@Function	: Factory Reset Menu
@Parameter	: Null
------------------------------------------------------------------------------*/
static void stgMenu_FactorySettingsCBS(void)
{
    unsigned char keys = 0xFF;

    static unsigned short timer = 0;
    
    static unsigned char Complete = 0;
    
    static unsigned char stgMainMenuSelectedPos = 0;

    if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
    {
        pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
        
        stgMainMenuSelectedPos = 0;

        hal_Oled_Clear();
        
        hal_Oled_ShowString(19, 0, "Default setting", 12, 1);

        hal_Oled_ShowString(25, 28, "Are you sure?", 12, 1);
        
        hal_Oled_ShowString(40, 48, "Yes", 12, 1);
        hal_Oled_ShowString(88, 48, "No", 12, 0);

        hal_Oled_Refresh();

        Complete = 0;
        timer = 0;
    }

    if(pModeMenu->keyVal != 0xFF)
    {
        keys = pModeMenu->keyVal;

        pModeMenu->keyVal = 0xFF;

        if(!Complete)
        {
            if((keys == KEY3_CLICK_RELEASE) || (keys == KEY4_CLICK_RELEASE))
            {
                if(stgMainMenuSelectedPos == 0)
                {
                    stgMainMenuSelectedPos = 1;

                    hal_Oled_ClearArea(40, 48, 88, 16);
                    
                    hal_Oled_ShowString(40, 48, "Yes", 12, 0);
                    
                    hal_Oled_ShowString(88, 48, "No", 12, 1);

                    hal_Oled_Refresh();
                }
                else if(stgMainMenuSelectedPos == 1)
                {
                    stgMainMenuSelectedPos = 0;

                    hal_Oled_ClearArea(40, 48, 88, 16);

                    hal_Oled_ShowString(40, 48, "Yes", 12, 1);

                    hal_Oled_ShowString(88, 48, "No", 12, 0);
                    
                    hal_Oled_Refresh();
                }
            }
            else if(keys == KEY6_CLICK_RELEASE)
            {
                if(stgMainMenuSelectedPos)
                {
                    Complete = 1;
                    timer = 0;

                    Device_FactoryReset();
                    hal_Oled_Clear();
                    hal_Oled_ShowString(16, 20, "Update..", 24, 1);
                    hal_Oled_Refresh();
                }
                else
                {
                    pModeMenu = pModeMenu->pParent;
                    pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
                }
            }
            else if(keys == KEY5_CLICK_RELEASE)
            {
                pModeMenu = pModeMenu->pParent;
                pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
            }
            else if(keys == KEY5_LONG_PRESS)
            {
                pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];
                pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
            }
        }
    }

    if(Complete)
    {
        timer++;

        if(timer > 150)
        {
            timer = 0;
            Complete = 0;
            pModeMenu = pModeMenu->pParent;
            pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
        }
    }
}


/*----------------------------------------------------------------------------
@Name		: S_ENArmModeProc()
@Function	: Away arm mode process
@Parameter	: Null
------------------------------------------------------------------------------*/
static void S_ENArmModeProc()
{
    unsigned char tBuff[3], id, dat;   
    Stru_DTC tStuDtc;                  

    static unsigned short time1;

    if(pStuSystemMode->refreshScreenCmd == SCREEN_CMD_RESET)
    {
        pStuSystemMode->refreshScreenCmd = SCREEN_CMD_NULL;
        pStuSystemMode->keyVal = 0xFF;
        
        hal_Oled_ClearArea(0,20,128,24); 
        hal_Oled_ClearArea(0,4,92,8);

        hal_Oled_ShowString(16,20,"Away arm",24,1);

        hal_Oled_Refresh();

        time1 = 0;
    }

    if(QueueDataLen(RFD_RxMsg))
    {
        QueueDataOut(RFD_RxMsg,&dat);

        if(dat == '#')
        {
            QueueDataOut(RFD_RxMsg,&tBuff[2]); // address high byte
            QueueDataOut(RFD_RxMsg,&tBuff[1]); // address low byte
            QueueDataOut(RFD_RxMsg,&tBuff[0]); // function code
            
            id = Device_DTCMatching(tBuff); 

            if(id != 0xFF)
            {
                Device_GetDTCStructure(&tStuDtc, id-1);
                
                if(tStuDtc.DTCType == DTC_REMOTE)
                {
                    if(tBuff[0] == SENSOR_CODE_REMOTE_DISARM)
                    {
                        SystemMode_Change(SYSTEM_MODE_DISARM);    
                    }
                    else if(tBuff[0] == SENSOR_CODE_REMOTE_HOMEARM)
                    {
                        SystemMode_Change(SYSTEM_MODE_HOMEARM);    
                    }
                    else if(tBuff[0] == SENSOR_CODE_REMOTE_SOS)
                    {
                        SystemMode_Change(SYSTEM_MODE_ALARM);    
                        
                        QueueDataIn(DtcTriggerIDMsg, &id, 1); 
                    }
                }
                else if(tBuff[0] == SENSOR_CODE_DOOR_OPEN) 
                {
                    SystemMode_Change(SYSTEM_MODE_ALARM);
                    
                    QueueDataIn(DtcTriggerIDMsg, &id, 1);
                }
                else if(tBuff[0]==SENSOR_CODE_DOOR_CLOSE)   
                {
                    time1 = 500;                          
                }

                if(time1 == 500) 
                {
                    hal_Oled_ClearArea(0,4,92,8); 
                    hal_Oled_ShowString(2,4,"Door:",8,1); 
                    hal_Oled_ShowString(32,4,tStuDtc.DeviceName,8,1);        
                    hal_Oled_Refresh();                                        
                }
            }
        }
    }

    if(time1)  
    {
        time1--; 
        if(time1 == 0)
        {
            hal_Oled_ClearArea(0,4,92,8);
            hal_Oled_Refresh();        
        }
    }
}

/*----------------------------------------------------------------------------
@Name		: S_DisArmModeProc()
@Function	: Disarm mode process
@Parameter	: Null
------------------------------------------------------------------------------*/
static void S_DisArmModeProc()
{
    unsigned char tBuff[3],id,dat;
    Stru_DTC tStuDtc;
    static unsigned short time1;

    if(pStuSystemMode->refreshScreenCmd == SCREEN_CMD_RESET)
    {
        pStuSystemMode->refreshScreenCmd = SCREEN_CMD_NULL;
        pStuSystemMode->keyVal = 0xFF;

        hal_Oled_ClearArea(0,20,128,24);             
        hal_Oled_ClearArea(0,4,92,8);               
        hal_Oled_ShowString(28,20,"Disarm",24,1); 
        
        hal_Oled_Refresh();
        
        time1 = 0;
    }
 
    if(QueueDataLen(RFD_RxMsg))
    {
        QueueDataOut(RFD_RxMsg,&dat);
        if(dat == '#')
        {
            QueueDataOut(RFD_RxMsg,&tBuff[2]);
            QueueDataOut(RFD_RxMsg,&tBuff[1]);
            QueueDataOut(RFD_RxMsg,&tBuff[0]);
            id = Device_DTCMatching(tBuff); 

            if(id != 0xFF)
            {
                Device_GetDTCStructure(&tStuDtc,id-1);
                 
                if(tStuDtc.DTCType == DTC_REMOTE)
                {
                    if(tBuff[0] == SENSOR_CODE_REMOTE_ENARM)
                    {
                        SystemMode_Change(SYSTEM_MODE_ENARM);        
                    }else if(tBuff[0] == SENSOR_CODE_REMOTE_DISARM)
                    {
                        SystemMode_Change(SYSTEM_MODE_DISARM);      
                    }else if(tBuff[0] == SENSOR_CODE_REMOTE_HOMEARM)
                    {
                        SystemMode_Change(SYSTEM_MODE_HOMEARM);      
                    }else if(tBuff[0] == SENSOR_CODE_REMOTE_SOS)
                    {
                        SystemMode_Change(SYSTEM_MODE_ALARM);     
                        QueueDataIn(DtcTriggerIDMsg, &id, 1);
                    }
                }
                else if(tBuff[0]==SENSOR_CODE_DOOR_OPEN)
                {
                    if(tStuDtc.ZoneType==ZONE_TYP_24HOURS)
                    {
                        SystemMode_Change(SYSTEM_MODE_ALARM);      
                        QueueDataIn(DtcTriggerIDMsg, &id, 1);
                    }
                    else
                    {
                        time1 = 500;
                    }
                }
                else if(tBuff[0]==SENSOR_CODE_DOOR_CLOSE)
                {
                    time1 = 500;                                        
                }

                if(time1 == 500)
                {
                    hal_Oled_ClearArea(0,4,92,8);       
                    hal_Oled_ShowString(2,4,"Door:",8,1);
                    hal_Oled_ShowString(32,4,tStuDtc.DeviceName,8,1);        
                    hal_Oled_Refresh();                                        
                }          
            }
                
        }
    }

    if(time1)
    {
        time1 --;
        if(time1 == 0)
        {
            hal_Oled_ClearArea(0,4,92,8); 
            hal_Oled_Refresh();        
        }
    }
}

/*----------------------------------------------------------------------------
@Name		: S_HomeArmModeProc()
@Function	: HomwArm mode process
@Parameter	: Null
------------------------------------------------------------------------------*/
static void S_HomeArmModeProc()
{
    unsigned char tBuff[3],id,dat;
    Stru_DTC tStuDtc;
    static unsigned short time1;
    if(pStuSystemMode->refreshScreenCmd == SCREEN_CMD_RESET)
    {
        pStuSystemMode->refreshScreenCmd = SCREEN_CMD_NULL;
        pStuSystemMode->keyVal = 0xFF;
        
        hal_Oled_ClearArea(0,20,128,24);              
        hal_Oled_ClearArea(0,4,92,8);                 
        hal_Oled_ShowString(16,20,"Home Arm",24,1);
        hal_Oled_Refresh();
        
        time1 = 0;         
    }
    
    if(QueueDataLen(RFD_RxMsg))
    {
        QueueDataOut(RFD_RxMsg,&dat);
        if(dat == '#')
        {
            QueueDataOut(RFD_RxMsg,&tBuff[2]);
            QueueDataOut(RFD_RxMsg,&tBuff[1]);
            QueueDataOut(RFD_RxMsg,&tBuff[0]);
            id = Device_DTCMatching(tBuff);      
            if(id != 0xFF)
            {
                Device_GetDTCStructure(&tStuDtc,id-1);
                 
                if(tStuDtc.DTCType == DTC_REMOTE)
                {
                    if(tBuff[0] == SENSOR_CODE_REMOTE_ENARM)
                    {
                        SystemMode_Change(SYSTEM_MODE_ENARM);      
                    }else if(tBuff[0] == SENSOR_CODE_REMOTE_DISARM)
                    {
                        SystemMode_Change(SYSTEM_MODE_DISARM); 
                    }else if(tBuff[0] == SENSOR_CODE_REMOTE_SOS)
                    {
                        SystemMode_Change(SYSTEM_MODE_ALARM);    
                        QueueDataIn(DtcTriggerIDMsg, &id, 1);
                    }
                }
                else if(tBuff[0]==SENSOR_CODE_DOOR_OPEN)
                {
                    if(tStuDtc.ZoneType != ZONE_TYP_2ND)
                    {
                        SystemMode_Change(SYSTEM_MODE_ALARM);
                        QueueDataIn(DtcTriggerIDMsg, &id, 1);
                    }
                    else
                    {
                        time1 = 500;
                    }
                        
                }else if(tBuff[0]==SENSOR_CODE_DOOR_CLOSE)
                {
                    time1 = 500;                                        
                }

                if(time1 == 500)
                {
                    hal_Oled_ClearArea(0,4,92,8);
                    hal_Oled_ShowString(2,4,"Door:",8,1);
                    hal_Oled_ShowString(32,4,tStuDtc.DeviceName,8,1); 
                    hal_Oled_Refresh();                                               
                } 
            }
        }
    }

    if(time1)
    {
        time1 --;
        if(time1 == 0)
        {
            hal_Oled_ClearArea(0,4,92,8); 
            hal_Oled_Refresh();        
        }
    }
}

/*----------------------------------------------------------------------------
@Name		: S_AlarmModeProc()
@Function	: Alarm mode process
@Parameter	: Null
------------------------------------------------------------------------------*/
static void S_AlarmModeProc()
{
    static unsigned short timer = 0;
    
    static unsigned short timer2 = 0;
    
    static unsigned char displayAlarmFlag = 1;
    
    unsigned char tBuff[3], id, dat;

    Stru_DTC tStuDtc;

    if(pStuSystemMode->refreshScreenCmd == SCREEN_CMD_RESET)
    {
        pStuSystemMode->refreshScreenCmd = SCREEN_CMD_NULL;

        pStuSystemMode->keyVal = 0xFF;
        
        hal_Oled_ClearArea(0,20,128,24); 
        hal_Oled_ClearArea(0,4,92,8); 
        
        hal_Oled_ShowString(16,20,"Alarming",24,1);
        
        hal_Oled_Refresh();
        
        displayAlarmFlag = 1;
        timer = 0;
        timer2 = 0;
    }

    if(QueueDataLen(RFD_RxMsg))
    {
        QueueDataOut(RFD_RxMsg,&dat);
        
        if(dat == '#')
        {
            QueueDataOut(RFD_RxMsg,&tBuff[2]); 
            QueueDataOut(RFD_RxMsg,&tBuff[1]); 
            QueueDataOut(RFD_RxMsg,&tBuff[0]); 
            
            id = Device_DTCMatching(tBuff); 

            if(id != 0xFF)
            {
                Device_GetDTCStructure(&tStuDtc, id-1);
          
                if(tStuDtc.DTCType == DTC_REMOTE) 
                {
                    if(tBuff[0] == SENSOR_CODE_REMOTE_DISARM)
                    {
                        SystemMode_Change(SYSTEM_MODE_DISARM);
                    }
                    else if(tBuff[0] == SENSOR_CODE_REMOTE_SOS) 
                    {
                        QueueDataIn(DtcTriggerIDMsg, &id, 1);
                    }
                }
                else if(tBuff[0]==SENSOR_CODE_DOOR_OPEN)  
                {
                    QueueDataIn(DtcTriggerIDMsg, &id, 1);
                }
            }
        }
    }

    if(QueueDataLen(DtcTriggerIDMsg) && (!timer))
    {
        QueueDataOut(DtcTriggerIDMsg,&id);

        if(id > 0)
        {
            Device_GetDTCStructure(&tStuDtc,id-1); 
            
            if(tStuDtc.DTCType == DTC_REMOTE)  
            {
                hal_Oled_ClearArea(0,4,92,8);             
                hal_Oled_ShowString(2,4,"Remote:",8,1);
                hal_Oled_ShowString(44,4,"sos",8,1);

                OneNet_UpEventQueue(UPDATA_ALARMINFO_REMOTE);
                OneNet_UpEventQueue((En_OneNetUpDatList)id); 
            }
            else if(tStuDtc.DTCType == DTC_DOOR)
            {
                hal_Oled_ClearArea(0,4,92,8);            
                hal_Oled_ShowString(2,4,"Door:",8,1);
                hal_Oled_ShowString(32,4,tStuDtc.DeviceName,8,1); 

                OneNet_UpEventQueue(UPDATA_ALARMINFO_DOOR);   
                OneNet_UpEventQueue((En_OneNetUpDatList)id); 
            }

            hal_Oled_Refresh();

            timer = 1;
        }
    }
    
    if(timer)
    {
        timer++;

        if(timer > 500)
        {
            timer = 0;

            hal_Oled_ClearArea(0,4,92,8);        
            hal_Oled_Refresh();
        }
    }
    
    timer2++;

    if(timer2 > 49)
    {
        timer2 = 0;
        
        displayAlarmFlag = !displayAlarmFlag;
        
        if(displayAlarmFlag)
        {
            hal_Oled_ShowString(16,20,"Alarming",24,1);
            
            hal_Oled_Refresh();

        }
        else
        {
            hal_Oled_ClearArea(0,20,128,24); 

            hal_Oled_Refresh();
        }
    }
}

/*----------------------------------------------------------------------------
@Name		: SystemMode_Change()
@Function	: change WorkingMode
@Parameter	: 
        --> sysMode: target working mode 
------------------------------------------------------------------------------*/
static void SystemMode_Change(SYSTEMMODE_TYPEDEF sysMode)
{
    if(sysMode < SYSTEM_MODE_SUM) 
    {
        pStuSystemMode = &stu_Sysmode[sysMode]; 
        
        pStuSystemMode->refreshScreenCmd = SCREEN_CMD_RESET; 
      
        OneNet_UpEventQueue((En_OneNetUpDatList)sysMode);

        ScreenControl(1);

        if(sysMode == SYSTEM_MODE_ALARM) 
        {   
            Hal_Beep_PWMCtrl(1);
        }
        else
        {
            if(sysMode == SYSTEM_MODE_DISARM)
            {
                Hal_LED_MsgInput(BUZ, LED_ON_100MS, 1);
            }
            Hal_LED_MsgInput(BUZ, LED_ON_100MS, 0);

            Hal_Beep_PWMCtrl(0);
        } 
    }
}



/*----------------------------------------------------------------------------
@Name		: ScreenControl(unsigned char cmd)
@Function	: Screen Control(on/off)
@Parameter	: 
        --> cmd: 0->off, 1->on
------------------------------------------------------------------------------*/
static void ScreenControl(unsigned char cmd)
{
    if(cmd)
    {
        if(!ScreenState)
        {
            ScreenState = 1;
        }

        hal_Oled_Display_on();

        PutoutScreenTiemr = 0;        
    }
    else
    {
        if(ScreenState)
        {
            ScreenState = 0;

            hal_Oled_Display_Off();

            PutoutScreenTiemr = 0;
        }    
    }
}

/***********************************************************************
@Name       : HexToAscii()
@Function   : transfer a Hex string to a corresponding Ascii string
@Parameter  : pHex, pAscii, nLen
@Note       : result ASCII  is all upper case
************************************************************************/
static void HexToAscii(unsigned char *pHex, unsigned char *pAscii, int nLen)
{
    unsigned char Nibble[2];
    unsigned int i,j;
    for (i = 0; i < nLen; i++){
        Nibble[0] = (pHex[i] & 0xF0) >> 4;
        Nibble[1] = pHex[i] & 0x0F;
        for (j = 0; j < 2; j++){
            if (Nibble[j] < 10){            
                Nibble[j] += 0x30;
            }
            else{
                if (Nibble[j] < 16)
                    Nibble[j] = Nibble[j] - 10 + 'A';
            }
            *pAscii++ = Nibble[j];
        }             
    }          
}

/*----------------------------------------------------------------------------
@Name		: App_Pro()
@Function	: App polling function
@Parameter	: Null
------------------------------------------------------------------------------*/
void App_Pro(void)
{
    if(pModeMenu->menuPos != DESKTOP_MENU_POS) 
    {
        SetupMenuTimeOutCnt++;

        if(SetupMenuTimeOutCnt > SETUPMENU_TIMEOUT_PERIOD)
        {
            SetupMenuTimeOutCnt = 0;
            pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP]; 
            pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET; 
        }
    }
    
	pModeMenu->action();

    if(pStuSystemMode->ID!=SYSTEM_MODE_ALARM)
        {
            PutoutScreenTiemr++;
            if(PutoutScreenTiemr > PUTOUT_SCREEN_PERIOD)
            {
                PutoutScreenTiemr = 0;
                ScreenControl(0);     
            }
        }
}

/*---------------------------Event handler----------------------------------*/
static void KeyEventHandler(KEY_VALUE_TYPEDEF keys)
{
    if(!ScreenState)
    {
        ScreenControl(1);
    }
    else
    {
        if(pModeMenu)
        {
            pModeMenu->keyVal = keys;

            if(pModeMenu->menuPos != DESKTOP_MENU_POS) 
            {   
                SetupMenuTimeOutCnt = 0; 
            }

            PutoutScreenTiemr = 0;
        }
    }    
	
	if((keys==KEY1_CLICK)
	|| (keys==KEY2_CLICK)
	|| (keys==KEY3_CLICK)
	|| (keys==KEY4_CLICK)
	|| (keys==KEY5_CLICK)
	|| (keys==KEY6_CLICK))
    {

    }

}

static void RFDRxHandler(unsigned char *pBuff)
{
	unsigned char temp;
	unsigned char RFDBuff[3];
	
	RFDBuff[0] = pBuff[0];
	RFDBuff[1] = pBuff[1];
	RFDBuff[2] = pBuff[2];
	RFDBuff[2] &= 0x0F; 		
	
	temp = '#';
	QueueDataIn(RFD_RxMsg, &temp, 1);
	QueueDataIn(RFD_RxMsg, &RFDBuff[0], 3);
}


static void ServerEventHandle(en_NBIot_MSG_TYPE type, unsigned char *pData)
{
    switch(type)
    {
        case NBIOT_HOST_STATE:      
        {
            if(*pData == ONENET_DOWNDATA_OPER_AWAYARM)
            {
                SystemMode_Change(SYSTEM_MODE_ENARM);   
            }
            else if(*pData == ONENET_DOWNDATA_OPER_HOMEARM)
            {
                SystemMode_Change(SYSTEM_MODE_HOMEARM); 
            }
            else if(*pData == ONENET_DOWNDATA_OPER_DISARM)
            {
                 SystemMode_Change(SYSTEM_MODE_DISARM); 
            }
        }
        break;

        case NBIOT_TIME: 
        {
            //2024-04-09 wes 17:22
            stuSystemtime.year=2000+pData[0];   
            stuSystemtime.mon=pData[1];         
            stuSystemtime.day=pData[2];         
            stuSystemtime.hour=pData[3];       
            stuSystemtime.min=pData[4];        
            stuSystemtime.sec=pData[5];        
        }        
        break;

        case NBIOT_CONNECT_STATE:   
        {
            NbIotWorkState = *pData; 
        }
        break;

        case NBIOT_CSQ:
        {
            if(*pData < 4) 
            {
                NbIotCsQ = NBIOT_CSQL_0; 
            }
            else if(*pData < 8) 
            {
                NbIotCsQ = NBIOT_CSQL_1; 
            }
            else if(*pData < 13)
            {
                NbIotCsQ = NBIOT_CSQL_2;
            }        
            else if(*pData < 18)
            {
                NbIotCsQ = NBIOT_CSQL_3;
            }        
            else if(*pData < 32)
            {
                NbIotCsQ = NBIOT_CSQL_4;
            }
            else
            {
                NbIotCsQ = NBIOT_CSQL_UNKUOW; 
            }    
        }
        break;
    }
}
