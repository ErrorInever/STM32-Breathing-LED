#include "pwm.h"
#include "main.h"
#include "uart.h"
#include <stdint.h>


#define PSC_VAL 84UL
#define ARR_VAL 1000UL
#define PWM_MODE_1 6UL

typedef enum {INHALING, PAUSE_UP, EXHALING, PAUSE_DOWN} BreathState; // state

volatile int32_t brightness_step = 1;   // brightness step for LED
const int32_t PAUSE_DURATION = 500;     // pause duration of breath



void tim2_init(void) {
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
  (void)RCC->APB1ENR;
  TIM2->PSC = PSC_VAL - 1UL;
  // PWM 1Khz: 1000 PWM full cycles in sec (each cycle has 1000 tick)
  // Summary: 1000 cycles * 1000 ticks = 1 million ticks in sec
  TIM2->ARR = ARR_VAL - 1UL;
  // Duty cycle. ARR = 1000, therefore 10% of 1000 = 100; 20% of 1000 = 200, etc. 
  TIM2->CCR1 = 0UL; // Start from 0%
  TIM2->CCMR1 &= ~TIM_CCMR1_OC1M_Msk; // CC1S output (0x00 by default)
  // OC1M PWM Mode 1 channel 1 (while CNT < CCR pin is 1, else 0)
  TIM2->CCMR1 |= (PWM_MODE_1 << TIM_CCMR1_OC1M_Pos);
  TIM2->CCMR1 |= TIM_CCMR1_OC1PE; // Enable preload
  // Enable CCER
  TIM2->CCER &= ~TIM_CCER_CC1E_Msk;
  TIM2->CCER |= TIM_CCER_CC1E; 
  // Enable generate interrupts (when CNT == ARR)
  TIM2->DIER &= ~TIM_DIER_UDE_Msk;
  TIM2->DIER |= TIM_DIER_UIE;
  HAL_NVIC_EnableIRQ(TIM2_IRQn); // enable interrupt in NVIC
  // Start timer
  TIM2->CR1 &= ~TIM_CR1_CEN_Msk;
  TIM2->CR1 |= TIM_CR1_CEN;
}

void TIM2_IRQHandler(void) {
  // if PWM pass 1 full cycle (CNT == ARR)
  static int32_t brightness = 0;
  static BreathState state = INHALING;
  static int32_t pause_timer = 0;

  if(TIM2->SR & TIM_SR_UIF) {
    TIM2->SR &= ~TIM_SR_UIF_Msk; // reset flag
    (void)TIM2->SR; // save delay
    // Breath states
    switch (state) {
      case INHALING:
        brightness += brightness_step;
        if(brightness >= 1000) {
          brightness = 1000;
          state = PAUSE_UP;
          USART2_log("state: PAUSE_UP\r\n");
          pause_timer = 0;
        }
        break;
      case PAUSE_UP:
        pause_timer++;
        if(pause_timer >= PAUSE_DURATION) {
          state = EXHALING;
          USART2_log("state: EXHALING\r\n");
        }
        break;
      case EXHALING:
        brightness -= brightness_step;
        if(brightness <= 0) {
          brightness = 0;
          state = PAUSE_DOWN;
          USART2_log("state: PAUSE_DOWN\r\n");
          pause_timer = 0;
        }
        break;
      case PAUSE_DOWN:
        pause_timer++;
        if(pause_timer >= PAUSE_DURATION) {
          state = INHALING;
          USART2_log("state: INHALING\r\n");
        }
        break;
    }
    TIM2->CCR1 = (uint32_t)brightness;
  }
}