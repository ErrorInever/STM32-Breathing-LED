#include "main.h"
#include <stdint.h>

volatile uint32_t msticks = 0;         // ms ticks for delay

void sysTick_init(void) {
  // interrupt each 1ms. therefore LOAD = 84Mhz / 1000ms. 84000 ticks for one ms.
  uint32_t N = 84000UL;
  SysTick->LOAD = N - 1UL;
  SysTick->VAL = 0UL;
  // Get MHz from CPU, enable interrupt, start countdown
  SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk |
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_CLKSOURCE_Msk;
}

void delay_ms(uint32_t ms) { 
  uint32_t start_ms = msticks;
  while((msticks - start_ms) < ms);
}

void SysTick_Handler(void) {
  msticks++;
}