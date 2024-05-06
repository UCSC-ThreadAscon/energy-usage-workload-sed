#include "main.h"

void onDeepSleepWakeup(struct timeval *events, uuid *deviceId)
{
  nvs_handle_t handle;
  openReadWrite(NVS_NAMESPACE, &handle);

  nvsReadArray(&handle, NVS_EVENTS_ARRAY, events, EVENTS_ARRAY_SIZE);
  nvsReadArray(&handle, NVS_UUID, deviceId, UUID_SIZE_BYTES);

#if NVS_DEBUG
  printEventsArray(events, NUM_EVENTS);
  printUUID(deviceId);
#endif

  nvs_close(handle);
  return;
}