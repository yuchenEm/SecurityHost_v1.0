#ifndef __APP_H_
#define __APP_H_

// Doorsensor code
#define SENSOR_CODE_DOOR_OPEN           0x0A          // door open       
#define SENSOR_CODE_DOOR_CLOSE          0x0E          // door close
#define SENSOR_CODE_DOOR_TAMPER         0x07          // tamper
#define SENSOR_CODE_DOOR_LOWPWR         0x06          // voltage low

// remote code
#define SENSOR_CODE_REMOTE_ENARM        0x02         
#define SENSOR_CODE_REMOTE_DISARM       0x01                 
#define SENSOR_CODE_REMOTE_HOMEARM      0x04
#define SENSOR_CODE_REMOTE_SOS          0x08

#define PUTOUT_SCREEN_PERIOD            3000        

#define SETUPMENU_TIMEOUT_PERIOD        2000       

// Screen command
typedef enum
{
	SCREEN_CMD_NULL,        
    SCREEN_CMD_RESET,      	
    SCREEN_CMD_RECOVER,  	
    SCREEN_CMD_UPDATE,  	
}SCREEN_CMD;              	


typedef enum
{
    DESKTOP_MENU_POS, 		
    STG_MENU_POS,     		
    STG_WIFI_MENU_POS, 		
    STG_SUB_2_MENU_POS, 	
    STG_SUB_3_MENU_POS, 	
    STG_SUB_4_MENU_POS, 	
}MENU_POS;

typedef enum
{
    GNL_MENU_DESKTOP,   
	
    GNL_MENU_SUM,		
}GENERAL_MENU_LIST;    

typedef enum
{
    STG_MENU_MAIN_SETTING,		//0
    STG_MENU_LEARNING_SENSOR,	//1
    STG_MENU_DTC_LIST,			//2
    STG_MENU_MACHINE_INFO,		//3
    STG_MENU_FACTORY_SETTINGS,	//4
	
    STG_MENU_SUM				//5
}STG_MENU_LIST;

// DTC List->Zone xxx->Review
typedef enum
{
    STG_MENU_DL_ZX_REVIEW_MAIN,
    
    STG_MENU_DL_ZX_REVIEW,
    
    STG_MENU_DL_ZX_EDIT,
    
    STG_MENU_DL_ZX_DELETE,
    
    STG_MENU_DL_ZX_SUM,
} STG_MENU_DZ_ZX_LIST;

typedef enum 
{
    SYSTEM_MODE_ENARM,        
    SYSTEM_MODE_HOMEARM,      
    SYSTEM_MODE_DISARM,       
    SYSTEM_MODE_ALARM,      

    SYSTEM_MODE_SUM          
}SYSTEMMODE_TYPEDEF; 

typedef struct MODE_MENU
{
    unsigned char ID;        		// Menu ID
    MENU_POS menuPos;       		// Current Menu Position
    unsigned char *pModeType; 		// Menu name pointer
    void (*action)(void); 			// Function pointer
    SCREEN_CMD refreshScreenCmd; 	// Screen command
    unsigned char reserved;  		
    unsigned char keyVal;    		// KeyValue, 0xFF->no action
    struct MODE_MENU *pLast;  		
    struct MODE_MENU *pNext;  		
    struct MODE_MENU *pParent; 		// point to the parent menu                     
    struct MODE_MENU *pChild;   	// point to the child menu
}stu_mode_menu;

typedef struct SYSTEM_MODE
{
    SYSTEMMODE_TYPEDEF ID;          
    SCREEN_CMD refreshScreenCmd;    
    unsigned char keyVal;           
    void (*action)(void);           
}stu_system_mode;

typedef struct SYSTEM_TIME
{
    unsigned short year;		
    unsigned char mon;    		
    unsigned char day;   		
    unsigned char week;   
    unsigned char hour;    		
    unsigned char min;    		
    unsigned char sec;       	
}stu_system_time;

void App_Init(void);
void App_Pro(void);


#endif
