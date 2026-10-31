#include <stdint.h>

extern uint32_t _estack;
extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss;
extern void SystemInit(void);
extern int main(void);

void Reset_Handler(void);
void Default_Handler(void);
void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));

__attribute__((section(".isr_vector")))
const uint32_t g_pfnVectors[] = {
    (uint32_t)&_estack,
    (uint32_t)Reset_Handler,
    (uint32_t)NMI_Handler,
    (uint32_t)HardFault_Handler,
    0, 0, 0, 0, 0, 0, 0,
    (uint32_t)SVC_Handler,
    0, 0,
    (uint32_t)PendSV_Handler,
    (uint32_t)SysTick_Handler,
};

void Default_Handler(void)
{
  while (1) {
  }
}

void SystemInit(void)
{
  *((volatile uint32_t *)0xE000ED08) = 0x08000000u;
}

void Reset_Handler(void)
{
  uint32_t *src = &_sidata;
  uint32_t *dst = &_sdata;
  while (dst < &_edata) {
    *dst++ = *src++;
  }
  for (dst = &_sbss; dst < &_ebss; dst++) {
    *dst = 0;
  }
  SystemInit();
  main();
  while (1) {
  }
}
