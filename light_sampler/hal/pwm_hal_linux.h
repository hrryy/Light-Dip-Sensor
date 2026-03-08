
#ifndef _PWM_HAL_LINUX_H_
#define _PWM_HAL_LINUX_H_

#include <stdbool.h>

#define PWM_MIN_FREQ_HZ 0
#define PWM_MAX_FREQ_HZ 500

/**
 * Initialize PWM using Linux sysfs interface
 * Uses GPIO12 (HAT pin 32) - LED Emitter
 */
bool PWM_init(void);

/**
 * Cleanup PWM
 */
void PWM_cleanup(void);

/**
 * Set PWM frequency
 */
void PWM_setFrequency(int frequencyHz);

/**
 * Get current PWM frequency
 */
int PWM_getFrequency(void);

#endif