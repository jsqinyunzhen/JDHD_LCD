#include "USART.h"
#include "ADC.h"
#include "delay.h"
//#include "MPU6050.h"
#include "display.h"
#include "string.h"
#include "stdlib.h"
#include "WORK.h"

#define SETLCDID1 "SET LCD ID:0\r\n"
#define SETLCDID2 "SET LCD ID:10\r\n"

u32 receive_finish = 0;
u8 rec_buf[32] = {0};
u8 buf_reply_LCDID[32] = {0};
u8 rec_data_len = 0;
extern u32 sys_ms_cnt;
extern u8 CurDispNum ;
extern DisplayCabinet DispCabinet[];
extern u8 LCD_ID;
extern uint8_t Version[];

#define RX_FRAME_LEN 14


void USART_init(void)           //串口初始化
{
	   GPIO_InitTypeDef  Usart_IOinit;                        //配置IO口
	   USART_InitTypeDef  Usart_init;                         //配置串口
	   NVIC_InitTypeDef  NVIC_init;                           //配置中断
	
	   RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  //使能端口A时钟
	   RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE); //使能串口1时钟
	
	   Usart_IOinit.GPIO_Pin=GPIO_Pin_9;                      //配置GPIOA9  TX
	   Usart_IOinit.GPIO_Speed=GPIO_Speed_50MHz;
	   Usart_IOinit.GPIO_Mode=GPIO_Mode_AF_PP;
	   GPIO_Init(GPIOA,&Usart_IOinit);
	
	   Usart_IOinit.GPIO_Pin=GPIO_Pin_10;                     //配置GPIOA10  RX
	   Usart_IOinit.GPIO_Speed=GPIO_Speed_50MHz;
	   Usart_IOinit.GPIO_Mode=GPIO_Mode_IN_FLOATING;
	   GPIO_Init(GPIOA,&Usart_IOinit);
	
	   Usart_init.USART_BaudRate=9600;                        //配置串口
	   Usart_init.USART_WordLength=USART_HardwareFlowControl_None;
	   Usart_init.USART_StopBits=USART_StopBits_1;
	   Usart_init.USART_Parity=USART_Parity_No;
	   Usart_init.USART_Mode=USART_Mode_Rx|USART_Mode_Tx;
	   Usart_init.USART_HardwareFlowControl=USART_WordLength_8b;
	   USART_Init(USART1,&Usart_init);
	   
	   USART_Cmd(USART1,ENABLE);                             //使能串口1
	   
	   USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);          //选择串口中断
	   
	   NVIC_init.NVIC_IRQChannel=USART1_IRQn;                //配置中断函数
	   NVIC_init.NVIC_IRQChannelCmd=ENABLE;
	   NVIC_init.NVIC_IRQChannelPreemptionPriority=1;
	   NVIC_init.NVIC_IRQChannelSubPriority=1;
	   NVIC_Init(&NVIC_init);

	   Usart_IOinit.GPIO_Pin=GPIO_Pin_11;                      //配置485 TX
	   Usart_IOinit.GPIO_Speed=GPIO_Speed_50MHz;
	   Usart_IOinit.GPIO_Mode=GPIO_Mode_Out_PP;
	   GPIO_Init(GPIOA,&Usart_IOinit);
	
	   Usart_IOinit.GPIO_Pin=GPIO_Pin_12;                     //配置485  RX
	   Usart_IOinit.GPIO_Speed=GPIO_Speed_50MHz;
	   Usart_IOinit.GPIO_Mode=GPIO_Mode_Out_PP;
	   GPIO_Init(GPIOA,&Usart_IOinit);
       PAout(11) = 0;
       PAout(12) = 0;
}

//串口1发送1个字符 
//c:要发送的字符
void usart1_send_data(u8 c)
{   	
	while(USART_GetFlagStatus(USART1,USART_FLAG_TC)==RESET); //循环发送,直到发送完毕   
	USART_SendData(USART1,c);  
} 
void Usart_SetLCDIdReply(void)
{
    int i =0;
    int len = strlen((char*)buf_reply_LCDID);
    
    if(len > 0)
    {
        PAout(11) = 1;
        PAout(12) = 1;
        delay_ms(5);
        for(i = 0; i < len; i++)
        {
            while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
            USART_SendData(USART1,buf_reply_LCDID[i]);
        }
        delay_ms(5);
        memset(buf_reply_LCDID,0,sizeof(buf_reply_LCDID));
				
        PAout(11) = 0;
        PAout(12) = 0;
        
    }
}

int USART1_FrameCheckSum(u8 *buf , u8 len)
{
    int i =0;
    u16 sum = 0;
    if(buf == 0 || len < RX_FRAME_LEN)
    {
        return 1;//帧错误
    }
    if(buf[0] != 0x3A || buf[2] != 0xFF)
    {
        return 2;//帧错误
    }
    len = buf[3]+3; //要校验的数据数
    for(i = 1; i < (len+1); i++)
    {
        sum += buf[i];
    }
    return sum - (buf[i]+(((u16)buf[i+1])<<8));

}


