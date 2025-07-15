#include "stm32f10x.h"
#include "os_system.h"
#include "hal_cpu.h"

static void Hal_CoreClock_Init(void);
static void Hal_CPU_Critical_Control(CPU_EA_TYPEDEF cmd, unsigned char *pSta);


/**************************************************************************
	@Name		: Hal_Get_Interrupt_State
	@Function	: get CPU interrupt status
	@Return		: 0: interrupt is off; 1: interrupt is on
***************************************************************************/
static unsigned char Hal_Get_Interrupt_State(void)
{
	return(!__get_PRIMASK());
	/*
	__get_PRIMASK() will return (__regPriMask)
	__regPriMask: Bit[0] of priority mask register(Exception Mask Registers)
	Bit[0]=1: any interrupts priority>0 will be masked(only Reset, NMI, Hardfault can respond)
	
	Exception Mask Registers: PRIMASK, FAULTMASK, BASEPRI
	*/
}


/**************************************************************************
	@Name		: Hal_CPU_Critical_Control
	@Function	: CPU eadge condition handler
	@cmd		: control command, @*psta: interrupt condition
***************************************************************************/
static void Hal_CPU_Critical_Control(CPU_EA_TYPEDEF cmd, unsigned char *pSta)
{
	if(cmd == CPU_ENTER_CRITICAL)
	{
		*pSta = Hal_Get_Interrupt_State();	// save interrupt status
		__disable_irq();		// turn off interrupt
	}
	else if(cmd == CPU_EXIT_CRITICAL)
	{
		if(*pSta)
		{
			__enable_irq();		// turn on interrupt
		}
		else 
		{
			__disable_irq();	// turn off interrupt
		}
	}
}

void Hal_CPU_Init(void)
{
	Hal_CoreClock_Init();
	OS_CPUInterruptCBSRegister(Hal_CPU_Critical_Control);
}

static void Hal_CoreClock_Init(void)
{
	SysTick_Config(SystemCoreClock / 100); // 10ms
}


/*--------------------------------------------------------------------------
	@Name		: SysTick_Handler()
	@Function	: 
--------------------------------------------------------------------------*/
void SysTick_Handler(void)
{
	OS_ClockInterruptHandle();
}
