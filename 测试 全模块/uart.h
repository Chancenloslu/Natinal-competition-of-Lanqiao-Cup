#ifndef _UART_H
#define _UART_H

extern unsigned char rx_sbuf[6];
extern unsigned char rx_index;


void SendString(char *s);
void SendData(unsigned char dat);
void UartInit(void);		//9600bps@11.0592MHz

bit judge(unsigned char a[],unsigned char b[]);
#endif