#include "../hal/adc_hal.h"
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>

static volatile bool s_running = true;

void signal_handler(int signal)
{
    (void)signal;
    s_running = false;
}

static void sleepForMs(long long delayInMs)
{
    const long long NS_PER_MS = 1000 * 1000;
    const long long NS_PER_SECOND = 1000000000;
    long long delayNs = delayInMs * NS_PER_MS;
    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, NULL);
}

int main(void)
{
    printf("========================================\n");
    printf("  Potentiometer Test (Channel 1)\n");
    printf("========================================\n");
    printf("Turn the potentiometer knob!\n");
    printf("Press Ctrl+C to exit\n\n");

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (!ADC_init()) {
        fprintf(stderr, "Failed to initialize ADC\n");
        return 1;
    }

    printf("Reading potentiometer on MCP3208 Channel 1...\n\n");

    while (s_running) {
        int raw = ADC_readPotentiometerRaw();
        double voltage = ADC_readPotentiometerVoltage();

        if (raw >= 0 && voltage >= 0.0) {
            // Calculate percentage (0-100%)
            int percent = (raw * 100) / MCP3208_MAX_VALUE;

            // Map to LED frequency for preview (0-500Hz)
            int frequency = (raw * 500) / MCP3208_MAX_VALUE;

            // Visual bar (50 chars)
            int barLength = percent / 2;
            printf("\rPot: %4d (%.3fV) [%3d%%] LED: %3dHz [", 
                   raw, voltage, percent, frequency);
            for (int i = 0; i < 50; i++) {
                printf("%c", i < barLength ? '#' : ' ');
            }
            printf("]");
            fflush(stdout);
        } else {
            printf("\rError reading potentiometer                    ");
            fflush(stdout);
        }

        sleepForMs(50);  // 50ms refresh
    }

    printf("\n\nShutdown complete\n");
    ADC_cleanup();
    return 0;
}