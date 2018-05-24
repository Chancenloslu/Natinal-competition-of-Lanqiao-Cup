#include "stc15f2k60s2.h"
#include "ds1302.h"
#include "iic.h"

#define setkeyboard(x) P4=((x&0x80)>>3)|((x&0x40)>>4),P3=x
#define getkeyboard() ((P4&0x10)<<3)|((P4&0x04)<<4)|(P3&0x3f)
#define somenop Delay5us()

//sbit relay=P0^4;
//sbit buzzer=P0^6;
sbit TX = P1^0;
sbit RX = P1^1;

unsigned int door_count,error_count,buzzer_count,sonic_count,distance,t;
unsigned char read_key_data,cont,trg,display_mode,pw_index;
unsigned char hour,minute,second;
unsigned char smg_duan[10]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
unsigned char smg_wei[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char display[8],password[6],number_pw[6],initial_pw[6]={6,5,4,3,2,1};

bit key_flag,pw_judge_flag,dooropen_flag,buzzer_flag,day_flag,sonic_flag;

//---------------------函数声明-------------------------
void read_sfm(void);
void key_scan(void);
void send_wave(void);
void Delay10ms(void);
//------------------------------------------------------
void Delay10ms()		//@11.0592MHz
{
	unsigned char i, j;

	i = 108;
	j = 145;
	do
	{
		while (--j);
	} while (--i);
}

void send_wave()
{
	unsigned char i=8;
	do
	{
		TX=1;
		somenop;somenop;somenop;somenop;somenop;
		TX = 0;
		somenop;somenop;somenop;somenop;somenop;
	}while(i--);
	
}

void key_scan()
{
	unsigned char i;
		if(key_flag)
	{
		key_flag=0;
		
		setkeyboard(0xf0);
		read_key_data=getkeyboard();
		setkeyboard(0x0f);
		read_key_data=(read_key_data|getkeyboard())^0xff;
		trg=read_key_data&(read_key_data^cont);
		cont=read_key_data;
		
		switch(trg)
		{
			case 0x88:;break;		//s4
			case 0x84:password[i]=smg_duan[8];number_pw[i]=8;i++;break;			//s5
			case 0x82:password[i]=smg_duan[4];number_pw[i]=4;i++;break;		//s6
			case 0x81:password[i]=smg_duan[0];number_pw[i]=0;i++;break;		//s7
			case 0x48:;break;		//s8
			case 0x44:password[i]=smg_duan[9];number_pw[i]=9;i++;break;			//s9
			case 0x42:password[i]=smg_duan[5];number_pw[i]=5;i++;break;		//s10
			case 0x41:password[i]=smg_duan[1];number_pw[i]=1;i++;break;		//s11
			case 0x28:pw_judge_flag=1;break;		//s12 确认
			case 0x24:display_mode=3;for(pw_index=0;pw_index<6;pw_index++)password[pw_index]=0;break;			//s13	设置
			case 0x22:password[i]=smg_duan[6];number_pw[i]=6;i++;break;		//s14
			case 0x21:password[i]=smg_duan[2];number_pw[i]=2;i++;break;		//s15
			case 0x18:display_mode=2;password[0]=0;password[1]=0;password[2]=0;password[3]=0;password[4]=0;password[5]=0;break;		//s16 退出	输入密码界面
			case 0x14:				
				for(pw_index=0;pw_index<6;pw_index++)
				{
					write_at24c02(0x01+pw_index,6-pw_index);Delay10ms();
				}
				break;			//s17	复位
			case 0x12:password[i]=smg_duan[7];number_pw[i]=7;i++;break;		//s18
			case 0x11:password[i]=smg_duan[3];number_pw[i]=3;i++;break;		//s19
		}
		if(i==6)i=0;
	}
}

void read_sfm()				//BCD
{
	hour=Read_Ds1302(0x85);
	minute=Read_Ds1302(0x83);
	second=Read_Ds1302(0x81);
}
//-----------------------------------------------

/* Timer0 interrupt routine */
void tm0_isr() interrupt 1 using 1	//1ms
{
	static int smg_count,key_count;static unsigned char i;
	smg_count++;key_count++;
	
	if(smg_count==3)
	{
		smg_count=0;

		P2=0xc0;P0=0xff;P2=0;
		P2=0xe0;P0=~display[i];P2=0;
		P2=0xc0;P0=smg_wei[i];P2=0;
		i++;
		if(i==8)i=0;
	}
	if(key_count==10)
	{
		key_flag=1;
		key_count=0;
	}
	if(dooropen_flag)
	{
		door_count++;
		if(door_count==5000)
		{
			door_count=0;
			dooropen_flag=0;
			P2=0xa0;P0=0;P2=0;
		}
	}
	if(buzzer_flag)
	{
		buzzer_count++;
		if(buzzer_count==3000)
		{
			buzzer_count=0;
			buzzer_flag=0;
			P2=0xa0;P0=0;P2=0;
		}
	}
	if (sonic_count-- ==0)
	{
		sonic_count=200;
		sonic_flag=1;
	}
}

//-----------------------------------------------
void main()
{
	
	AUXR |= 0x80;		//定时器时钟1T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0xCD;		//设置定时初值					1ms
	TH0 = 0xD4;		//设置定时初值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	ET0=1;
	EA=1;
	
	P2=0xa0;P0=0;P2=0;
	set_sfm(6,59,0);
	day_flag=1;
//	initial_pw[0]=read_at24c02(0x01);
	for(pw_index=0;pw_index<6;pw_index++)
	{
		initial_pw[pw_index]=read_at24c02(0x01+pw_index);
	}
	
	while(1)
	{
		read_sfm();

		if(day_flag)
		{
			if(hour<=22&&hour>=7)
			{
				display_mode=1;
				if(sonic_flag)								//超声波
				{
					sonic_flag=0;
					send_wave();
					TR1=1;
					while((RX == 1) && (TF1 == 0)); 
					TR1=0;
					if(TF1==1)
					{
						TF1 = 0;
						distance = 999;  
					}
					else
					{
						t = TH1;
						t <<= 8;
						t |= TL1;
						distance = (unsigned int)(t*0.017);
					}
					TH1 = 0;
					TL1 = 0;
	//				distance_table[0]=T_display[distance/100] | 0x80;
	//				distance_table[1]=T_display[distance/10%10];
	//				distance_table[2]=T_display[distance%10];
				}
				if(distance<=30)
				{
					dooropen_flag=1;
					P2=0xa0;P0=0x10;P2=0;
				}
			}
			else
			{
				display_mode=2;
				day_flag=0;
			}
		}
		if(hour>22||hour<7)day_flag=0;
		else day_flag=1;
		
		switch(display_mode)
		{
			case 1:													//时间显示界面
			display[0]=smg_duan[hour/16];
			display[1]=smg_duan[hour%16];
			display[2]=0x40;
			display[3]=smg_duan[minute/16];
			display[4]=smg_duan[minute%16];
			display[5]=0x40;
			display[6]=smg_duan[second/16];
			display[7]=smg_duan[second%16];
			break;
			case 2:														//密码输入界面
			display[0]=0x40;
			display[1]=0x40;
			display[2]=password[0];
			display[3]=password[1];
			display[4]=password[2];
			display[5]=password[3];
			display[6]=password[4];
			display[7]=password[5];
			break;
			case 3:														//密码设置初始密码输入界面
			display[0]=0;
			display[1]=0x40;
			display[2]=password[0];
			display[3]=password[1];
			display[4]=password[2];
			display[5]=password[3];
			display[6]=password[4];
			display[7]=password[5];	
			break;
			case 4:
			display[0]=0x40;
			display[1]=0;
			display[2]=password[0];
			display[3]=password[1];
			display[4]=password[2];
			display[5]=password[3];
			display[6]=password[4];
			display[7]=password[5];
			break;
		}
		if(display_mode==2)											//密码输入界面
		{
			if(pw_judge_flag==1)
			{
				pw_judge_flag=0;
					for(pw_index=0;pw_index<6;pw_index++)
					{
						initial_pw[pw_index]=read_at24c02(0x01+pw_index);
					}
				if(number_pw[0]==initial_pw[0]&&number_pw[1]==initial_pw[1]&&number_pw[2]==initial_pw[2]&&number_pw[3]==initial_pw[3]&&number_pw[4]==initial_pw[4]&&number_pw[5]==initial_pw[5])
				{
					dooropen_flag=1;
					error_count=0;
					for(pw_index=0;pw_index<6;pw_index++)password[pw_index]=0;
					P2=0xa0;P0=0x10;P2=0;
				}
				else
				{
					display_mode=2;
					error_count++;
					for(pw_index=0;pw_index<6;pw_index++)password[pw_index]=0;
				}
				if(error_count==3){error_count=0;buzzer_flag=1;P2=0xa0;P0=0x40;P2=0;}						//蜂鸣器
			}
		}
		if(display_mode==3)												//密码设置初始密码输入界面
		{
			if(pw_judge_flag==1)
			{
				pw_judge_flag=0;
				for(pw_index=0;pw_index<6;pw_index++)	//读取EEPROM中的密码
				{
					initial_pw[pw_index]=read_at24c02(0x01+pw_index);
				}
				if(number_pw[0]==initial_pw[0]&&number_pw[1]==initial_pw[1]&&number_pw[2]==initial_pw[2]&&number_pw[3]==initial_pw[3]&&number_pw[4]==initial_pw[4]&&number_pw[5]==initial_pw[5])//比较
				{
					display_mode=4;
					error_count=0;
					for(pw_index=0;pw_index<6;pw_index++)password[pw_index]=0;
				}
				else
				{
					error_count++;
					for(pw_index=0;pw_index<6;pw_index++)password[pw_index]=0;
				}
				if(error_count==3)
				{
					display_mode=2;
					for(pw_index=0;pw_index<6;pw_index++)password[pw_index]=0;				//输入界面清零
					error_count=0;
			 		buzzer_flag=1;P2=0xa0;P0=0x40;P2=0;						//蜂鸣器
				}				
			}
		}
		if(display_mode==4)											//密码设置界面
		{
			if(pw_judge_flag==1)
			{
				pw_judge_flag=0;
				for(pw_index=0;pw_index<6;pw_index++)
				{
					write_at24c02(0x01+pw_index,number_pw[pw_index]);Delay10ms();			//延时10ms 写数据完成
				}
				for(pw_index=0;pw_index<6;pw_index++)password[pw_index]=0;
				display_mode=2;
			}
		}
		key_scan();
	}
} 