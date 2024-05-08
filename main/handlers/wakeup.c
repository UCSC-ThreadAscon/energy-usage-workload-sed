#include "main.h"
#include "assert.h"

void wakeupInit(nvs_handle_t handle, struct timeval *events, uuid *deviceId,
                struct timeval *nextBatteryWakeup)
{
  nvsReadBlob(handle, NVS_EVENTS_ARRAY, events, EVENTS_ARRAY_SIZE);
  nvsReadBlob(handle, NVS_UUID, deviceId, UUID_SIZE_BYTES);
  nvsReadBlob(handle, NVS_BATTERY_WAKEUP, nextBatteryWakeup, sizeof(struct timeval));

#if NVS_DEBUG
  printEventsArray(events, NUM_EVENTS);
#endif

  return;
}

static inline void incrementEventsIndex(nvs_handle_t handle,
                                        uint8_t currentEventsIndex)
{
  currentEventsIndex += 1;
  nvsWriteByteUInt(handle, NVS_EVENTS_INDEX, currentEventsIndex);
  return;
}

uint64_t getNextEventSleepTime(struct timeval *events, uint8_t eventsIndex,
                               struct timeval tvNow)
{
    struct timeval tvNextEvent = events[eventsIndex];
    return timeDiffMs(tvNow, tvNextEvent);
}

uint64_t getNextBatterySleepTime(struct timeval batteryWakeupTime,
                                 struct timeval tvNow)
{
  otLogNotePlat("tvNow: %" PRIu64 ".", toMicro(tvNow));
  otLogNotePlat("batteryWakeupTime: %" PRIu64 ".", toMicro(batteryWakeupTime));
  return timeDiffMs(tvNow, batteryWakeupTime);
}

void setPacketType(nvs_handle_t handle, PacketSendType packetType)
{
  nvsWriteByteUInt(handle, NVS_PACKET_TYPE, packetType);
  return;
}

void setEventSleepTime(nvs_handle_t handle,
                       uint64_t sleepTime,
                       uint8_t eventsIndex)
{
  setPacketType(handle, EventPacket);
  initDeepSleepTimerMs(sleepTime);
  incrementEventsIndex(handle, eventsIndex);
  return;
}

void setBatterySleepTime(nvs_handle_t handle,
                         uint64_t sleepTime,
                         struct timeval *batteryWakeup,
                         struct timeval tvNow)
{
  setPacketType(handle, BatteryPacket);
  initDeepSleepTimerMs(sleepTime);
  moveToNextBatteryWakeup(handle, batteryWakeup, tvNow);
  return;
}

void onWakeup(nvs_handle_t handle,
              struct timeval *events,
              uuid *deviceId,
              otSockAddr *socket,
              struct timeval *batteryWakeup,
              struct timeval tvNow)
{
  PacketSendType currentPacketType = nvsReadByteUInt(handle, NVS_PACKET_TYPE);

  uint64_t nextBatterySleepTime = getNextBatterySleepTime(*batteryWakeup, tvNow);
  uint8_t eventsIndex = nvsReadByteUInt(handle, NVS_EVENTS_INDEX);

  if (!noMoreEventsToSend(eventsIndex))
  {
    uint64_t nextEventSleepTime = getNextEventSleepTime(events, eventsIndex, tvNow);

#if COMPARISON_DEBUG
    printSleepTimes(nextBatterySleepTime, nextEventSleepTime);
#endif

    if (nextEventSleepTime <= nextBatterySleepTime)
    {
      setEventSleepTime(handle, nextEventSleepTime, eventsIndex);
    }
    else
    {
      setBatterySleepTime(handle, nextBatterySleepTime, batteryWakeup, tvNow);
    }
  }
  else
  {
    setBatterySleepTime(handle, nextBatterySleepTime, batteryWakeup, tvNow);
  }

  coapStart();
  if (currentPacketType == EventPacket)
  {
    sendEventPacket(socket, *deviceId, handle);
  }
  else
  {
    sendBatteryPacket(socket, *deviceId, handle);
  }

#if SHOW_DEBUG_STATS
  printPacketType(currentPacketType);
#endif

  return;
}