
#include "gpio_driver.h"
#include "pwm.h"
#include "rcc.h"
#include "uart.h"
#include "systick.h"
#include <stdbool.h>

// TODO 
// brighness step button too small

int main(void) {
  SystemClock_Config();
  led_init();
  button_init();
  button_interrupt_init();
  sysTick_init();
  tim2_init();
  USART2_init();

  while (1) {
   if(needs_log) {
    UART_Printf("Brightness step update: %d\r\n", brightness_step);
    needs_log = false;
   }
  }
}
