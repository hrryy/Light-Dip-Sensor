#include "dip_detector.h"
#include <stdbool.h>
#include <stdio.h>

// Dip detection thresholds
#define DIP_THRESHOLD 0.1        // Voltage must drop 0.1V below average to trigger
#define HYSTERESIS_THRESHOLD 0.07 // Must rise to 0.07V below average to reset

int DipDetector_countDips(const double* samples, int numSamples, double averageVoltage)
{
    if (samples == NULL || numSamples <= 0) {
        return 0;
    }

    int dipCount = 0;
    bool inDip = false;

    // Process each sample
    for (int i = 0; i < numSamples; i++) {
        double voltage = samples[i];
        double deviationFromAvg = averageVoltage - voltage;

        if (!inDip) {
            // Check if we're entering a dip
            // Light level must drop 0.1V below average
            if (deviationFromAvg >= DIP_THRESHOLD) {
                dipCount++;
                inDip = true;
            }
        } else {
            // We're in a dip - check if we've recovered
            // Must rise back to within 0.07V of average to exit dip state
            if (deviationFromAvg <= HYSTERESIS_THRESHOLD) {
                inDip = false;
            }
        }
    }

    return dipCount;
}
