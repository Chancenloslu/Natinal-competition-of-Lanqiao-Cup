#include "stc15f2k60s2.h"
#include "ds1302.h"
#include "iic.h"
#include "onewire.h"
#include "uart.h"

//#define setkeyboard(x) P4=((x&0x80)>>3)|((x&0x40)>>4);P3=x
//#define getkeyboard() ((P4&0x10)<<3)|((P4&0x04)<<4)|(P3&0x3f)
#define key_input P3
#define key_state_0 0	//判断有键是否按下
#define key_state_1 1	//判断是否有抖动
#define key_state_2 2	//判断是否弹起
#define key_mask 0x0f

sbit L0=P0^0;
sbit L1=P0^1;
sbit L2=P0^2;

//smg
unsigned char code smg_duan[10]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
												//   0    1    2    3    4    5    6    7    8    9  
unsigned char code smg_wei[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
unsigned char display[8];
unsigned char display_mode,shi,miao,fen;
unsigned char liangdu,shidu,key_val;
unsigned int fengefu_count,temperature;
//uart
unsigned char code dat_true[6]={'A','A','A','S','S','S'};
//stay_time
unsigned char stay_time[5]={50,100,20,30,40};
unsigned int guangmin_count;
unsigned char stay_time_index;
//workmoede
unsigned char work_mode;//0 自动传输模式（默认） 1自动记录模式
unsigned char last_miao,tx_index,tx2_index,eeprom_index;
xdata unsigned char eeprom[5][24]={"{26-47%}{00-01-28}{05}\r\n","{26-47%}{00-01-29}{05}\r\n","{26-47%}{00-01-30}{05}\r\n","{26-47%}{00-01-31}{05}\r\n","{26-47%}{00-01-32}{05}\r\n"};

bit key_flag,blick_flag,stay_time_flag,send_flag,index_flag,last_stay_time_flag;

//**********函数声明**********************
void Delay10ms(void);
unsigned char read_KBD();
void sfm_read(void);
//****************************************
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
void sfm_read()
{
	last_miao=miao;
	miao=Read_Ds1302(0x81);
	fen=Read_Ds1302(0x83);
	shi=Read_Ds1302(0x85);
}

unsigned char read_KBD()
{
		static unsigned char key_state=0;
		char key_press,key_return=0;
	if(key_flag)
	{
		key_flag=0;

		key_press=key_input&key_mask;
		switch(key_state)
		{
			case key_state_0:
				if(key_press!=key_mask)key_state=key_state_1;break;
			case key_state_1:
				if(key_press==(key_input&key_mask))
				{
					switch(key_press)
					{
						case 0x0e:key_return=7;break;
						case 0x0c:key_return=6;break;
						case 0x0b:key_return=5;break;//s5
						case 0x07:key_return=4;break;//s4
					}
					key_state = key_state_2;
				}
				else
				{
					key_state = key_state_0;
				}break;
				case key_state_2:
					if(key_press==0x0f)key_state=key_state_0;break;
		}
	}
	return key_return;
}


void tm0_isr() interrupt 1 using 1													//t0
{
	static unsigned int smg_count,key_count,i;
	smg_count++;key_count++;fengefu_count++;
	if(smg_count==3)
	{
		smg_count=0;
		
		P2=0xc0;P0=0;P2=0;
		P2=0xe0;P0=~display[i];P2=0;
		P2=0xc0;P0=smg_wei[i];P2=0;
		if(++i==8)i=0;
		
	}
	if(key_count==10)
	{
		key_count=0;
		key_flag=1;
	}

	if(fengefu_count==1000)
	{
		fengefu_count=0;
		blick_flag=~blick_flag;
	}
	
	if(stay_time_flag)
	{
		if(index_flag)
		{
			stay_time_index++;
			if(stay_time_index==5)stay_time_index=0;
			stay_time[stay_time_index]=0;
			index_flag=0;
		}
		guangmin_count++;
		if(guangmin_count==1000)
		{
			guangmin_count=0;
			stay_time[stay_time_index]++;
		}
	}


}
void main()
{
	AUXR |= 0x80;		//定时器时钟1T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0xCD;		//设置定时初值
	TH0 = 0xD4;		//设置定时初值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	ET0 = 1;
	EA = 1;
	
	UartInit();
	
	display_mode=1;
	set_sfm(23,59,55);
	P2=0xa0;P0=0;P2=0x80;P0=0xff;P2=0;
	
	
	while(1)
	{
		if(sbuf_index>5)
		{
			if(Judge_revdat(dat_rev,dat_true))
			{
				send_flag=1;
//				SendString("right");
//				for(sbuf_index=0;sbuf_index<6;sbuf_index++)
//				{
//					SendData(dat_rev[sbuf_index]);					
//				}
			}
			else send_flag = 0;
			sbuf_index=0;
		}		
		
		
		EA = 0;
		sfm_read();
		shidu=pcf8591_read(0x01)/255.0*99;							//电位器
		liangdu=pcf8591_read(0x03)/255.0*99;						//光敏  两路信号串线 将两路地址调换
		EA = 1;
		temperature=(unsigned int)ds18b20_tempread();
		
		if(send_flag)
		{
			if(work_mode==0)			//自动传输模式
			{
				if(last_miao!=miao)		//1秒后
				{
					SendData('{');SendData(temperature/10+'0');SendData(temperature%10+'0');SendData('-');SendData(shidu/10+'0');SendData(shidu%10+'0');SendData('%');SendData('}');
					SendData('{');SendData(shi/16+'0');SendData(shi%16+'0');SendData('-');SendData(fen/16+'0');SendData(fen%16+'0');SendData('-');SendData(miao/16+'0');SendData(miao%16+'0');SendData('}');
					SendData('{');SendData(stay_time[stay_time_index]/10+'0');SendData(stay_time[stay_time_index]%10+'0');SendData('}');SendData('\r');SendData('\n');
				}
			}
			if(work_mode==1)			//自动记录模式		发送数据
			{
				for(tx_index=0;tx_index<5;tx_index++)
				{
					for(tx2_index=0;tx2_index<24;tx2_index++)
					{
						SendData(eeprom[tx_index][tx2_index]);
					}
				}
				send_flag=0;
			}
		}
		
		last_stay_time_flag=stay_time_flag;		//记录等候时间时 last_stay_time_flag=1
		if(liangdu<50){stay_time_flag=1;P2=0x80;L2=0;P2=0;}		//1111 1011
		else {stay_time_flag=0;index_flag=1;P2=0x80;L2=1;P2=0;} //0000 0100
		
		if(work_mode==1&&last_stay_time_flag==1&&stay_time_flag==0)			//当人离开stay_time_flag=0 last_stay_time_flag>stay_time_flag
		{
//			eeprom[stay_time_index][0]={'{'}

				write_at24c02(stay_time_index*16+0,temperature);Delay10ms();
				write_at24c02(stay_time_index*16+1,shidu);Delay10ms();
				write_at24c02(stay_time_index*16+2,shi);Delay10ms();
				write_at24c02(stay_time_index*16+3,fen);Delay10ms();
				write_at24c02(stay_time_index*16+4,miao);Delay10ms();
				write_at24c02(stay_time_index*16+5,stay_time[stay_time_index]);Delay10ms();
		}
		for(eeprom_index=0;eeprom_index<5;eeprom_index++)
		{
			eeprom[eeprom_index][0]='{';
			eeprom[eeprom_index][1]=read_at24c02(eeprom_index*16+0)/10+'0';eeprom[eeprom_index][2]=read_at24c02(eeprom_index*16+0)%10+'0';
			eeprom[eeprom_index][3]='-';
			eeprom[eeprom_index][4]=read_at24c02(eeprom_index*16+1)/10+'0';eeprom[eeprom_index][5]=read_at24c02(eeprom_index*16+1)%10+'0';
			eeprom[eeprom_index][6]='%';eeprom[eeprom_index][7]='}';
			eeprom[eeprom_index][8]='{';
			eeprom[eeprom_index][9]=read_at24c02(eeprom_index*16+2)/16+'0';eeprom[eeprom_index][10]=read_at24c02(eeprom_index*16+2)%16+'0';
			eeprom[eeprom_index][11]='-';
			eeprom[eeprom_index][12]=read_at24c02(eeprom_index*16+3)/16+'0';eeprom[eeprom_index][13]=read_at24c02(eeprom_index*16+3)%16+'0';
			eeprom[eeprom_index][14]='-';
			eeprom[eeprom_index][15]=read_at24c02(eeprom_index*16+4)/16+'0';eeprom[eeprom_index][16]=read_at24c02(eeprom_index*16+4)%16+'0';
			eeprom[eeprom_index][17]='}';eeprom[eeprom_index][18]='{';
			eeprom[eeprom_index][19]=read_at24c02(eeprom_index*16+5)/10+'0';eeprom[eeprom_index][20]=read_at24c02(eeprom_index*16+5)%10+'0';
			eeprom[eeprom_index][21]='}';
			eeprom[eeprom_index][22]='\r';eeprom[eeprom_index][23]='\n';	//回车换行
		}

		
		switch(work_mode)
		{
			case 0:P2=0x80;L0=0;L1=1;P2=0;break;		//1111 1*10
			case 1:P2=0x80;L0=1;L1=0;P2=0;break;		//1111 1*01
		}
		switch(display_mode)
		{
			case 1:
			{
				display[0]=smg_duan[temperature/10];
				display[1]=smg_duan[temperature%10];
				display[2]=0x39;						//C
				display[3]=0;
				display[4]=0;
				display[5]=smg_duan[shidu/10];
				display[6]=smg_duan[shidu%10];
				display[7]=0x76;						//H
			}break;
			case 2:																			//时钟
			{
				display[0]=smg_duan[shi/16];
				display[1]=smg_duan[shi%16];
				display[3]=smg_duan[fen/16];
				display[4]=smg_duan[fen%16];
				display[6]=smg_duan[miao/16];
				display[7]=smg_duan[miao%16];
				if(blick_flag)
				{
					display[2]=0;
					display[5]=0;	
				}
				else
				{
					display[2]=0x40;						//-
					display[5]=0x40;						//-
				}
			}break;
			case 3:
			{
				display[0]=0;
				display[1]=0;
				display[2]=0;						
				display[3]=0x40;
				display[4]=smg_duan[stay_time[stay_time_index]/1000];
				display[5]=smg_duan[stay_time[stay_time_index]%1000/100];
				display[6]=smg_duan[stay_time[stay_time_index]%100/10];
				display[7]=smg_duan[stay_time[stay_time_index]%10];									
		
			}break;	
		}
//		key_val=read_KBD();
//		if(key_val==3)//S15
//		{
//			if(++display_mode==4){display_mode=1;}
//		}
//		if(key_val==4)//S14
//		{
//			P2=0xa0;buzzer=0;P2=0x00;
//		}
		switch(read_KBD())
		{
			case 4:work_mode++;if(work_mode==2)work_mode=0;break;				//s4
			case 5:if(++display_mode==4){display_mode=1;}break;					//s5
		}

	}
}