#include "stc15f2k60s2.h"
#include "sonic.h"
#include "intrins.h"
void Delay5us1()		//@11.0592MHz
{
	unsigned char i;

	_nop_();
	i = 11;
	while (--i);
}

#define somenop Delay5us1()

void send_sonic()															//连续发送8个脉冲
{
	unsigned char i=8;
	do
	{
		TX=1;
		somenop;somenop;somenop;somenop;somenop;
		TX=0;
		somenop;somenop;somenop;somenop;somenop;
	}while(i--);
}

unsigned int get_distance()														//接受，发送完打开计时器1，直到检测到溢出或者RX=0
{
	unsigned int time;
	unsigned int distance;
	send_sonic();
	TR1=1;
	while(RX==1&&TF1==0);
	TR1=0;
	if(TF1==1)
	{
		TF1 = 0;
		distance=999;
	}
	else
	{
		time=TL1;
		time<<=8;
		time|=TH1;
		distance = (unsigned int)time*0.017;
	}
	TH1=0;
	TL1=0;
	return distance;
}