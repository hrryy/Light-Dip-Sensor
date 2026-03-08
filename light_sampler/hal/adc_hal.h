#ifndef _ADC_HAL_H_
#define _ADC_HAL_H_

#include <stdbool.h>

// MCP3208 specifications
#define MCP3208_MAX_CHANNELS 8
#define MCP3208_MAX_VALUE 4095

// Reference voltage (3.3V system)
#define ADC_VREF 3.3

// Sensor channel assignments
#define LIGHT_SENSOR_CHANNEL 0
#define POTENTIOMETER_CHANNEL 1  // ← Changed to Channel 1

bool ADC_init(void);
void ADC_cleanup(void);

int ADC_readLightRaw(void);
double ADC_readLightVoltage(void);

// Potentiometer functions
int ADC_readPotentiometerRaw(void);
double ADC_readPotentiometerVoltage(void);

// Generic channel access
int ADC_readRaw(int channel);
double ADC_readVoltage(int channel);

bool ADC_isInitialized(void);

#endif // _ADC_HAL_H_