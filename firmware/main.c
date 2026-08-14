#include <stdint.h>

#define RCC_APB2ENR (*(volatile uint32_t*)0x40021018)
#define GPIOC_CRH   (*(volatile uint32_t*)0x40011004)
#define GPIOC_ODR   (*(volatile uint32_t*)0x4001100C)
#define GPIOA_CRL   (*(volatile uint32_t*)0x40010800)
#define GPIOA_IDR   (*(volatile uint32_t*)0x40010808)
#define GPIOA_ODR   (*(volatile uint32_t*)0x4001080C)
#define GPIOA_BSRR  (*(volatile uint32_t*)0x40010810)
#define GPIOA_CRH   (*(volatile uint32_t*)0x40010804)

#define USART1_SR   (*(volatile uint32_t*)0x40013800)
#define USART1_DR   (*(volatile uint32_t*)0x40013804)
#define USART1_BRR  (*(volatile uint32_t*)0x40013808)
#define USART1_CR1  (*(volatile uint32_t*)0x4001380C)

#define STK_CTRL    (*(volatile uint32_t*)0xE000E010)
#define STK_LOAD    (*(volatile uint32_t*)0xE000E014)
#define STK_VAL     (*(volatile uint32_t*)0xE000E018)

void delay(volatile uint32_t count) {
    while (count--) {}
}

void systick_init(void) {
    STK_LOAD = 0xFFFFFF;
    STK_CTRL = (1 << 0) | (1 << 2);
}

void delay_us(uint32_t us) {
    uint32_t start = STK_VAL;
    uint32_t ticks = us * 8;
    while ((start - STK_VAL) < ticks) {
        if (STK_VAL > start) {
            start = STK_VAL;
        }
    }
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

void uart_send_int(int32_t n) {
    char buf[12];
    int i = 0;
    uint8_t neg = 0;

    if (n < 0) {
        neg = 1;
        n = -n;
    }
    if (n == 0) buf[i++] = '0';
    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    if (neg) uart_send_char('-');
    while (i > 0) uart_send_char(buf[--i]);
}

void dht_pin_output(void) {
    GPIOA_CRL &= ~(0xF << 0);
    GPIOA_CRL |=  (0x3 << 0);
}

void dht_pin_input(void) {
    GPIOA_CRL &= ~(0xF << 0);
    GPIOA_CRL |=  (0x8 << 0);
}

void dht_pin_high(void) {
    GPIOA_BSRR = (1 << 0);
}

void dht_pin_low(void) {
    GPIOA_BSRR = (1 << 16);
}

uint8_t dht_pin_read(void) {
    return (GPIOA_IDR & (1 << 0)) ? 1 : 0;
}

uint8_t dht_read(uint8_t* humidity, uint8_t* temperature) {
    uint8_t data[5] = {0, 0, 0, 0, 0};
    uint32_t timeout;

    dht_pin_output();
    dht_pin_low();
    delay(1800000);
    dht_pin_high();
    delay_us(30);
    dht_pin_input();

    timeout = 10000;
    while (dht_pin_read() == 1) {
        if (--timeout == 0) return 1;
    }
    timeout = 10000;
    while (dht_pin_read() == 0) {
        if (--timeout == 0) return 2;
    }
    timeout = 10000;
    while (dht_pin_read() == 1) {
        if (--timeout == 0) return 3;
    }

    for (int i = 0; i < 40; i++) {
        timeout = 10000;
        while (dht_pin_read() == 0) {
            if (--timeout == 0) return 4;
        }

        delay_us(40);
        uint8_t bit_val = dht_pin_read();

        timeout = 10000;
        while (dht_pin_read() == 1) {
            if (--timeout == 0) return 5;
        }

        data[i / 8] <<= 1;
        if (bit_val) {
            data[i / 8] |= 1;
        }
    }

    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        return 6;
    }

    *humidity = data[0];
    *temperature = data[2];
    return 0;
}

int main(void) {
    RCC_APB2ENR |= (1 << 4);
    GPIOC_CRH &= ~(0xF << 20);
    GPIOC_CRH |=  (0x2 << 20);

    RCC_APB2ENR |= (1 << 2);

    systick_init();
    uart_init();

    uart_send_string("STM32 DHT11 Sensoru Baslatildi\r\n");

    delay(5000000);

    while (1) {
        GPIOC_ODR ^= (1 << 13);

        uint8_t humidity = 0, temperature = 0;
        uint8_t result = dht_read(&humidity, &temperature);

        if (result == 0) {
            uart_send_string("Nem=");
            uart_send_int(humidity);
            uart_send_string("% Sicaklik=");
            uart_send_int(temperature);
            uart_send_string(" C\r\n");
        } else {
            uart_send_string("DHT okuma hatasi, kod=");
            uart_send_int(result);
            uart_send_string("\r\n");
        }

        delay(3000000);
    }
}

void _start(void) { main(); }
