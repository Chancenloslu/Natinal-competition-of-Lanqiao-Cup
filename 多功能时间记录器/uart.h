#ifndef __UART_H_
#define __UART_H_

extern unsigned char dat_rev[6];
extern unsigned char sbuf_index;

void SendData(unsigned char dat);
void SendString(char *s);
void UartInit(void);		//9600bps@11.0592MHz
bit Judge_revdat(unsigned char a[],unsigned char b[]);


#endif