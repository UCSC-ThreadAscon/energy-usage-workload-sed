/**
 * My Storage API is based upon the NVS Binary Large Object example code
 * in the ESP-IDF Github repository:
 * https://github.com/espressif/esp-idf/tree/v5.2.1/examples/storage/nvs_rw_blob
*/
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>

#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#define NVS_EVENTS_ARRAY "events"