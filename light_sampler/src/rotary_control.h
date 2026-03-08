#ifndef _ROTARY_CONTROL_H_
#define _ROTARY_CONTROL_H_

#include <stdbool.h>

/**
 * Initialize rotary control module
 * Starts background thread to monitor encoder and control PWM
 */
bool RotaryControl_init(void);

/**
 * Cleanup rotary control module
 */
void RotaryControl_cleanup(void);

/**
 * Get current LED frequency
 */
int RotaryControl_getFrequency(void);

/**
 * Set LED frequency manually
 */
void RotaryControl_setFrequency(int frequencyHz);

#endif // _ROTARY_CONTROL_H_