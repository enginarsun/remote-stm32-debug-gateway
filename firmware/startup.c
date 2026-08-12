#include <stdint.h>

extern void _start(void);
extern uint32_t _estack;

void Reset_Handler(void) {
    _start();
}

void Default_Handler(void) {
    while (1) {}
}

__attribute__((section(".isr_vector")))
void (* const vector_table[])(void) = {
    (void(*)(void))&_estack,
    Reset_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    0, 0, 0, 0,
    Default_Handler,
    Default_Handler,
    0,
    Default_Handler,
    Default_Handler,
};
