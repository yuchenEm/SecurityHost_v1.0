#ifndef __OS_SYSTEM_H_
#define __OS_SYSTEM_H_

extern void S_QueueEmpty(unsigned char **Head, unsigned char **Tail, unsigned char *HBuff);
extern void S_QueueDataIn(unsigned char **Head, unsigned char **Tail, unsigned char *HBuff, unsigned short Len, unsigned char *HData, unsigned short DataLen);
extern unsigned char S_QueueDataOut(unsigned char **Head, unsigned char **Tail, unsigned char *HBuff, unsigned short Len, unsigned char *Data);
extern unsigned short S_QueueDataLen(unsigned char **Head, unsigned char **Tail, unsigned short Len);
 
 
#define QueueEmpty(x)	   S_QueueEmpty((unsigned char**)&(x).Head,(unsigned char**)&(x).Tail,(unsigned char*)(x).Buff) 
#define QueueDataIn(x,y,z) S_QueueDataIn((unsigned char**)&(x).Head,(unsigned char**)&(x).Tail,(unsigned char*)(x).Buff,sizeof((x).Buff),(y),(z))
#define QueueDataOut(x,y)  S_QueueDataOut((unsigned char**)&(x).Head,(unsigned char**)&(x).Tail,(unsigned char*)(x).Buff,sizeof((x).Buff),(y)) 
#define QueueDataLen(x)	   S_QueueDataLen((unsigned char**)&(x).Head,(unsigned char**)&(x).Tail,sizeof((x).Buff))  


/* */
typedef struct
{
	unsigned char *Head; 
	unsigned char *Tail; 
	unsigned char Buff[4+1];
}Queue4;
typedef struct{unsigned char *Head; unsigned char *Tail; unsigned char Buff[8+1];}    Queue8;
typedef struct{unsigned char *Head; unsigned char *Tail; unsigned char Buff[16+1];}   Queue16; 
typedef struct{unsigned char *Head; unsigned char *Tail; unsigned char Buff[32+1];}   Queue32;
typedef struct{unsigned char *Head; unsigned char *Tail; unsigned char Buff[64+1];}   Queue64;
typedef struct{unsigned char *Head; unsigned char *Tail; unsigned char Buff[90+1];}   Queue90;
typedef struct{unsigned char *Head; unsigned char *Tail; unsigned char Buff[120+1];}  Queue120;
typedef struct{unsigned char *Head; unsigned char *Tail; unsigned char Buff[340+1];}  Queue340;
typedef struct{unsigned char *Head; unsigned char *Tail; unsigned char Buff[745+1];}  Queue745;
typedef struct{unsigned char *Head; unsigned char *Tail; unsigned char Buff[128+1];}  Queue128;
typedef struct{unsigned char *Head; unsigned char *Tail; unsigned char Buff[248+1];}  Queue248;
typedef struct{unsigned char *Head; unsigned char *Tail; unsigned char Buff[256+1];}  Queue256;
typedef struct{unsigned char *Head; unsigned char *Tail; unsigned char Buff[512+1];}  Queue512;
typedef struct{unsigned char *Head; unsigned char *Tail; unsigned char Buff[1024+1];} Queue1K;
typedef struct{unsigned char *Head; unsigned char *Tail; unsigned char Buff[2048+1];} Queue2K;
typedef struct{unsigned char *Head; unsigned char *Tail; unsigned char Buff[5120+1];} Queue5K;
typedef struct{unsigned char *Head; unsigned char *Tail; unsigned char Buff[10240+1];} Queue10K;



typedef enum
{
	CPU_ENTER_CRITICAL,		//CPU enter critical
	CPU_EXIT_CRITICAL,		//CPU exit critical
}CPU_EA_TYPEDEF;

// define a CPU interrupt control call-back function pointer: CPUInterrupt_CallBack_t,
typedef void (*CPUInterrupt_CallBack_t)(CPU_EA_TYPEDEF cmd,unsigned char *pSta);


// task ID
typedef enum
{
	OS_TASK1, 	// 0
	OS_TASK2,
	OS_TASK3,
	OS_TASK4,
	OS_TASK5,
	OS_TASK6,
	
	OS_TASK_SUM	// trick to count number of enum members
}OS_TaskIDTypeDef;


// system running state
typedef enum
{
	OS_SLEEP,			// sleep
	OS_RUN=!OS_SLEEP	// run
}OS_TaskStatusTypeDef;

// system task structure
typedef struct
{
	void (*task)(void);					// task function pointer
	OS_TaskStatusTypeDef RunFlag;		// task running state
	unsigned short	RunPeriod;			// task handler run time
	unsigned short RunTimer;			// task handler timer
}OS_TaskTypeDef;

 
/*******************************************************************************/
void OS_CPUInterruptCBSRegister(CPUInterrupt_CallBack_t pCPUInterruptCtrlCBS);
void OS_ClockInterruptHandle(void);
void OS_TaskInit(void);
void OS_CreatTask(unsigned char ID, void (*proc)(void), unsigned short TimeDly, OS_TaskStatusTypeDef flag);
void OS_Start(void);
void OS_TaskGetUp(OS_TaskIDTypeDef taskID);	
void OS_TaskSleep(OS_TaskIDTypeDef taskID);

#endif
