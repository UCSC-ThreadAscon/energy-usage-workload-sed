/**
 * All of the code in the documention is both taken from and based upon the code
 * from the "System Time" page from the ESP-IDF documentation:
 * https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/system_time.html
*/

#include "time.h"

int64_t timevalToMicro(struct timeval time) {
  return (int64_t)time.tv_sec * 1000000L + (int64_t)time.tv_usec;
}

struct timeval getCurrentTime()
{
  struct timeval tvNow;
  gettimeofday(&tvNow, NULL);
  return tvNow;
}

/**
 * What will be the timestamp if if it is "x" seconds from now?
*/
struct timeval getFutureTimeval(int64_t seconds) {
  struct timeval tvFuture;
  gettimeofday(&tvFuture, NULL);
  tvFuture.tv_sec += seconds;
  return tvFuture;
}

/**
 * The code to find the duration of time between two timevals comes from:
 * https://github.com/espressif/esp-idf/blob/master/examples/openthread/ot_sleepy_device/deep_sleep/main/esp_ot_sleepy_device.c#L117
 *
 * This function assumes that "tv2" occurs later than "tv1". The function
 * returns the time elapsed starting at "tv1" to "tv2" in milliseconds.
*/
uint64_t timeDiff(struct timeval tv1, struct timeval tv2) {
  return (tv2.tv_sec - tv1.tv_sec) * 1000 + (tv2.tv_usec - tv1.tv_usec) / 1000;
}