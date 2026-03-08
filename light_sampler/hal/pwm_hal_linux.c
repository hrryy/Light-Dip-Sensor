#include "pwm_hal_linux.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#define PWM_PATH "/dev/hat/pwm/GPIO12"
#define PWM_PERIOD_FILE PWM_PATH "/period"
#define PWM_DUTY_FILE PWM_PATH "/duty_cycle"
#define PWM_ENABLE_FILE PWM_PATH "/enable"

static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
static int s_frequency = 10;
static bool s_initialized = false;

// Helper to write to a sysfs file
static bool writeFile(const char* path, const char* value)
{
    FILE* file = fopen(path, "w");
    if (!file) {
        perror("PWM: Failed to open file");
        return false;
    }
    
    fprintf(file, "%s", value);
    fclose(file);
    return true;
}

bool PWM_init(void)
{
    printf("PWM initializing on GPIO12...\n");
    
    // Disable first
    writeFile(PWM_ENABLE_FILE, "0");
    
    // Set initial frequency (10Hz)
    s_frequency = 10;
    PWM_setFrequency(s_frequency);
    
    // Enable
    if (!writeFile(PWM_ENABLE_FILE, "1")) {
        fprintf(stderr, "PWM: Failed to enable\n");
        return false;
    }
    
    s_initialized = true;
    printf("PWM initialized at %d Hz\n", s_frequency);
    return true;
}

void PWM_cleanup(void)
{
    if (s_initialized) {
        writeFile(PWM_ENABLE_FILE, "0");
        s_initialized = false;
    }
    printf("PWM cleanup complete\n");
}

void PWM_setFrequency(int frequencyHz)
{
    pthread_mutex_lock(&s_mutex);
    
    // Clamp frequency
    if (frequencyHz < PWM_MIN_FREQ_HZ) frequencyHz = PWM_MIN_FREQ_HZ;
    if (frequencyHz > PWM_MAX_FREQ_HZ) frequencyHz = PWM_MAX_FREQ_HZ;
    
    if (frequencyHz == 0) {
        // Turn off
        writeFile(PWM_ENABLE_FILE, "0");
        s_frequency = 0;
    } else {
        // Calculate period in nanoseconds
        // Period = 1/frequency (in seconds) = 1,000,000,000/frequency (in ns)
        long long period_ns = 1000000000LL / frequencyHz;
        long long duty_ns = period_ns / 2;  // 50% duty cycle
        
        char buffer[32];
        
        // Disable first
        writeFile(PWM_ENABLE_FILE, "0");
        
        // Set duty to 0
        writeFile(PWM_DUTY_FILE, "0");
        
        // Set period
        snprintf(buffer, sizeof(buffer), "%lld", period_ns);
        writeFile(PWM_PERIOD_FILE, buffer);
        
        // Set duty cycle (50%)
        snprintf(buffer, sizeof(buffer), "%lld", duty_ns);
        writeFile(PWM_DUTY_FILE, buffer);
        
        // Enable
        writeFile(PWM_ENABLE_FILE, "1");
        
        s_frequency = frequencyHz;
        printf("PWM frequency set to %d Hz\n", s_frequency);
    }
    
    pthread_mutex_unlock(&s_mutex);
}

int PWM_getFrequency(void)
{
    int freq;
    pthread_mutex_lock(&s_mutex);
    freq = s_frequency;
    pthread_mutex_unlock(&s_mutex);
    return freq;
}