#ifndef _BLE_SPP_SERVER_H_
#define _BLE_SPP_SERVER_H_

#include "ble_spp_server.h"
#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLE_TAG "BLE_ACS"
#define spp_sprintf(s, ...) sprintf((char *)(s), ##__VA_ARGS__)
#define SPP_DATA_MAX_LEN (512)
#define SPP_CMD_MAX_LEN (20)
#define SPP_STATUS_MAX_LEN (20)
#define SPP_DATA_BUFF_MAX_LEN (2 * 1024)

enum
{
  SPP_IDX_SVC,

  SPP_IDX_SPP_DATA_RECV_CHAR,
  SPP_IDX_SPP_DATA_RECV_VAL,

  SPP_IDX_SPP_DATA_NOTIFY_CHAR,
  SPP_IDX_SPP_DATA_NTY_VAL,
  SPP_IDX_SPP_DATA_NTF_CFG,

  SPP_IDX_SPP_COMMAND_CHAR,
  SPP_IDX_SPP_COMMAND_VAL,

  SPP_IDX_SPP_STATUS_CHAR,
  SPP_IDX_SPP_STATUS_VAL,
  SPP_IDX_SPP_STATUS_CFG,

#ifdef SUPPORT_HEARTBEAT
  SPP_IDX_SPP_HEARTBEAT_CHAR,
  SPP_IDX_SPP_HEARTBEAT_VAL,
  SPP_IDX_SPP_HEARTBEAT_CFG,
#endif

  SPP_IDX_NB,
};

#define GATTS_TABLE_TAG "GATTS_SPP"

#define SPP_PROFILE_NUM 2
#define SPP_PROFILE_APP_IDX 0
#define ESP_SPP_APP_ID 0x56
#define SAMPLE_DEVICE_NAME \
  "ESP_CHARGE_STATION_TEST" // The Device Name Characteristics in GAP
#define SPP_SVC_INST_ID 0

/// SPP Service
static const uint16_t spp_service_uuid = 0xABF0;

/// Characteristic UUID
#define ESP_GATT_UUID_SPP_DATA_RECEIVE 0xABF1
#define ESP_GATT_UUID_SPP_COMMAND_NOTIFY 0xABF4

#define ACS_NUMBER_NAME CONFIG_ACS_NUMBER_NAME

static const uint8_t spp_adv_data[23] = {
    /* Flags */
    0x02, 0x01, 0x06,
    /* Complete List of 16-bit Service Class UUIDs */
    0x03, 0x03, 0xF0, 0xAB,
    /* Complete Local Name in advertising */
    0x0F, 0x09,
    '_', 'A', 'C', 'S', '_', '0', '0', (char)ACS_NUMBER_NAME + 48, '_', 'B', 'L', 'E', '_', '_'};
void ble_task_start(void);
#endif
