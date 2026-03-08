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
    printf("\nReceived signal, shutting down...\n");
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
    printf("=================================\n");
    printf("  MCP3208 Light Sensor Test\n");
    printf("=================================\n");
    printf("Using SPI device: /dev/spidev0.0\n");
    printf("Light sensor on channel: %d\n", LIGHT_SENSOR_CHANNEL);
    printf("Press Ctrl+C to exit\n\n");

    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize ADC
    if (!ADC_init()) {
        fprintf(stderr, "Failed to initialize ADC\n");
        return EXIT_FAILURE;
    }

    printf("ADC initialized successfully!\n");
    printf("Sampling light sensor...\n\n");

    // Sample counter
    int sample_count = 0;
    int error_count = 0;

    // Continuously read light sensor
    while (s_running) {
        int raw = ADC_readLightRaw();
        double voltage = ADC_readLightVoltage();
        
        if (raw >= 0 && voltage >= 0.0) {
            // Display with bar graph for visualization
            int bar_length = (int)((voltage / ADC_VREF) * 50);
            
            printf("\rSample %05d | Raw: %4d | Voltage: %.3fV | [", 
                   sample_count, raw, voltage);
            
            for (int i = 0; i < 50; i++) {
                printf("%c", i < bar_length ? '#' : ' ');
            }
            printf("]");
            fflush(stdout);
            
            sample_count++;
        } else {
            error_count++;
            printf("\nError reading ADC (error #%d)\n", error_count);
        }
        
        sleepForMs(50);  // 50ms = 20 Hz sampling
    }

    printf("\n\n=================================\n");
    printf("Statistics:\n");
    printf("  Total samples: %d\n", sample_count);
    printf("  Errors: %d\n", error_count);
    printf("=================================\n");

    ADC_cleanup();
    
    return EXIT_SUCCESS;
}