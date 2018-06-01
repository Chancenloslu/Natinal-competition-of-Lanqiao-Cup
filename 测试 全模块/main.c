#include "stc15f2k60s2.h"
#include "iic.h"
#include "ds1302.h"
#include "onewire.h"
#include "sonic.h"
#include "uart.h"
#include "ne555.h"

#define key_state_0 0				//判断是否按下
#define key_state_1 1				//是否是抖动
#define key_state_2 2				//是否弹起
#define setkeyboard(x) P4=((x&0x80)>>3)|((x&0x40)>>4);P3=x
#define getkeyboard() ((P4&0x10)<<3)|((P4&0x04)<<4)|(P3&0x3f)
sbit buzzer=P0^6;
//smg
unsigned char code smg_duan[10]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
															//   0    1    2    3    4    5    6    7    8    9
unsigned char code smg_wei[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char code password[6]={'1','2','3','4','5','6'};
unsigned char display[8];
unsigned char display_mode;
//keyboard
unsigned char readkeyvalue,cont,trg;
//超声波
unsigned int distance,sonic_count;
bit sonic_flag;
//pcf8591
unsigned char val;
//温度
unsigned int temperature;
//time
unsigned char shi,fen,miao;
//555
unsigned int frequence;
bit key_flag;
/********函数声明*********/
void read_sfm(void);
void key_scan(void);
void Timer2Init(void);

/*************************/
void Timer2Init(void)		//1毫秒@11.0592MHz
{
	AUXR |= 0x04;		//定时器时钟1T模式
	T2L = 0xCD;		//设置定时初值
	T2H = 0xD4;		//设置定时初值
	AUXR |= 0x10;		//定时器2开始计时
	IE2 |= (1<<2);
}

void key_scan()
{
	setkeyboard(0x0f);
	readkeyvalue=getkeyboard();
	setkeyboard(0xf0);
	readkeyvalue=(readkeyvalue|getkeyboard())^0xff;
	trg=readkeyvalue&(readkeyvalue^cont);
	cont=readkeyvalue;
	switch(trg)
	{
		case 0x88:display_mode=0;break;
		case 0x84:display_mode=1;break;
		case 0x82:display_mode=2;break;
		case 0x81:display_mode=3;break;
	}
//	static unsigned char key_state;					//状态机消抖，需要在中断进行10ms延时。
//	unsigned char key_press;
//		key_press=P3&0x0f;
//	
//	if(key_flag)
//	{
//		key_flag=0;
//		switch(key_state)
//		{
//			case 0:if(key_press!=0x0f)key_state=key_state_1;break;
//			case 1:
//			if(key_press==(P3&0x0f))
//			{
//				switch(key_press)
//				{
//					case 0x07:P2=0xa0;buzzer=1;P2=0;break;			//s4
//					case 0x0b:P2=0xa0;buzzer=0;P2=0;break;				//s5
//				}
//				key_state=key_state_2;
//			}
//			else
//			{
//				key_state=key_state_0;
//			}break;
//			case 2:
//				if(key_press==0x0f)key_state=key_state_0;break;
//		}
//	}
}
void read_sfm()
{
	miao=Read_Ds1302(0x81);
	fen=Read_Ds1302(0x83);
	shi=Read_Ds1302(0x85);				//BCD
}

//-----------------------------------------------


void tm2_isr() interrupt 12 using 3
{
	static unsigned char i,smg_count;
	smg_count++;sonic_count++;
	if(smg_count==3)
	{
		smg_count=0;
		P2=0xc0;P0=0;P2=0;
		P2=0xe0;P0=~display[i];P2=0;
		P2=0xc0;P0=smg_wei[i];P2=0;
		i++;
		if(i==8)i=0;
	}
//	if(sonic_count==200)
//	{
//		sonic_count=0;
//		sonic_flag=1;
//	}
}



//-----------------------------------------------

void main()
{
	
	Timer0Init();
	Timer1Init();
	Timer2Init();
//	UartInit();		//9600bps@11.0592MHz		使用了T2作为波特率发生器

	EA = 1;
//	SendString("STC15F2K60S2\r\nUart Test !\r\n");
	P2=0xa0;P0=0;P2=0;
	set_sfm(23,59,55);
	while(1)
	{
		if(rx_index>5)
		{
			rx_index=0;
			if(judge(password,rx_sbuf))
			{
				SendString("right");
			}
			else
			{
				SendString("wrong");
			}
		}		
		read_sfm();
		temperature=(unsigned int)rd_temperature();
		EA=0;
		val=(unsigned char)read_pcf8591(0x03)/255.0*99;
		EA=1;
		
		frequence=get_freq();
			
//		if(sonic_flag)						//使用了T1，与数码管重复。555使用P34作外部脉冲输入，使用T0的计数器。
//		{
//			sonic_flag=0;
//			distance = get_distance();
//		}
		switch(display_mode)
		{
			case 0:
				{
					display[0]=smg_duan[shi/16];
					display[1]=smg_duan[shi%16];
					display[2]=0x40;
					display[3]=smg_duan[fen/16];
					display[4]=smg_duan[fen%16];
					display[5]=0x40;
					display[6]=smg_duan[miao/16];
					display[7]=smg_duan[miao%16];
				}break;
			case 1:
				{
					display[0]=smg_duan[temperature/10];
					display[1]=smg_duan[temperature%10];
					display[2]=smg_duan[val/10];
					display[3]=smg_duan[val%10];
					display[4]=0;
					display[5]=smg_duan[distance/100];
					display[6]=smg_duan[distance%100/10];
					display[7]=smg_duan[distance%10];
				}break;
			case 2:
			{
				display[0]=0;
				display[1]=0;
				display[2]=0;
				display[3]=smg_duan[frequence/10000];
				display[4]=smg_duan[frequence%10000/1000];
				display[5]=smg_duan[frequence%1000/100];
				display[6]=smg_duan[frequence%100/10];
				display[7]=smg_duan[frequence%10];
			}
			
		}
		key_scan();
	}
}