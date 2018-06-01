#ifndef __SONIC_H__
#define __SONIC_H__

sbit TX=P1^0;
sbit RX=P1^1;
unsigned int get_distance(void);														//接受，发送完打开计时器1，直到检测到溢出或者RX=0
void send_sonic(void);															//连续发送8个脉冲

#endif