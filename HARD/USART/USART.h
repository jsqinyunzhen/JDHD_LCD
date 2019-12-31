#ifndef _USART_H
#define _USART_H

#include "stm32f10x.h"

void USART_init(void);
void usart1_send_data(u8 c);
void usart1_niming_send(u8 fun,u8*data,u8 len);
void usart1_send(short aacx,short aacy,short aacz,short gyrox,short gyroy,short gyroz,short roll,short pitch,short yaw);
void USART1_IRQHandler(void);

#endif

