/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 *
 * OpenThread Command Line Example
 *
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 *
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
*/
#include "utilities.h"
#include "workload.h"

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_openthread.h"
#include "esp_openthread_netif_glue.h"
#include "esp_ot_sleepy_device_config.h"
#include "esp_vfs_eventfd.h"
#include "nvs_flash.h"
#include "driver/rtc_io.h"
#include "driver/uart.h"
#include "openthread/logging.h"
#include "openthread/thread.h"

#if !SOC_IEEE802154_SUPPORTED
#error "Openthread sleepy device is only supported for the SoCs which have IEEE 802.15.4 module"
#endif

#define TAG "ot_send_deep_sleep"

static void create_config_network(otInstance *instance)
{
    otLinkModeConfig linkMode = { 0 };

    linkMode.mRxOnWhenIdle = false;
    linkMode.mDeviceType = false;
    linkMode.mNetworkData = false;

    if (otLinkSetPollPeriod(instance, POLL_PERIOD_MS) != OT_ERROR_NONE) {
        ESP_LOGE(TAG, "Failed to set OpenThread pollperiod.");
        abort();
    }
    ESP_LOGI(TAG, "Poll period is currently at %" PRIu32 ".",
             otLinkGetPollPeriod(instance));

    if (otThreadSetLinkMode(instance, linkMode) != OT_ERROR_NONE) {
        ESP_LOGE(TAG, "Failed to set OpenThread linkmode.");
        abort();
    }
    ESP_ERROR_CHECK(esp_openthread_auto_start(NULL));
}

static esp_netif_t *init_openthread_netif(const esp_openthread_platform_config_t *config)
{
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_OPENTHREAD();
    esp_netif_t *netif = esp_netif_new(&cfg);
    assert(netif != NULL);
    ESP_ERROR_CHECK(esp_netif_attach(netif, esp_openthread_netif_glue_init(config)));

    return netif;
}

static void otDeepSleepInit(int wakeupTimeMs)
{
    otLogNotePlat("Enabling timer wakeup: %dms\n", wakeupTimeMs);
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(MS_TO_MICRO(wakeupTimeMs)));
}

static void ot_task_worker(void *aContext)
{
    esp_openthread_platform_config_t config = {
        .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
        .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
    };

    // Initialize the OpenThread stack
    ESP_ERROR_CHECK(esp_openthread_init(&config));

#if CONFIG_OPENTHREAD_LOG_LEVEL_DYNAMIC
    // The OpenThread log level directly matches ESP log level
    (void)otLoggingSetLevel(CONFIG_LOG_DEFAULT_LEVEL);
#endif
    esp_netif_t *openthread_netif;
    // Initialize the esp_netif bindings
    openthread_netif = init_openthread_netif(&config);
    esp_netif_set_default_netif(openthread_netif);

    create_config_network(esp_openthread_get_instance());

    // TX power must be set before starting the OpenThread CLI.
    setTxPower();

    // Run the main loop
    esp_openthread_launch_mainloop();

    // Clean up
    esp_openthread_netif_glue_deinit();
    esp_netif_destroy(openthread_netif);

    esp_vfs_eventfd_unregister();
    vTaskDelete(NULL);
}


#define NVS_PACKET_TYPE_KEY "workload_type"

void app_main(void)
{
    // Used eventfds:
    // * netif
    // * ot task queue
    // * radio driver
    esp_vfs_eventfd_config_t eventfd_config = {
        .max_fds = 3,
    };

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_vfs_eventfd_register(&eventfd_config));

    xTaskCreate(ot_task_worker, "ot_power_save_main", 4096, NULL, 5, NULL);

    /**
     * Non-Volatile Storage is used to determine whether or not the next packet
     * to send should be an PERIODIC scenario 1 packet, or an APERIODIC scenario 2
     * packet. 
    */
    nvs_handle_t packetTypeHandle;
    ESP_ERROR_CHECK(nvs_open(NVS_PACKET_TYPE_KEY, NVS_READWRITE, &packetTypeHandle));

    type currentType = Periodic;
    esp_err_t espError = nvs_get_u32(packetTypeHandle, NVS_PACKET_TYPE_KEY, 
                                  (uint32_t *) &currentType);
    switch (espError) {
      case ESP_ERR_NVS_NOT_FOUND:
        ESP_ERROR_CHECK(nvs_set_u32(packetTypeHandle, NVS_PACKET_TYPE_KEY,
                                    (uint32_t) currentType));
        break;

      default:
        ESP_ERROR_CHECK(espError);
    }

    /** ---- CoAP Client Implementation ---- */
    otDeepSleepInit(PERIODIC_WAIT_TIME_MS);

    checkConnection(OT_INSTANCE);
    printMeshLocalEid(OT_INSTANCE);

    otError error = otCoapStart(OT_INSTANCE, COAP_SOCK_PORT);

    if (error != OT_ERROR_NONE) {
      otLogCritPlat("Failed to start COAP server.");
    } else {
      otLogNotePlat("Started CoAP server at port %d.", COAP_SERVER_PORT);
    }

    otSockAddr socket;
    otIp6Address server;

    EmptyMemory(&socket, sizeof(otSockAddr));
    EmptyMemory(&server, sizeof(otIp6Address));

    otIp6AddressFromString(CONFIG_SERVER_IP_ADDRESS, &server);
    socket.mAddress = server;
    socket.mPort = COAP_SERVER_PORT;

    /**
     * Determine what type of packet to send next.
    */
#if (CONFIG_SCENARIO == 1)
    bool sendScenario2 = waterLeakOccured();
#elif (CONFIG_SCENARIO == 2)
    bool sendScenario2 = someoneAtFrontDoor();
#elif (CONFIG_SCENARIO == 3)
    bool sendScenario2 = someoneAtSecondStory();
#endif

    int nextWaitTime;
    type nextType;

    if (!sendScenario2) {
      nextType = Periodic;
      nextWaitTime = PERIODIC_WAIT_TIME_MS;
    }
    else {
      nextType = APeriodic;

      // To prevent aperiodic packets from being sent every PERIODIC_WAIT_TIME_MS,
      // (like periodic packets), we force aperiodic packets to be sent
      // at some time before the next PERIODIC_WAIT_TIME_MS.
      //
      nextWaitTime = esp_random() % PERIODIC_WAIT_TIME_MS;
    }

    ESP_ERROR_CHECK(nvs_set_u32(packetTypeHandle, NVS_PACKET_TYPE_KEY, nextType));
    otDeepSleepInit(nextWaitTime);

    /**
     * Send the packet, then go to sleep after receiving a response.
    */
    sendRequest(currentType, &socket);
    if (currentType == Periodic) {
      otLogNotePlat("Sent scenario 1 packet.");
    } else {
      otLogNotePlat("Sent scenario 2 packet.");
    }

    otLogNotePlat("Will wait %d ms before sending next packet.",
                  nextWaitTime);

    KEEP_THREAD_ALIVE();
    return;
}
