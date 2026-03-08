#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>
#include <gpiod.h>

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
    printf("  Raw GPIO Monitor (Debug Tool)\n");
    printf("========================================\n");
    printf("This will show raw GPIO states.\n");
    printf("Turn encoder and watch values change.\n");
    printf("Press Ctrl+C to exit\n\n");

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    const char* gpioChip = "/dev/gpiochip0";
    unsigned int clkLine = 23;
    unsigned int dtLine = 24;

    // Open chip
    struct gpiod_chip* chip = gpiod_chip_open(gpioChip);
    if (!chip) {
        perror("Failed to open GPIO chip");
        return 1;
    }

    // Configure line settings - NO INTERNAL PULL-UPS (you have external ones)
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_DISABLED);  // ← Changed!

    // Configure request
    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "debug_monitor");

    // Configure line config
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    unsigned int offsets[2] = {clkLine, dtLine};
    gpiod_line_config_add_line_settings(line_cfg, offsets, 2, settings);

    // Request lines
    struct gpiod_line_request* request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    gpiod_line_config_free(line_cfg);
    gpiod_request_config_free(req_cfg);
    gpiod_line_settings_free(settings);

    if (!request) {
        perror("Failed to request lines");
        gpiod_chip_close(chip);
        return 1;
    }

    printf("✓ GPIO lines opened successfully\n");
    printf("Configuration: External pull-up resistors to VCC\n");
    printf("Monitoring GPIO %u (CLK/OUT A) and GPIO %u (DT/OUT B)\n\n", clkLine, dtLine);
    printf("CLK | DT  | State\n");
    printf("----+-----+------------------------\n");

    int lastClk = -1;
    int lastDt = -1;
    int changeCount = 0;

    while (s_running) {
        enum gpiod_line_value clkVal = gpiod_line_request_get_value(request, clkLine);
        enum gpiod_line_value dtVal = gpiod_line_request_get_value(request, dtLine);

        int clk = (clkVal == GPIOD_LINE_VALUE_ACTIVE) ? 1 : 0;
        int dt = (dtVal == GPIOD_LINE_VALUE_ACTIVE) ? 1 : 0;

        // Print when values change
        if (clk != lastClk || dt != lastDt) {
            changeCount++;
            printf(" %d  |  %d  | Change #%d", clk, dt, changeCount);
            
            // Detect edges
            if (lastClk == 1 && clk == 0) {
                printf(" [CLK falling ↓]");
            } else if (lastClk == 0 && clk == 1) {
                printf(" [CLK rising ↑]");
            }
            
            if (lastDt == 1 && dt == 0) {
                printf(" [DT falling ↓]");
            } else if (lastDt == 0 && dt == 1) {
                printf(" [DT rising ↑]");
            }
            
            printf("\n");
            
            lastClk = clk;
            lastDt = dt;
        }

        sleepForMs(1);
    }

    printf("\nTotal changes detected: %d\n", changeCount);
    printf("Cleaning up...\n");
    gpiod_line_request_release(request);
    gpiod_chip_close(chip);
    return 0;
}