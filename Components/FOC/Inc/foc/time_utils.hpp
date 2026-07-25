#ifndef TIME_UTILS_H
#define TIME_UTILS_H

/**
 * Function implementing delay() function in milliseconds
 * - blocking function
 * - hardware specific

 * @param ms number of milliseconds to wait
 */
// function buffering delay()
// arduino uno function doesn't work well with interrupts
void _delay(unsigned long ms);

/**
 * Function implementing timestamp getting function in microseconds
 * hardware specific
 */
// function buffering _micros()
// arduino function doesn't work well with interrupts
unsigned long _micros();
unsigned long _delayMicroseconds(unsigned long us);

#endif
