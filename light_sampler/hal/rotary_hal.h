#ifndef _ROTARY_HAL_H_
#define _ROTARY_HAL_H_

#include <stdbool.h>

typedef enum {
    ROTARY_NONE = 0,
    ROTARY_CW = 1,
    ROTARY_CCW = 2
} RotaryDirection_t;

/**
 * Initialize rotary encoder
 * @param gpioChip - GPIO chip name (e.g., "gpiochip2")
 * @param clkLine - GPIO line number for CLK signal (e.g., 7 for GPIO16)
 * @param dtLine - GPIO line number for DT signal (e.g., 8 for GPIO17)
 * @return true on success, false on failure
 */
bool Rotary_init(const char* gpioChip, unsigned int clkLine, unsigned int dtLine);

/**
 * Cleanup rotary encoder resources
 */
void Rotary_cleanup(void);

/**
 * Read current rotary encoder direction
 * Must be called frequently (e.g., every 1ms)
 * @return ROTARY_CW, ROTARY_CCW, or ROTARY_NONE
 */
RotaryDirection_t Rotary_read(void);

/**
 * Get current position (cumulative count of rotations)
 * @return Current position counter
 */
int Rotary_getPosition(void);

/**
 * Reset position counter to zero
 */
void Rotary_resetPosition(void);

#endif // _ROTARY_HAL_H_