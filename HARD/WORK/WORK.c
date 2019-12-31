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

void TIME4_init(u16 arr,u16 psc)                      //��ʱ������
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

void TIM4_IRQHandler(void)                           //��ʱ���жϺ���
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

#define STM32_FLASH_SIZE 	64 	 		//��ѡSTM32��FLASH������С(��λΪK)
#define STM32_FLASH_WREN 	1              	//ʹ��FLASHд��(0��������;1��ʹ��)
#define FLASH_WAITETIME  	50000          	//FLASH�ȴ���ʱʱ��

//FLASH��ʼ��ַ
#define STM32_FLASH_BASE 0x08000000 		//STM32 FLASH����ʼ��ַ

void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead);

//��ȡָ����ַ�İ���(16λ����)
//faddr:����ַ(�˵�ַ����Ϊ2�ı���!!)
//����ֵ:��Ӧ����.
u16 STMFLASH_ReadHalfWord(u32 faddr)
{
	return *(vu16*)faddr; 
}
#if STM32_FLASH_WREN	//���ʹ����д   
//������д��
//WriteAddr:��ʼ��ַ
//pBuffer:����ָ��
//NumToWrite:����(16λ)��   
void STMFLASH_Write_NoCheck(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite)   
{ 			 		 
	u16 i;
	for(i=0;i<NumToWrite;i++)
	{
		FLASH_ProgramHalfWord(WriteAddr,pBuffer[i]);
	    WriteAddr+=2;//��ַ����2.
	}  
} 
//��ָ����ַ��ʼд��ָ�����ȵ�����
//WriteAddr:��ʼ��ַ(�˵�ַ����Ϊ2�ı���!!)
//pBuffer:����ָ��
//NumToWrite:����(16λ)��(����Ҫд���16λ���ݵĸ���.)
#if STM32_FLASH_SIZE<256
#define STM_SECTOR_SIZE 1024 //�ֽ�
#else 
#define STM_SECTOR_SIZE	2048
#endif		 
u16 STMFLASH_BUF[STM_SECTOR_SIZE/2];//�����2K�ֽ�
void STMFLASH_Write(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite)	
{
	u32 secpos;	   //������ַ
	u16 secoff;	   //������ƫ�Ƶ�ַ(16λ�ּ���)
	u16 secremain; //������ʣ���ַ(16λ�ּ���)	   
 	u16 i;    
	u32 offaddr;   //ȥ��0X08000000��ĵ�ַ
	if(WriteAddr<STM32_FLASH_BASE||(WriteAddr>=(STM32_FLASH_BASE+1024*STM32_FLASH_SIZE)))return;//�Ƿ���ַ
	FLASH_Unlock();						//����
	offaddr=WriteAddr-STM32_FLASH_BASE;		//ʵ��ƫ�Ƶ�ַ.
	secpos=offaddr/STM_SECTOR_SIZE;			//������ַ  0~127 for STM32F103RBT6
	secoff=(offaddr%STM_SECTOR_SIZE)/2;		//�������ڵ�ƫ��(2���ֽ�Ϊ������λ.)
	secremain=STM_SECTOR_SIZE/2-secoff;		//����ʣ��ռ��С   
	if(NumToWrite<=secremain)secremain=NumToWrite;//�����ڸ�������Χ
	while(1) 
	{	
		STMFLASH_Read(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE,STMFLASH_BUF,STM_SECTOR_SIZE/2);//������������������
		for(i=0;i<secremain;i++)//У������
		{
			if(STMFLASH_BUF[secoff+i]!=0XFFFF)break;//��Ҫ����  	  
		}
		if(i<secremain)//��Ҫ����
		{
			FLASH_ErasePage(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE);//�����������
			for(i=0;i<secremain;i++)//����
			{
				STMFLASH_BUF[i+secoff]=pBuffer[i];	  
			}
			STMFLASH_Write_NoCheck(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE,STMFLASH_BUF,STM_SECTOR_SIZE/2);//д����������  
		}else STMFLASH_Write_NoCheck(WriteAddr,pBuffer,secremain);//д�Ѿ������˵�,ֱ��д������ʣ������. 				   
		if(NumToWrite==secremain)break;//д�������
		else//д��δ����
		{
			secpos++;				//������ַ��1
			secoff=0;				//ƫ��λ��Ϊ0 	 
		   	pBuffer+=secremain;  	//ָ��ƫ��
			WriteAddr+=secremain;	//д��ַƫ��	   
		   	NumToWrite-=secremain;	//�ֽ�(16λ)���ݼ�
			if(NumToWrite>(STM_SECTOR_SIZE/2))secremain=STM_SECTOR_SIZE/2;//��һ����������д����
			else secremain=NumToWrite;//��һ����������д����
		}	 
	};	
	FLASH_Lock();//����
}
#endif

//��ָ����ַ��ʼ����ָ�����ȵ�����
//ReadAddr:��ʼ��ַ
//pBuffer:����ָ��
//NumToWrite:����(16λ)��
void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead)   	
{
	u16 i;
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i]=STMFLASH_ReadHalfWord(ReadAddr);//��ȡ2���ֽ�.
		ReadAddr+=2;//ƫ��2���ֽ�.	
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//WriteAddr:��ʼ��ַ
//WriteData:Ҫд�������
void STMFLASH_WriteHalfWord(u32 WriteAddr,u16 WriteData)   	
{
	STMFLASH_Write(WriteAddr,&WriteData,1);//д��һ���� 
}


/*
���Ź���ʱʱ��(40kHz������ʱ��(LSI)) (1)
Ԥ��Ƶϵ��  PR[2:0]λ   RL[11:0] = 0x000  RL[11:0] = 0xFFF
/4          0           0.1                 409.6
/8          1           0.2                 819.2
/16         2           0.4                 1638.4
/32         3           0.8                 3276.8
/64         4           1.6                 6553.6
/128        5           3.2                 13107.2
/256        (6��7)      6.4                 26214.4

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




