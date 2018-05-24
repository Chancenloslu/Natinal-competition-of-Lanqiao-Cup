#include "stc15f2k60s2.h"
#include "uart.h"

unsigned char dat_rev[6];
unsigned char sbuf_index=0;

bit busy;

bit Judge_revdat(unsigned char a[],unsigned char b[])
{
	unsigned char i;
	for(i=0;i<6;i++)
	{
		if(a[i]!=b[i])return 0;
	}
	return 1;
}

void UartInit(void)		//1200bps@11.0592MHz
{
	SCON = 0x50;		//8位数据,可变波特率
	AUXR |= 0x40;		//定时器1时钟为Fosc,即1T
	AUXR &= 0xFE;		//串口1选择定时器1为波特率发生器
	TMOD &= 0x0F;		//设定定时器1为16位自动重装方式
	TL1 = 0x00;		//设定定时初值
	TH1 = 0xF7;		//设定定时初值
	ET1 = 0;		//禁止定时器1中断
	TR1 = 1;		//启动定时器1
	ES = 1;
	EA = 1;
	SendString("STC15F2K60S2\r\nUart Test !\r\n");
}

/*----------------------------
发送串口数据
----------------------------*/
void SendData(unsigned char dat)
{
    while (busy);               //等待前面的数据发送完成
    busy = 1;
    SBUF = dat;                 //写数据到UART数据寄存器
}

/*----------------------------
发送字符串
----------------------------*/
void Uart() interrupt 4 using 2
{
    if (RI)
    {
        RI = 0;                 //清除RI位
        dat_rev[sbuf_index] = SBUF;
				sbuf_index++;
//        P22 = RB8;              //P2.2显示校验位
    }
    if (TI)
    {
        TI = 0;                 //清除TI位
        busy = 0;               //清忙标志
    }
}


void SendString(char *s)
{
    while (*s)                  //检测字符串结束标志
    {
        SendData(*s++);         //发送当前字符
    }
}
