#include <stdint.h>

#ifndef PWM_H_
#define PWM_H_

extern volatile int32_t brightness_step;
void tim2_init(void);
void TIM2_IRQHandler(void);

#endif