#include "../hal/rotary_hal.h"
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
    printf("  Rotary Encoder Test\n");
    printf("  BeagleY-AI GPIO Configuration\n");
    printf("========================================\n");
    printf("Turn the rotary encoder!\n");
    printf("Press Ctrl+C to exit\n\n");

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // GPIO16 and GPIO17 are on gpiochip2, lines 7 and 8
    const char* gpioChip = "gpiochip2";
    unsigned int clkLine = 7;   // GPIO16
    unsigned int dtLine = 8;    // GPIO17

    printf("Configuration:\n");
    printf("  GPIO Chip: %s\n", gpioChip);
    printf("  CLK: line %u (GPIO16)\n", clkLine);
    printf("  DT:  line %u (GPIO17)\n", dtLine);
    printf("\n");

    if (!Rotary_init(gpioChip, clkLine, dtLine)) {
        fprintf(stderr, "\n❌ Failed to initialize rotary encoder\n\n");
        fprintf(stderr, "Troubleshooting:\n");
        fprintf(stderr, "1. Verify GPIO lines:\n");
        fprintf(stderr, "   gpioinfo | grep -E 'GPIO1[67]'\n");
        fprintf(stderr, "2. Test reading manually:\n");
        fprintf(stderr, "   gpioget gpiochip2 7 8\n");
        fprintf(stderr, "3. Check wiring:\n");
        fprintf(stderr, "   OUT A → GPIO16 (gpiochip2 line 7)\n");
        fprintf(stderr, "   OUT B → GPIO17 (gpiochip2 line 8)\n");
        fprintf(stderr, "   GND   → GND\n");
        return 1;
    }

    printf("✓ Rotary encoder initialized!\n");
    printf("Turn the encoder clockwise or counter-clockwise...\n\n");
    printf("Count | Direction | Position\n");
    printf("------|-----------|----------\n");

    int changeCount = 0;

    while (s_running) {
        RotaryDirection_t direction = Rotary_read();

        if (direction != ROTARY_NONE) {
            int position = Rotary_getPosition();
            changeCount++;
            
            printf("%5d | %s  | %8d\n",
                   changeCount,
                   direction == ROTARY_CW ? "CW  ➜   " : "CCW ⬅   ",
                   position);
            fflush(stdout);
        }

        sleepForMs(1);
    }

    printf("\n========================================\n");
    printf("Summary:\n");
    printf("  Total changes: %d\n", changeCount);
    printf("  Final position: %d\n", Rotary_getPosition());
    printf("========================================\n");
    
    Rotary_cleanup();
    return 0;
}