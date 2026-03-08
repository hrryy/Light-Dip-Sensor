#include "../hal/rotary_hal.h"
#include "../hal/pwm_hal_linux.h"
#include "../src/rotary_control.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static volatile bool running = true;

void handler(int sig) {
    (void)sig;
    running = false;
}

int main(void)
{
    signal(SIGINT, handler);
    
    printf("Initializing system...\n");
    
    // Init PWM (uses GPIO12 automatically)
    if (!PWM_init()) {
        fprintf(stderr, "PWM init failed\n");
        return 1;
    }
    
    // Init rotary encoder (update GPIO lines!)
    if (!Rotary_init("/dev/gpiochip0", 6, 14)) {  // Change these!
        fprintf(stderr, "Rotary init failed\n");
        PWM_cleanup();
        return 1;
    }
    
    // Init rotary control
    if (!RotaryControl_init()) {
        fprintf(stderr, "Rotary control init failed\n");
        Rotary_cleanup();
        PWM_cleanup();
        return 1;
    }
    
    printf("\nSystem ready! Turn encoder to change LED frequency.\n\n");
    
    while (running) {
        sleep(1);
    }
    
    RotaryControl_cleanup();
    Rotary_cleanup();
    PWM_cleanup();
    return 0;
}