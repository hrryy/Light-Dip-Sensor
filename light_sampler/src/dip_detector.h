#ifndef _DIP_DETECTOR_H_
#define _DIP_DETECTOR_H_

// Module to detect light dips in sample history
// A dip is when light level drops significantly below the average,
// using hysteresis to prevent false triggers from noise.

/**
 * Analyze sample history and count the number of dips
 * 
 * @param samples Array of voltage samples
 * @param numSamples Number of samples in array
 * @param averageVoltage Current exponentially smoothed average
 * @return Number of dips detected
 */
int DipDetector_countDips(const double* samples, int numSamples, double averageVoltage);

#endif // _DIP_DETECTOR_H_