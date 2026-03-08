#include "rotary_control.h"
#include "../hal/rotary_hal.h"
#include "../hal/pwm_hal_linux.h"
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

// GPIO configuration for BeagleY-AI
// GPIO16 and GPIO17 are on gpiochip2, lines 7 and 8
#define ROTARY_GPIO_CHIP "gpiochip2"
#define ROTARY_CLK_LINE 7   // GPIO16
#define ROTARY_DT_LINE 8    // GPIO17

// Frequency control
#define INITIAL_FREQUENCY_HZ 10
#define FREQUENCY_STEP_HZ 1

static pthread_t s_thread;
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool s_running = false;
static int s_currentFrequency = INITIAL_FREQUENCY_HZ;

// Sleep function using nanosleep
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

static void* rotaryControlThread(void* arg)
{
    (void)arg;
    
    printf("Rotary control thread started\n");
    
    while (s_running) {
        RotaryDirection_t direction = Rotary_read();
        
        if (direction != ROTARY_NONE) {
            pthread_mutex_lock(&s_mutex);
            
            int newFreq = s_currentFrequency;
            
            if (direction == ROTARY_CW) {
                newFreq += FREQUENCY_STEP_HZ;
                printf("Rotary: CW → Freq = %d Hz\n", newFreq);
            } else if (direction == ROTARY_CCW) {
                newFreq -= FREQUENCY_STEP_HZ;
                printf("Rotary: CCW → Freq = %d Hz\n", newFreq);
            }
            
            // Clamp frequency
            if (newFreq < PWM_MIN_FREQ_HZ) newFreq = PWM_MIN_FREQ_HZ;
            if (newFreq > PWM_MAX_FREQ_HZ) newFreq = PWM_MAX_FREQ_HZ;
            
            // Only update if frequency changed
            if (newFreq != s_currentFrequency) {
                s_currentFrequency = newFreq;
                PWM_setFrequency(s_currentFrequency);
            }
            
            pthread_mutex_unlock(&s_mutex);
        }
        
        sleepForMs(1);  // 1ms polling
    }
    
    printf("Rotary control thread stopped\n");
    return NULL;
}

bool RotaryControl_init(void)
{
    printf("Initializing rotary control...\n");
    
    // Initialize rotary encoder (gpiochip2, lines 7 and 8)
    if (!Rotary_init(ROTARY_GPIO_CHIP, ROTARY_CLK_LINE, ROTARY_DT_LINE)) {
        fprintf(stderr, "RotaryControl_init: Failed to initialize rotary encoder\n");
        return false;
    }
    
    // Set initial frequency
    s_currentFrequency = INITIAL_FREQUENCY_HZ;
    
    // Start monitoring thread
    s_running = true;
    if (pthread_create(&s_thread, NULL, rotaryControlThread, NULL) != 0) {
        perror("RotaryControl_init: Failed to create thread");
        Rotary_cleanup();
        return false;
    }
    
    printf("Rotary control initialized (Step: %d Hz per click)\n", FREQUENCY_STEP_HZ);
    return true;
}

void RotaryControl_cleanup(void)
{
    if (s_running) {
        s_running = false;
        pthread_join(s_thread, NULL);
        Rotary_cleanup();
        printf("Rotary control cleanup complete\n");
    }
}

int RotaryControl_getFrequency(void)
{
    int freq;
    pthread_mutex_lock(&s_mutex);
    freq = s_currentFrequency;
    pthread_mutex_unlock(&s_mutex);
    return freq;
}

void RotaryControl_setFrequency(int frequencyHz)
{
    pthread_mutex_lock(&s_mutex);
    
    if (frequencyHz < PWM_MIN_FREQ_HZ) frequencyHz = PWM_MIN_FREQ_HZ;
    if (frequencyHz > PWM_MAX_FREQ_HZ) frequencyHz = PWM_MAX_FREQ_HZ;
    
    s_currentFrequency = frequencyHz;
    PWM_setFrequency(s_currentFrequency);
    
    pthread_mutex_unlock(&s_mutex);
}
