#include "stm32f10x.h"
#include "USART.h"
#include "SYS.h"
#include "delay.h"
#include "WORK.h"
#include "display.h"
#include "ADC.h"
#include "MPU6050.h"
#include "inv_mpu.h"
#include "WORK.h"
#include "KS0108.h"

uint8_t Version[]={"V1.2 20200122"};
u32 sys_ms_cnt = 0;
extern uint8_t Version[];
u8 LCD_ID = 0x0;
extern void USART1_Receive_DataAnalysis(void);
extern void Usart_SetLCDIdReply(void);

void Get_LCD_ID(void)
{
#if 0
    GPIO_InitTypeDef  GPIO_init;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB , ENABLE);

    GPIO_init.GPIO_Pin=GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
   // GPIO_init.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_init.GPIO_Mode=GPIO_Mode_IPU;
    GPIO_Init(GPIOB,&GPIO_init);

    delay_ms(10);
    LCD_ID =0;
    LCD_ID |= PBin(15);
    LCD_ID <<= 1;
    LCD_ID |= PBin(14);
    LCD_ID <<= 1;
    LCD_ID |= PBin(13);
    LCD_ID <<= 1;
    LCD_ID |= PBin(12);
    LCD_ID <<= 1;
    LCD_ID |= PBin(11);
    delay_ms(10);
#else
    LCD_ID = STMFLASH_ReadHalfWord(FLASH_SAVE_ADDR);
#endif
}

void Led_OnOff(void)
{
	static u32 led_delay = 0;
	
	if(sys_ms_cnt - led_delay > 1000)
	{
		led_delay = sys_ms_cnt;
		PAout(8)=!PAout(8);
	}
}

#define BREAKER_CRC_POLY (0x01)
u8 Breaker_CRC8(u8 *ptr, u8 len)
{
    u8 i; 
    u8 crc=0x00; /* 计算的初始crc值 */ 

    while(len--)
    {
        crc ^= *ptr++;  /* 每次先与需要计算的数据异或,计算完指向下一数据 */  
        for (i=8; i>0; --i)   /* 下面这段计算过程与计算一个字节crc一样 */  
        { 
            if (crc & 0x80)
                crc = (crc << 1) ^ BREAKER_CRC_POLY;
            else
                crc = (crc << 1);
        }
    }

    return (crc); 
}
#define IAP_ADDR 0X08000000
#define ApplicationAddress    0x08008000 //32k
#define APP_OFFSET (ApplicationAddress-IAP_ADDR)

int main()
{
    SysTick_Config(72000);
#if 1 //is iap
    NVIC_SetVectorTable(NVIC_VectTab_FLASH,APP_OFFSET);
#endif
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    iwdg_init();
    delay_init();
    //Adc_Init();
    LED_IO_INIT();
    USART_init();
    Display_InitializeEN();
    GLCD_Initialize();
    Get_LCD_ID();
    Display_Init();
		
    while(1)
    {
        IWDG_ReloadCounter();
        Led_OnOff();
        Usart_SetLCDIdReply();
        USART1_Receive_DataAnalysis();
        Display_CabinetStatus();
    }			
}
 

