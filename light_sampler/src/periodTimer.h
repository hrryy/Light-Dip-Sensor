#ifndef _PERIOD_TIMER_H_
#define _PERIOD_TIMER_H_

// Module to record and report the timing of periodic events.
//     Written by Brian Fraser

// Maximum number of timestamps to record for a given event.
#define MAX_EVENT_TIMESTAMPS (1024*4)

enum Period_whichEvent {
    PERIOD_EVENT_SAMPLE_LIGHT,
    PERIOD_EVENT_MARK_SECOND,
    NUM_PERIOD_EVENTS
};

typedef struct {
    int numSamples;
    double minPeriodInMs;
    double maxPeriodInMs;
    double avgPeriodInMs;
} Period_statistics_t;

// Initialize/cleanup the module's data structures.
void Period_init(void);
void Period_cleanup(void);

// Record the current time as a timestamp for the indicated event.
void Period_markEvent(enum Period_whichEvent whichEvent);

// Fill the `pStats` struct with statistics about the periodic event.
void Period_getStatisticsAndClear(
    enum Period_whichEvent whichEvent,
    Period_statistics_t *pStats
);

#endif