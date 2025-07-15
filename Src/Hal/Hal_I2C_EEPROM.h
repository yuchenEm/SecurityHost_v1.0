#ifndef __HAL_I2C_EEPROM_H_
#define __HAL_I2C_EEPROM_H_

#define I2C_SCL_PORT	GPIOB
#define I2C_SCL_PIN		GPIO_Pin_8

#define I2C_SDA_PORT	GPIOB
#define I2C_SDA_PIN		GPIO_Pin_9

void Hal_I2C_EEPROM_Init(void);
void Hal_I2C_EEPROM_ByteWrite(unsigned short address, unsigned char Data);
unsigned char Hal_I2C_EEPROM_RandomRead(unsigned short address);
void Hal_I2C_EEPROM_PageWrite(unsigned short address, unsigned char *pDat, unsigned short Num);
void Hal_I2C_EEPROM_SequentialRead(unsigned short address, unsigned char *pBuffer, unsigned short Num);

#endif
