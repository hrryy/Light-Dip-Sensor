#include "../src/sampler.h"
#include "../src/periodTimer.h"
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
    printf("========================================\n");
    printf("  Sampler Module Test\n");
    printf("========================================\n");
    printf("Sampling light at ~1ms intervals\n");
    printf("Press Ctrl+C to exit\n\n");

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize sampler
    if (!Sampler_init()) {
        fprintf(stderr, "Failed to initialize sampler\n");
        return EXIT_FAILURE;
    }

    printf("Collecting data...\n\n");

    // Main loop - print statistics every second
    int secondCount = 0;
    while (s_running) {
        // Sleep for 1 second
        sleepForMs(1000);
        
        // Move current data to history
        Sampler_moveCurrentDataToHistory();

        // Get timing statistics
        Period_statistics_t stats;
        Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, &stats);

        // Get sampler data
        int historySize = Sampler_getHistorySize();
        double avgVoltage = Sampler_getAverageReading();
        long long totalSamples = Sampler_getNumSamplesTaken();

        // Print statistics
        printf("Second %3d: #Smpl=%4d, TotalSmpl=%lld, AvgV=%.3fV, "
               "Timing ms[%.3f, %.3f] avg %.3f/%d\n",
               ++secondCount,
               historySize,
               totalSamples,
               avgVoltage,
               stats.minPeriodInMs,
               stats.maxPeriodInMs,
               stats.avgPeriodInMs,
               stats.numSamples);

        // Get and display 10 evenly-spaced samples from history
        int size;
        double *history = Sampler_getHistory(&size);
        if (history != NULL && size > 0) {
            printf("  Samples: ");
            for (int i = 0; i < 10 && i < size; i++) {
                int index = (i * size) / 10;
                printf("%d:%.3f ", index, history[index]);
            }
            printf("\n");
            free(history);
        }
        printf("\n");
    }

    printf("\nShutting down...\n");
    Sampler_cleanup();
    
    return EXIT_SUCCESS;
}