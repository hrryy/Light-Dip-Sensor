#include "src/sampler.h"
#include "src/dip_detector.h"
#include "src/periodTimer.h"
#include "src/udp_server.h"
#include "hal/pwm_hal_linux.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>

static volatile bool s_running = true;
static int s_ledFrequency = 10;  // Fixed 10Hz for now

void signalHandler(int sig)
{
    (void)sig;
    printf("\nShutting down...\n");
    s_running = false;
}

int main(void)
{
    printf("========================================\n");
    printf("  Light Sampler - Assignment 2\n");
    printf("========================================\n\n");

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Initialize PWM LED
    printf("Initializing PWM LED...\n");
    if (!PWM_init()) {
        fprintf(stderr, "Failed to initialize PWM\n");
        return 1;
    }
    PWM_setFrequency(s_ledFrequency);

    // Initialize sampler
    Sampler_init();

    // Initialize UDP server
    UDPServer_init();

    printf("\n✓ System ready!\n");
    printf("LED flashing at %d Hz\n", s_ledFrequency);
    printf("Sampling light levels...\n\n");

    // Main loop - runs every second
    while (s_running) {
        sleep(1);

        // Move current samples to history
        Sampler_moveCurrentDataToHistory();

        // Get stats
        int historySize = Sampler_getHistorySize();
        double avgLight = Sampler_getAverageReading();
        long long totalSamples = Sampler_getNumSamplesTaken();

        // Get timing stats
        Period_statistics_t stats = Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT);

        // Count dips
        int dips = DipDetector_countDips(avgLight);

        // Print to terminal
        printf("#Smpl/s = %4d Flash @ %dHz avg = %.3fV dips = %2d Smpl ms[%.3f, %.3f] avg %.3f/%d\n",
               historySize, s_ledFrequency, avgLight, dips,
               stats.minPeriodInMs, stats.maxPeriodInMs, stats.avgPeriodInMs, stats.numSamples);

        // TODO: Print 10 evenly spaced samples
    }

    // Cleanup
    printf("\nCleaning up...\n");
    UDPServer_cleanup();
    Sampler_cleanup();
    PWM_cleanup();

    printf("Shutdown complete\n");
    return 0;
}
