#include "stc15f2k60s2.h"

//555
unsigned int wave_count;
bit wave_flag;

void Timer0Init()
{
	AUXR |= 0x80;
	TMOD =0X04;
	TH0=TL0=0XFF;
	TR0 = 1;
	ET0 = 1;
}
void Timer1Init()		//50����@11.0592MHz
{
	AUXR &= 0xBF;		//��ʱ��ʱ��12Tģʽ
	TMOD &= 0x0F;		//���ö�ʱ��ģʽ
	TL1 = 0x00;		//���ö�ʱ��ֵ
	TH1 = 0x4C;		//���ö�ʱ��ֵ
	TF1 = 0;		//���TF1��־
	TR1 = 1;		//��ʱ��1��ʼ��ʱ
	ET1 = 1;
}

void tm0_isr() interrupt 1 using 1
{
    wave_count++;
}

void tm1_isr() interrupt 3 using 2
{
	static unsigned char ms_count;
	ms_count++;
	if(ms_count==20)
	{
		wave_flag=1;
		ms_count=0;
		TR0 = 0;
		TR1 = 0;
		ET0 = 0;
		ET1 = 0;
	}
}

unsigned int get_freq()
{
	static unsigned int frequence;
	if(wave_flag)
	{
		wave_flag=0;
		frequence=wave_count;
		wave_count=0;
		TR0 = 1;
		TR1 = 1;
		ET0 = 1;
		ET1 = 1;
	}
	return frequence;
}