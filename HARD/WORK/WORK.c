#include "WORK.h"
#include "SYS.h"
#include "delay.h"

void LED_IO_INIT(void)
{
    GPIO_InitTypeDef  GPIO_LEDinit;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA , ENABLE);
	
	 // GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE); 

	
	  GPIO_LEDinit.GPIO_Pin=GPIO_Pin_8;
	  GPIO_LEDinit.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_LEDinit.GPIO_Mode=GPIO_Mode_Out_PP;
	  GPIO_Init(GPIOA,&GPIO_LEDinit);

		PAout(8) = 0;
}
#if 0
void TIME4_init(u16 arr,u16 psc)                      //定时器配置
{
	   TIM_TimeBaseInitTypeDef  TIM_init;
	   NVIC_InitTypeDef  NVIC_init;
	   
	   RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,ENABLE);
	   
	   TIM_init.TIM_Period=arr;
	   TIM_init.TIM_Prescaler =psc;
	   TIM_init.TIM_ClockDivision=TIM_CKD_DIV1;
	   TIM_init.TIM_CounterMode=TIM_CounterMode_Up;
	   TIM_TimeBaseInit(TIM4,&TIM_init);
	
	   TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
	
	   NVIC_init.NVIC_IRQChannel=TIM4_IRQn;
	   NVIC_init.NVIC_IRQChannelCmd=ENABLE;
	   NVIC_init.NVIC_IRQChannelPreemptionPriority=0;
	   NVIC_init.NVIC_IRQChannelSubPriority=0;
	   NVIC_Init(&NVIC_init);
	
	   TIM_Cmd(TIM4,ENABLE);
}

void TIM4_IRQHandler(void)                           //定时器中断函数
{
	   if(TIM_GetITStatus(TIM4,TIM_IT_Update)==1)
	   {
		     PBout(8)=!PBout(8);
			   //PBout(12)=!PBout(12);
			   //delay_ms(200);
			   //PBout(8)=1;
	   }
	   TIM_ClearITPendingBit(TIM4,TIM_IT_Update);
}
#endif

#define STM32_FLASH_SIZE 	64 	 		//所选STM32的FLASH容量大小(单位为K)
#define STM32_FLASH_WREN 	1              	//使能FLASH写入(0，不是能;1，使能)
#define FLASH_WAITETIME  	50000          	//FLASH等待超时时间

//FLASH起始地址
#define STM32_FLASH_BASE 0x08000000 		//STM32 FLASH的起始地址

void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead);

