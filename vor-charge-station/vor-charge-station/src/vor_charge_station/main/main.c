/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "acs_ir_protocol.h"
#include "acs_pinout.h"
#include "battery_rs485.h"
#include "ble_spp_server.h"

#define TAG "CHARGE_STATION"
#define SPP_SERVER_NAME "CHARGE_STATION_SERVER"

#define TEST_LED 0
#define TEST_IR_TX_RX 0
#define TEST_INPUT_24V 0
#define TEST_RS485 0
#define TEST_CAN 0
#define Bluetooth_LE 1

#define ENABLE_LOG_ERR 0

extern bool charge_status;
TimerHandle_t xTimers[2];

#if TEST_RS485
#include "battery_rs485.h"
#endif

void vTimerCallback(TimerHandle_t xTimer) {
  configASSERT(xTimer);
  uint32_t ulCount = (uint32_t)pvTimerGetTimerID(xTimer);
  if (ulCount == 0) {
#if TEST_INPUT_24V
    static bool input_gpio[3];
    input_gpio[0] = gpio_get_level(X0);
    input_gpio[1] = gpio_get_level(X1);
    input_gpio[2] = gpio_get_level(X2);
    // ESP_LOGI(TAG,"Auto Charge Running");

    ESP_LOGI(TAG, "INPUT read: %d %d %d", input_gpio[0], input_gpio[1],
             input_gpio[2]);
    gpio_set_level(Y1, input_gpio[0]);
    gpio_set_level(Y2, input_gpio[1]);
    gpio_set_level(Y3, input_gpio[2]);
#endif
#if TEST_RS485
    get_basic_info();
    ESP_LOGI(TAG, "vol: %f, percent: %f, current: %f", get_battery_voltage(),
             get_percentage(), get_battery_current());
#endif
  }

  // timer led
  if (ulCount == 1) {
#if TEST_LED
    ESP_LOGI(TAG, "Toggle LED");
    gpio_toggle_pin(Y0);
    gpio_toggle_pin(Y1);
    gpio_toggle_pin(Y2);
    gpio_toggle_pin(Y3);
#else
    if (charge_status == 1) {
      gpio_toggle_pin(STATUS_LED);
    } else {
      gpio_set_level(STATUS_LED, 1);
    }
#endif
  }
}

static void acs_control_task(void *arg) {
  extern uint8_t ble_srv_data_received;
  extern uint8_t ir_data_received;
  while (1) {
    if (ble_srv_data_received == 1 || ir_data_received == DATA_ON) {
      if (acs_get_status() == ACS_STATUS_OFF) {
        acs_turn_on();
        ESP_LOGI(TAG, "TURN ON ACS");
      }
    }
    if (ble_srv_data_received == 2 || ir_data_received == DATA_OFF) {
      if (acs_get_status() == ACS_STATUS_ON) {
        acs_turn_off();
        ESP_LOGI(TAG, "TURN OFF ACS");
      }
    }
    ble_srv_data_received = 0;
    ir_data_received = 0;
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void app_main(void) {
  gpio_setup();
#if TEST_RS485
  acs_uart_config();
#endif
  ir_protocol_task_rx_start();
#if TEST_IR_TX_RX
  ir_protocol_task_tx_start();
#endif
  xTimers[0] = xTimerCreate("Timer chatter", pdMS_TO_TICKS(2000), pdTRUE,
                            (void *)0, vTimerCallback);
  xTimerStart(xTimers[0], 0);

  xTimers[1] = xTimerCreate("Timer status", pdMS_TO_TICKS(500), pdTRUE,
                            (void *)1, vTimerCallback);
  xTimerStart(xTimers[1], 0);
#if Bluetooth_LE
  ble_task_start();

  xTaskCreate(acs_control_task, "ble_control_task", 2048, NULL,
              (10 | portPRIVILEGE_BIT), NULL);
#endif
}
