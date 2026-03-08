#include <stdio.h>
#include <gpiod.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

static volatile bool running = true;

void signal_handler(int sig) {
    (void)sig;
    running = false;
}

int main(void)
{
    signal(SIGINT, signal_handler);
    
    const char* chip_path = "/dev/gpiochip0";
    unsigned int led_line = 11;  // CHANGE THIS to your LED GPIO line!
    
    printf("Testing LED on GPIO %u\n", led_line);
    printf("LED should blink. Press Ctrl+C to exit.\n\n");
    
    struct gpiod_chip* chip = gpiod_chip_open(chip_path);
    if (!chip) {
        perror("Cannot open chip");
        return 1;
    }
    
    // Configure as output
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);
    
    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "led_test");
    
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(line_cfg, &led_line, 1, settings);
    
    struct gpiod_line_request* request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    
    gpiod_line_config_free(line_cfg);
    gpiod_request_config_free(req_cfg);
    gpiod_line_settings_free(settings);
    
    if (!request) {
        perror("Cannot request line");
        gpiod_chip_close(chip);
        return 1;
    }
    
    printf("Blinking LED...\n");
    
    while (running) {
        // Turn LED ON
        gpiod_line_request_set_value(request, led_line, GPIOD_LINE_VALUE_ACTIVE);
        printf("LED ON\n");
        sleep(1);
        
        // Turn LED OFF
        gpiod_line_request_set_value(request, led_line, GPIOD_LINE_VALUE_INACTIVE);
        printf("LED OFF\n");
        sleep(1);
    }
    
    // Turn off LED
    gpiod_line_request_set_value(request, led_line, GPIOD_LINE_VALUE_INACTIVE);
    
    gpiod_line_request_release(request);
    gpiod_chip_close(chip);
    printf("\nLED test complete\n");
    return 0;
}