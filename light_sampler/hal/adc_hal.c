#include "adc_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdint.h>

// SPI Configuration (same as working joystick)
#define SPI_DEVICE "/dev/spidev0.0"
#define SPI_MODE 0
#define SPI_BITS_PER_WORD 8
#define SPI_SPEED_HZ 250000

// Internal state
static int s_spi_fd = -1;

/**
 * Read a single channel from MCP3208
 * Based on proven joystick implementation
 */
static int read_channel(int fd, int channel, uint32_t speed_hz)
{
    if (channel < 0 || channel >= MCP3208_MAX_CHANNELS) {
        fprintf(stderr, "ADC: Invalid channel %d\n", channel);
        return -1;
    }

    // MCP3208 command format for single-ended read
    uint8_t tx[3] = {
        (uint8_t)(0x06 | ((channel & 0x04) >> 2)),  // Start bit + MSB of channel
        (uint8_t)((channel & 0x03) << 6),            // Remaining channel bits
        0x00
    };
    uint8_t rx[3] = { 0 };

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = speed_hz,
        .bits_per_word = SPI_BITS_PER_WORD,
        .cs_change = 0
    };

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 1) {
        perror("ADC: SPI read failed");
        return -1;
    }

    // Extract 12-bit result
    return ((rx[1] & 0x0F) << 8) | rx[2];
}

bool ADC_init(void)
{
    if (s_spi_fd >= 0) {
        fprintf(stderr, "ADC_init: Already initialized\n");
        return true;
    }

    // Open SPI device
    s_spi_fd = open(SPI_DEVICE, O_RDWR);
    if (s_spi_fd < 0) {
        perror("ADC_init: Failed to open SPI device");
        return false;
    }

    // Optional: Configure SPI mode (usually default is fine)
    uint8_t mode = SPI_MODE;
    if (ioctl(s_spi_fd, SPI_IOC_WR_MODE, &mode) < 0) {
        perror("ADC_init: Failed to set SPI mode");
        close(s_spi_fd);
        s_spi_fd = -1;
        return false;
    }

    uint8_t bits = SPI_BITS_PER_WORD;
    if (ioctl(s_spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
        perror("ADC_init: Failed to set bits per word");
        close(s_spi_fd);
        s_spi_fd = -1;
        return false;
    }

    printf("ADC initialized on %s\n", SPI_DEVICE);
    return true;
}

void ADC_cleanup(void)
{
    if (s_spi_fd >= 0) {
        close(s_spi_fd);
        s_spi_fd = -1;
        printf("ADC cleanup complete\n");
    }
}

int ADC_readLightRaw(void)
{
    return ADC_readRaw(LIGHT_SENSOR_CHANNEL);
}

double ADC_readLightVoltage(void)
{
    return ADC_readVoltage(LIGHT_SENSOR_CHANNEL);
}

int ADC_readPotentiometerRaw(void)
{
    return ADC_readRaw(POTENTIOMETER_CHANNEL);
}

double ADC_readPotentiometerVoltage(void)
{
    return ADC_readVoltage(POTENTIOMETER_CHANNEL);
}

int ADC_readRaw(int channel)
{
    if (s_spi_fd < 0) {
        fprintf(stderr, "ADC_readRaw: Not initialized\n");
        return -1;
    }

    return read_channel(s_spi_fd, channel, SPI_SPEED_HZ);
}

double ADC_readVoltage(int channel)
{
    int raw = ADC_readRaw(channel);
    
    if (raw < 0) {
        return -1.0;
    }
    
    // Convert to voltage: (raw / max_value) * reference_voltage
    double voltage = ((double)raw / (double)MCP3208_MAX_VALUE) * ADC_VREF;
    
    return voltage;
}

bool ADC_isInitialized(void)
{
    return (s_spi_fd >= 0);
}
