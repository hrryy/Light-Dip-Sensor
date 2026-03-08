#include "../hal/adc_hal.h"
#include <stdio.h>
#include <stdlib.h>
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
    nanosleep(&reqDelay, (struct timespec *) NULL);
}

int main(void)
{
    printf("Testing ALL MCP3208 Channels\n");
    printf("============================\n\n");

    signal(SIGINT, signal_handler);

    if (!ADC_init()) {
        fprintf(stderr, "Failed to initialize ADC\n");
        return EXIT_FAILURE;
    }

    int sample_count = 0;

    while (s_running) {
        printf("\rSample #%05d: ", sample_count++);
        
        for (int ch = 0; ch < MCP3208_MAX_CHANNELS; ch++) {
            double voltage = ADC_readVoltage(ch);
            
            if (voltage >= 0.0) {
                printf("CH%d:%.3fV ", ch, voltage);
            } else {
                printf("CH%d:ERROR ", ch);
            }
        }
        
        fflush(stdout);
        sleepForMs(100);  // 100ms
    }

    printf("\n\nShutdown complete\n");
    ADC_cleanup();
    
    return EXIT_SUCCESS;
}