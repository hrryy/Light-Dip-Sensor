#include "../hal/pwm_hal_linux.h"
#include "../src/rotary_control.h"
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>

static volatile bool s_running = true;

void signal_handler(int signal)
{
    (void)signal;
    s_running = false;
}

int main(void)
{
    printf("========================================\n");
    printf("  Rotary Encoder + LED PWM Test\n");
    printf("  BeagleY-AI Complete System\n");
    printf("========================================\n");
    printf("Turn encoder to change LED frequency!\n");
    printf("Press Ctrl+C to exit\n\n");

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("Initializing PWM LED (GPIO12)...\n");
    if (!PWM_init()) {
        fprintf(stderr, "Failed to initialize PWM\n");
        fprintf(stderr, "Make sure you ran: sudo beagle-pwm-export --pin hat-32\n");
        return 1;
    }

    printf("Initializing rotary control (GPIO16/17 on gpiochip2)...\n");
    if (!RotaryControl_init()) {
        fprintf(stderr, "Failed to initialize rotary control\n");
        PWM_cleanup();
        return 1;
    }

    printf("\n✓ System ready!\n");
    printf("Turn encoder clockwise to increase frequency (0-500Hz)\n");
    printf("Turn encoder counter-clockwise to decrease frequency\n\n");

    int lastFreq = -1;
    while (s_running) {
        int freq = RotaryControl_getFrequency();
        
        if (freq != lastFreq) {
            printf("LED frequency: %3d Hz\r", freq);
            fflush(stdout);
            lastFreq = freq;
        }
        
        sleep(1);
    }

    printf("\n\nShutting down...\n");
    RotaryControl_cleanup();
    PWM_cleanup();
    
    printf("Shutdown complete\n");
    return 0;
}