//读取指定地址的半字(16位数据)
//faddr:读地址(此地址必须为2的倍数!!)
//返回值:对应数据.
u16 STMFLASH_ReadHalfWord(u32 faddr)
{
	return *(vu16*)faddr; 
}
#if STM32_FLASH_WREN	//如果使能了写   
//不检查的写入
//WriteAddr:起始地址
//pBuffer:数据指针
//NumToWrite:半字(16位)数   
void STMFLASH_Write_NoCheck(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite)   
{ 			 		 
	u16 i;
	for(i=0;i<NumToWrite;i++)
	{
		FLASH_ProgramHalfWord(WriteAddr,pBuffer[i]);
	    WriteAddr+=2;//地址增加2.
	}  
} 
//从指定地址开始写入指定长度的数据
//WriteAddr:起始地址(此地址必须为2的倍数!!)
//pBuffer:数据指针
//NumToWrite:半字(16位)数(就是要写入的16位数据的个数.)
#if STM32_FLASH_SIZE<256
#define STM_SECTOR_SIZE 1024 //字节
#else 
#define STM_SECTOR_SIZE	2048
#endif		 
u16 STMFLASH_BUF[STM_SECTOR_SIZE/2];//最多是2K字节
void STMFLASH_Write(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite)	
{
	u32 secpos;	   //扇区地址
	u16 secoff;	   //扇区内偏移地址(16位字计算)
	u16 secremain; //扇区内剩余地址(16位字计算)	   
 	u16 i;    
	u32 offaddr;   //去掉0X08000000后的地址
	if(WriteAddr<STM32_FLASH_BASE||(WriteAddr>=(STM32_FLASH_BASE+1024*STM32_FLASH_SIZE)))return;//非法地址
	FLASH_Unlock();						//解锁
	offaddr=WriteAddr-STM32_FLASH_BASE;		//实际偏移地址.
	secpos=offaddr/STM_SECTOR_SIZE;			//扇区地址  0~127 for STM32F103RBT6
	secoff=(offaddr%STM_SECTOR_SIZE)/2;		//在扇区内的偏移(2个字节为基本单位.)
	secremain=STM_SECTOR_SIZE/2-secoff;		//扇区剩余空间大小   
	if(NumToWrite<=secremain)secremain=NumToWrite;//不大于该扇区范围
	while(1) 
	{	
		STMFLASH_Read(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE,STMFLASH_BUF,STM_SECTOR_SIZE/2);//读出整个扇区的内容
		for(i=0;i<secremain;i++)//校验数据
		{
			if(STMFLASH_BUF[secoff+i]!=0XFFFF)break;//需要擦除  	  
		}
		if(i<secremain)//需要擦除
		{
			FLASH_ErasePage(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE);//擦除这个扇区
			for(i=0;i<secremain;i++)//复制
			{
				STMFLASH_BUF[i+secoff]=pBuffer[i];	  
			}
			STMFLASH_Write_NoCheck(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE,STMFLASH_BUF,STM_SECTOR_SIZE/2);//写入整个扇区  
		}else STMFLASH_Write_NoCheck(WriteAddr,pBuffer,secremain);//写已经擦除了的,直接写入扇区剩余区间. 				   
		if(NumToWrite==secremain)break;//写入结束了
		else//写入未结束
		{
			secpos++;				//扇区地址增1
			secoff=0;				//偏移位置为0 	 
		   	pBuffer+=secremain;  	//指针偏移
			WriteAddr+=secremain;	//写地址偏移	   
		   	NumToWrite-=secremain;	//字节(16位)数递减
			if(NumToWrite>(STM_SECTOR_SIZE/2))secremain=STM_SECTOR_SIZE/2;//下一个扇区还是写不完
			else secremain=NumToWrite;//下一个扇区可以写完了
		}	 
	};	
	FLASH_Lock();//上锁
}
#endif

//从指定地址开始读出指定长度的数据
//ReadAddr:起始地址
//pBuffer:数据指针
//NumToWrite:半字(16位)数
void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead)   	
{
	u16 i;
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i]=STMFLASH_ReadHalfWord(ReadAddr);//读取2个字节.
		ReadAddr+=2;//偏移2个字节.	
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//WriteAddr:起始地址
//WriteData:要写入的数据
void STMFLASH_WriteHalfWord(u32 WriteAddr,u16 WriteData)   	
{
	STMFLASH_Write(WriteAddr,&WriteData,1);//写入一个字 
}


/*
看门狗超时时间(40kHz的输入时钟(LSI)) (1)
预分频系数  PR[2:0]位   RL[11:0] = 0x000  RL[11:0] = 0xFFF
/4          0           0.1                 409.6
/8          1           0.2                 819.2
/16         2           0.4                 1638.4
/32         3           0.8                 3276.8
/64         4           1.6                 6553.6
/128        5           3.2                 13107.2
/256        (6或7)      6.4                 26214.4

*/
void iwdg_init(void)
{
    if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)
    {
      /* Clear reset flags */
      RCC_ClearFlag();
    }

    /* Enable the LSI OSC */
    RCC_LSICmd(ENABLE);

    /* Wait till LSI is ready */
    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
    {}

    /* Enable write access to IWDG_PR and IWDG_RLR registers */
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

    /* IWDG counter clock: LSI/32 */
    IWDG_SetPrescaler(IWDG_Prescaler_128);

    IWDG_SetReload(0xFFF);

    /* Reload IWDG counter */
    IWDG_ReloadCounter();

    /* Enable IWDG (the LSI oscillator will be enabled by hardware) */
    IWDG_Enable();
}




