#ifndef UART_H
#define UART_H

void uart_init();
void uart_putc(char c);
char uart_getc(void);
void uart_puts(const char* str);
void uart_hex(unsigned int d);
void uart_putc_wrapper(void* p, char c);

#endif // UART_H
