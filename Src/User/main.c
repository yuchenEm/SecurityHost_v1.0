/**
  * SecurityHost_v1.0/main.c
  *
  * Copyright (c) 2025 yuchenEm
  * Licensed under the MIT License
  */

#include "stm32f10x.h"
#include "hal_led.h"
#include "hal_timer.h"
#include "hal_cpu.h"
#include "hal_key.h"
#include "hal_rfd.h"
#include "hal_usart.h"
#include "os_system.h"
#include "app.h"
#include "hal_nbiot.h"

int main()
{
	Hal_CPU_Init(); 	
	OS_TaskInit();		
	Hal_Timer_Init(); 	
	
	Hal_LED_Init();		
	OS_CreatTask(OS_TASK1, Hal_LED_Pro, 1, OS_RUN); // Systick=10ms; runperiod=10ms
	
	Hal_Key_Init(); 	
	OS_CreatTask(OS_TASK2, Hal_Key_Pro, 1, OS_RUN);
	
	Hal_RFD_Init();		
	OS_CreatTask(OS_TASK3,Hal_RFD_Pro, 1, OS_RUN);
	
	Hal_USART_Init();	
	OS_CreatTask(OS_TASK4,Hal_USART_Pro, 1, OS_RUN);
	
	Hal_NBIOT_Init();
	OS_CreatTask(OS_TASK5, Hal_NBIOT_Pro, 1, OS_RUN);

	App_Init(); 		
	OS_CreatTask(OS_TASK6, App_Pro, 1, OS_RUN);
	
	/* Start scheduler*/
	OS_Start();
	
}
