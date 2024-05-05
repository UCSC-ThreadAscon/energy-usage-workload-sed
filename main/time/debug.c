#include "time.h"

void printEventMinutes(int eventNum, struct timeval time)
{
  int64_t timeMicro = timevalToMicro(time);
  int64_t timeMinutes = US_TO_MINUTES(timeMicro);
  otLogNotePlat("The %dth event will be in %" PRId64 " minutes.",
                eventNum, timeMinutes);
  return;
}

void printEventsArray(struct timeval *events, int numEvents) {
  for (int i = 0; i < numEvents; i++)
  {
    printEventMinutes(i + 1, events[i]);
  }
  return;
}