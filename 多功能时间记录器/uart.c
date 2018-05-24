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
	SCON = 0x50;		//8λ����,�ɱ䲨����
	AUXR |= 0x40;		//��ʱ��1ʱ��ΪFosc,��1T
	AUXR &= 0xFE;		//����1ѡ��ʱ��1Ϊ�����ʷ�����
	TMOD &= 0x0F;		//�趨��ʱ��1Ϊ16λ�Զ���װ��ʽ
	TL1 = 0x00;		//�趨��ʱ��ֵ
	TH1 = 0xF7;		//�趨��ʱ��ֵ
	ET1 = 0;		//��ֹ��ʱ��1�ж�
	TR1 = 1;		//������ʱ��1
	ES = 1;
	EA = 1;
	SendString("STC15F2K60S2\r\nUart Test !\r\n");
}

/*----------------------------
���ʹ�������
----------------------------*/
void SendData(unsigned char dat)
{
    while (busy);               //�ȴ�ǰ������ݷ������
    busy = 1;
    SBUF = dat;                 //д���ݵ�UART���ݼĴ���
}

/*----------------------------
�����ַ���
----------------------------*/
void Uart() interrupt 4 using 2
{
    if (RI)
    {
        RI = 0;                 //���RIλ
        dat_rev[sbuf_index] = SBUF;
				sbuf_index++;
//        P22 = RB8;              //P2.2��ʾУ��λ
    }
    if (TI)
    {
        TI = 0;                 //���TIλ
        busy = 0;               //��æ��־
    }
}


void SendString(char *s)
{
    while (*s)                  //����ַ���������־
    {
        SendData(*s++);         //���͵�ǰ�ַ�
    }
}
