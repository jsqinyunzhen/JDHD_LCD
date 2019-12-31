#ifndef _WORK_H
#define _WORK_H

#include "stm32f10x.h"

#define FLASH_SAVE_ADDR  0X0800FC00 	//设置FLASH 保存地址(必须为偶数，且其值要大于本代码所占用FLASH的大小+0X08000000)

void LED_IO_INIT(void);
void TIME4_init(u16 arr,u16 psc);
void TIM4_IRQHandler(void);
u16 STMFLASH_ReadHalfWord(u32 faddr);
void STMFLASH_WriteHalfWord(u32 WriteAddr,u16 WriteData);
void iwdg_init(void);

#endif