//3A+ID+FF+长度+状态+电池状态1+电池状态2+电池状态3+电池状态4+SOC+CheckSum低+CheckSum高+0D+0A
//0   1  2   3   4      5         6         7        8        9    10         11       12 13
void USART1_Receive_DataAnalysis(void)
{
	if((rec_data_len > 0 && (sys_ms_cnt - receive_finish) > 5) || (rec_data_len >= RX_FRAME_LEN))
	{
        if(USART1_FrameCheckSum(rec_buf,rec_data_len) == 0)
        {
            if(LCD_ID == rec_buf[1])
            {
                Battery_tatus old_cs = DispCabinet[CurDispNum].cs;
                
                if(rec_buf[4] == 0)
                {
                    DispCabinet[CurDispNum].cs = BAT_EMPTY;
                    DispCabinet[CurDispNum].updatescreen = 1;
                }
                else if(rec_buf[4] == 1)
                {
                    DispCabinet[CurDispNum].cs = BAT_RESERVED;
                    DispCabinet[CurDispNum].updatescreen = 1;
                }
                else if((rec_buf[8]&0x80) || rec_buf[9] == 100)
                {
                    DispCabinet[CurDispNum].cs = BAT_POWER_FULL;
                    DispCabinet[CurDispNum].updatescreen = 1;
                }
                else
                {
                    DispCabinet[CurDispNum].cs = BAT_CHARGING;
                }

                
                if(DispCabinet[CurDispNum].cs != old_cs)
                {
                    DispCabinet[CurDispNum].updatescreen = 1;
                }
                DispCabinet[CurDispNum].soc = rec_buf[9];
            }
        }
        else if(rec_data_len == strlen(SETLCDID1) || rec_data_len == strlen(SETLCDID2))
        {
            if(strncmp((char*)rec_buf,"SET LCD ID:", strlen("SET LCD ID:")) == 0)
            {
                u16 num = atoi((char*)rec_buf + strlen("SET LCD ID:"));
                u16 lcdid = 0xff;
                
                if(num > 0 && rec_buf[rec_data_len-1] == '\n' && rec_buf[rec_data_len-2] == '\r')
                {
                   num -= 1;
                   STMFLASH_WriteHalfWord(FLASH_SAVE_ADDR,num);
                   lcdid = STMFLASH_ReadHalfWord(FLASH_SAVE_ADDR);
                   if(lcdid == num)
                   {
                       LCD_ID = num;
                       memset((char*)buf_reply_LCDID,0,sizeof(buf_reply_LCDID));
                       strcpy((char*)buf_reply_LCDID,(char*)rec_buf);
                       strcat((char*)buf_reply_LCDID,"OK\r\n");
                   }
                   else
                   {
                       memset((char*)buf_reply_LCDID,0,sizeof(buf_reply_LCDID));
                       strcpy((char*)buf_reply_LCDID,(char*)rec_buf);
                       strcat((char*)buf_reply_LCDID,"error\r\n");
                   }
                }
                else if(rec_buf[rec_data_len-3] == '?')
                {
                    memset((char*)buf_reply_LCDID,0,sizeof(buf_reply_LCDID));
                    strcat((char*)buf_reply_LCDID,"ID=  \r\n");
                    
                    if(LCD_ID > 10)
                    {
                        buf_reply_LCDID[3] = '1'+LCD_ID/10%10;
                        buf_reply_LCDID[4] = '1'+LCD_ID%10;
                    }
                    else
                    {
                        buf_reply_LCDID[3] = '1'+LCD_ID%10;
                    }
                }
                else
                {
                    memset((char*)buf_reply_LCDID,0,sizeof(buf_reply_LCDID));
                    //strcpy((char*)buf_reply_LCDID,(char*)rec_buf);
                    strcat((char*)buf_reply_LCDID,"\r\nset error\r\n");
                }
            }
        }
        else if(rec_data_len == strlen("VERSION") && strncmp((char*)rec_buf,"VERSION", strlen("VERSION")) == 0)
        {
            memset((char*)buf_reply_LCDID,0,sizeof(buf_reply_LCDID));
            strcat((char*)buf_reply_LCDID,(char*)Version);
        }
        memset(rec_buf,0,sizeof(rec_buf));
        rec_data_len = 0;
	}
}

void USART1_IRQHandler(void)                         //串口1中断函数
{
	   if(USART_GetITStatus(USART1,USART_IT_RXNE))
	   {
		    //u16 data;
			  //short GX,GY,GZ;
			  //data = MPU_INIT();
			  //MPU_Get_TLY(&GX,&GY,&GZ);
		    rec_buf[rec_data_len++] = USART_ReceiveData(USART1);
			  if(rec_data_len >= sizeof(rec_buf))
			  {
						rec_data_len = 0;
			  }
			  receive_finish = sys_ms_cnt;
			  USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	   }
}


