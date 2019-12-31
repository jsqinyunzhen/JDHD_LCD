#ifndef _WORK_H
#define _WORK_H

#include "stm32f10x.h"

#define FLASH_SAVE_ADDR  0X0800FC00 	//����FLASH �����ַ(����Ϊż��������ֵҪ���ڱ�������ռ��FLASH�Ĵ�С+0X08000000)

void LED_IO_INIT(void);
void TIME4_init(u16 arr,u16 psc);
void TIM4_IRQHandler(void);
u16 STMFLASH_ReadHalfWord(u32 faddr);
void STMFLASH_WriteHalfWord(u32 WriteAddr,u16 WriteData);
void iwdg_init(void);

#endif

