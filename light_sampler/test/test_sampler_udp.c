#include "../src/sampler.h"
#include "../src/udp_server.h"
#include "../src/periodTimer.h"
#include "../src/dip_detector.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>

volatile bool g_running = true;

void signal_handler(int signal)
{
    (void)signal;
    g_running = false;
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
    printf("  Light Sampler with UDP Server\n");
    printf("========================================\n");
    printf("Sampling light at ~1ms intervals\n");
    printf("UDP server on port %d\n", UDP_PORT);
    printf("Press Ctrl+C to exit\n\n");

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize sampler
    if (!Sampler_init()) {
        fprintf(stderr, "Failed to initialize sampler\n");
        return EXIT_FAILURE;
    }

    // Initialize UDP server
    if (!UdpServer_init()) {
        fprintf(stderr, "Failed to initialize UDP server\n");
        Sampler_cleanup();
        return EXIT_FAILURE;
    }

    printf("System initialized. Ready for UDP commands!\n\n");

    // Main loop - print statistics every second
    int secondCount = 0;
    while (g_running) {
        sleepForMs(1000);
        
        Sampler_moveCurrentDataToHistory();

        Period_statistics_t stats;
        Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, &stats);

        int historySize = Sampler_getHistorySize();
        double avgVoltage = Sampler_getAverageReading();
        long long totalSamples = Sampler_getNumSamplesTaken();

        // Get dip count
int size;
double* history = Sampler_getHistory(&size);
int dips = 0;
if (history != NULL && size > 0) {
    dips = DipDetector_countDips(history, size, avgVoltage);
    free(history);
}

printf("Second %3d: #Smpl=%4d, TotalSmpl=%lld, AvgV=%.3fV, Dips=%2d, "
       "Timing ms[%.3f, %.3f] avg %.3f/%d\n",
       ++secondCount,
       historySize,
       totalSamples,
       avgVoltage,
       dips,
       stats.minPeriodInMs,
       stats.maxPeriodInMs,
       stats.avgPeriodInMs,
       stats.numSamples);
    }

    printf("\nShutting down...\n");
    UdpServer_cleanup();
    Sampler_cleanup();
    
    return EXIT_SUCCESS;
}