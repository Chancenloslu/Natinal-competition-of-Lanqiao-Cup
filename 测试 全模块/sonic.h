#ifndef __SONIC_H__
#define __SONIC_H__

sbit TX=P1^0;
sbit RX=P1^1;
unsigned int get_distance(void);														//���ܣ�������򿪼�ʱ��1��ֱ����⵽�������RX=0
void send_sonic(void);															//��������8������

#endif