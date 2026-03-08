#include "sampler.h"
#include "../hal/adc_hal.h"
#include "periodTimer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

#define MAX_SAMPLES_PER_SECOND 2000

static pthread_t s_samplingThread;
static pthread_mutex_s_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool s_running = false;

// Current samples being collected
static double s_currentSamples[MAX_SAMPLES_PER_SECOND];
static int s_currentCount = 0;

// History (previous second's samples)
static double s_historySamples[MAX_SAMPLES_PER_SECOND];
static int s_historyCount = 0;

// Statistics
static long long s_totalSamplesTaken = 0;
static double s_averageReading = 0.0;
static bool s_firstSample = true;

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

static void* samplingThread(void* arg)
{
    (void)arg;
    printf("Sampler thread started\n");

    while (s_running) {
        // Read ADC
        double voltage = ADC_readVoltage();

        pthread_mutex_lock(&s_mutex);

        // Store in current samples
        if (s_currentCount < MAX_SAMPLES_PER_SECOND) {
            s_currentSamples[s_currentCount++] = voltage;
        }

        // Update statistics
        s_totalSamplesTaken++;

        // Exponential smoothing (weight previous average 99.9%)
        if (s_firstSample) {
            s_averageReading = voltage;
            s_firstSample = false;
        } else {
            s_averageReading = (0.999 * s_averageReading) + (0.001 * voltage);
        }

        // Mark timing event
        Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);

        pthread_mutex_unlock(&s_mutex);

        // Sleep 1ms between samples
        sleepForMs(1);
    }

    printf("Sampler thread stopped\n");
    return NULL;
}

void Sampler_init(void)
{
    printf("Initializing sampler...\n");

    // Initialize ADC
    if (!ADC_init()) {
        fprintf(stderr, "Sampler_init: Failed to initialize ADC\n");
        return;
    }

    // Initialize period timer
    Period_init();

    // Start sampling thread
    s_running = true;
    if (pthread_create(&s_samplingThread, NULL, samplingThread, NULL) != 0) {
        perror("Sampler_init: Failed to create thread");
        ADC_cleanup();
        return;
    }

    printf("Sampler initialized\n");
}

void Sampler_cleanup(void)
{
    if (s_running) {
        s_running = false;
        pthread_join(s_samplingThread, NULL);
        ADC_cleanup();
        Period_cleanup();
        printf("Sampler cleanup complete\n");
    }
}

void Sampler_moveCurrentDataToHistory(void)
{
    pthread_mutex_lock(&s_mutex);

    // Copy current samples to history
    memcpy(s_historySamples, s_currentSamples, s_currentCount * sizeof(double));
    s_historyCount = s_currentCount;

    // Reset current samples
    s_currentCount = 0;

    pthread_mutex_unlock(&s_mutex);
}

int Sampler_getHistorySize(void)
{
    int size;
    pthread_mutex_lock(&s_mutex);
    size = s_historyCount;
    pthread_mutex_unlock(&s_mutex);
    return size;
}

double* Sampler_getHistory(int *size)
{
    pthread_mutex_lock(&s_mutex);

    *size = s_historyCount;
    double* copy = malloc(s_historyCount * sizeof(double));
    if (copy) {
        memcpy(copy, s_historySamples, s_historyCount * sizeof(double));
    }

    pthread_mutex_unlock(&s_mutex);
    return copy;
}

double Sampler_getAverageReading(void)
{
    double avg;
    pthread_mutex_lock(&s_mutex);
    avg = s_averageReading;
    pthread_mutex_unlock(&s_mutex);
    return avg;
}

long long Sampler_getNumSamplesTaken(void)
{
    long long count;
    pthread_mutex_lock(&s_mutex);
    count = s_totalSamplesTaken;
    pthread_mutex_unlock(&s_mutex);
    return count;
}
