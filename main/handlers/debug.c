#include "main.h"

bool noMoreEventsToSend(uint8_t eventsIndex)
{
  return eventsIndex >= NUM_EVENTS ? true : false;
}

void eventPacketsStats(uint8_t eventsIndex)
{
  if (!noMoreEventsToSend(eventsIndex))
  {
    otLogNotePlat("Sent %" PRIu8 " Event Packet(s) so far.", eventsIndex);
  }
  else
  {
    otLogNotePlat("Sent all Event Packets.");
  }
  return;
}

void printDeviceInfo() {
  printMeshLocalEid(OT_INSTANCE);

#if (CONFIG_SCENARIO == 1)
  otLogNotePlat("This device is the front door motion sensor.");
#elif (CONFIG_SCENARIO == 2)
  otLogNotePlat("This device is the air quality monitor.");
#else
  otLogNotePlat("This device is the second story door motion sensor.");
#endif
  return;
}

void printSleepTimes(uint64_t eventSleepMicro, uint64_t batterySleepMicro)
{
    otLogNotePlat(
      "The next EVENT sleep time is %.3f ms, or %.3f mins.",
      US_TO_MS((double) eventSleepMicro),
      US_TO_MINUTES((double) eventSleepMicro)
    );

    otLogNotePlat(
      "The next BATTERY sleep time is %.3f ms, or %.3f mins.",
      US_TO_MS((double) batterySleepMicro),
      US_TO_MINUTES((double) batterySleepMicro)
    );
    return;
}