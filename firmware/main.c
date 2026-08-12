#include <stdint.h>

#define RCC_APB2ENR (*(volatile uint32_t*)0x40021018)
#define GPIOC_CRH   (*(volatile uint32_t*)0x40011004)
#define GPIOC_ODR   (*(volatile uint32_t*)0x4001100C)
#define GPIOA_CRH   (*(volatile uint32_t*)0x40010804)

#define USART1_SR   (*(volatile uint32_t*)0x40013800)
#define USART1_DR   (*(volatile uint32_t*)0x40013804)
#define USART1_BRR  (*(volatile uint32_t*)0x40013808)
#define USART1_CR1  (*(volatile uint32_t*)0x4001380C)

void delay(volatile uint32_t count) {
    while (count--) {}
}

void uart_init(void) {
    RCC_APB2ENR |= (1 << 2);
    RCC_APB2ENR |= (1 << 14);

    GPIOA_CRH &= ~(0xF << 4);
    GPIOA_CRH |=  (0xB << 4);

    USART1_BRR = 0x0341;
    USART1_CR1 = (1 << 13) | (1 << 3);
}

void uart_send_char(char c) {
    while (!(USART1_SR & (1 << 7))) {}
    USART1_DR = c;
}

void uart_send_string(const char* s) {
    while (*s) {
        uart_send_char(*s++);
    }
}

int main(void) {
    RCC_APB2ENR |= (1 << 4);
    GPIOC_CRH &= ~(0xF << 20);
    GPIOC_CRH |=  (0x2 << 20);

    uart_init();

    uint32_t counter = 0;
    while (1) {
        GPIOC_ODR ^= (1 << 13);
        uart_send_string("Merhaba dunya! Sayac: ");
        
        char buf[12];
        int i = 0;
        uint32_t n = counter;
        if (n == 0) buf[i++] = '0';
        while (n > 0) {
            buf[i++] = '0' + (n % 10);
            n /= 10;
        }
        while (i > 0) uart_send_char(buf[--i]);
        uart_send_string("\r\n");
        
        counter++;
        delay(500000);
    }
}

void _start(void) { main(); }
