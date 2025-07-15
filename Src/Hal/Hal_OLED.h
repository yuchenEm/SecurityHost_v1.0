#ifndef __HAL_OLED_H_
#define __HAL_OLED_H_

#define OLED_CMD  0	// write command
#define OLED_DATA 1	// write data

#define OLED_PORT GPIOA

#define OLED_SLK_PORT  OLED_PORT
#define OLED_SLK_PIN	GPIO_Pin_5

#define OLED_DO_PORT	OLED_PORT
#define OLED_DO_PIN	GPIO_Pin_7

#define OLED_RES_PORT	OLED_PORT
#define OLED_RES_PIN	GPIO_Pin_4

#define OLED_CMD_PORT	OLED_PORT
#define OLED_CMD_PIN GPIO_Pin_6

#define OLED_SCL_Clr() GPIO_ResetBits(OLED_SLK_PORT,OLED_SLK_PIN)	//SCL
#define OLED_SCL_Set() GPIO_SetBits(OLED_SLK_PORT,OLED_SLK_PIN)

#define OLED_SDA_Clr() GPIO_ResetBits(OLED_DO_PORT,OLED_DO_PIN)		//SDA
#define OLED_SDA_Set() GPIO_SetBits(OLED_DO_PORT,OLED_DO_PIN)

#define OLED_RES_Clr() GPIO_ResetBits(OLED_RES_PORT,OLED_RES_PIN)	//RES
#define OLED_RES_Set() GPIO_SetBits(OLED_RES_PORT,OLED_RES_PIN)

#define OLED_DC_Clr()  GPIO_ResetBits(OLED_CMD_PORT,OLED_CMD_PIN)	//CMD
#define OLED_DC_Set()  GPIO_SetBits(OLED_CMD_PORT,OLED_CMD_PIN)

enum
{
    ICON_NO_SIM,            // no SIM Card
    ICON_SIGNAL_L00,        // Signal level_0
    ICON_SIGNAL_L01,        // Signal level_1
    ICON_SIGNAL_L02,        // Signal level_2
    ICON_SIGNAL_L03,        // Signal level_3
    ICON_SIGNAL_L04,        // Signal level_4
    ICON_ONENET_STA_BREAK,  // server connect fail
    ICON_ONENET_STA_LINK,   // server connect success

    ICON_MAX,
};


void hal_OledInit(void);
void hal_Oled_Color_Turn(unsigned char i);
void hal_Oled_Display_Turn(unsigned char i);
void hal_Oled_Display_on(void);
void hal_Oled_Display_Off(void);
void hal_Oled_Refresh(void);
void hal_Oled_Clear(void);
static void hal_Oled_DrawPoint(unsigned char x,unsigned char y,unsigned char t);
void hal_Oled_DrawLine(unsigned char x1,unsigned char y1,unsigned char x2,unsigned char y2,unsigned char mode);
void hal_Oled_DrawCircle(unsigned char x,unsigned char y,unsigned char r);
void hal_Oled_ShowChar(unsigned char x,unsigned char y,unsigned char chr,unsigned char size1,unsigned char mode);
void hal_Oled_ShowString(unsigned char x,unsigned char y,unsigned char *chr,unsigned char size1,unsigned char mode);
void hal_Oled_ShowNum(unsigned char x,unsigned char y,unsigned int num,unsigned char len,unsigned char size1,unsigned char mode);
void hal_Oled_ShowChinese(unsigned char x,unsigned char y,unsigned char num,unsigned char size1,unsigned char mode);
void hal_Oled_ScrollDisplay(unsigned char num,unsigned char space,unsigned char mode);
void hal_Oled_ShowPicture(unsigned char x,unsigned char y,unsigned char sizex,unsigned char sizey,const unsigned char BMP[],unsigned char mode);
void hal_Oled_ClearArea(unsigned char x,unsigned char y,unsigned char sizex,unsigned char sizey);
void hal_Oled_inco(unsigned char loc, unsigned char fuc);


#endif
