#include "acs_ir_protocol.h"
#include "driver/rmt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ir_tools.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>
#define TAG "IR_PROTOCOL"

// IR RMT
static rmt_channel_t tx_channel = RMT_CHANNEL_0;
static rmt_channel_t rx_channel = RMT_CHANNEL_2;
uint8_t ir_data_received;
/**
 * @brief RMT Receive Task
 *
 */
static void ir_rx_task(void *arg) {
  uint32_t addr = 0;
  uint32_t cmd = 0;
  size_t length = 0;
  bool repeat = false;
  RingbufHandle_t rb = NULL;
  rmt_item32_t *items = NULL;

  rmt_config_t rmt_rx_config = RMT_DEFAULT_CONFIG_RX(IRR_Pin, rx_channel);
  rmt_config(&rmt_rx_config);
  rmt_driver_install(rx_channel, 1000, 0);
  ir_parser_config_t ir_parser_config =
      IR_PARSER_DEFAULT_CONFIG((ir_dev_t)rx_channel);
  ir_parser_config.flags |=
      IR_TOOLS_FLAGS_PROTO_EXT; // Using extended IR protocols (both NEC and RC5
                                // have extended version)
  ir_parser_t *ir_parser = NULL;
#if CONFIG_IR_PROTOCOL_NEC
  ir_parser = ir_parser_rmt_new_nec(&ir_parser_config);
#elif CONFIG_IR_PROTOCOL_RC5
  ir_parser = ir_parser_rmt_new_rc5(&ir_parser_config);
#endif

  // get RMT RX ringbuffer
  rmt_get_ringbuf_handle(rx_channel, &rb);
  assert(rb != NULL);
  // Start receive
  rmt_rx_start(rx_channel, true);
  while (1) {
    items = (rmt_item32_t *)xRingbufferReceive(rb, &length, portMAX_DELAY);
    if (items) {
      length /= 4; // one RMT = 4 Bytes
      if (ir_parser->input(ir_parser, items, length) == ESP_OK) {
        if (ir_parser->get_scan_code(ir_parser, &addr, &cmd, &repeat) ==
            ESP_OK) {
          ESP_LOGI(TAG, "Scan Code %s --- addr: 0x%04x cmd: 0x%04x",
                   repeat ? "(repeat)" : "", addr, cmd);
          // ESP_LOGI(TAG,"%d",(uint8_t)addr);
          // ESP_LOGI(TAG,"%d",(uint8_t)cmd);
          if ((uint8_t)addr == ADDR && (uint8_t)cmd == DATA_ON) {
            ir_data_received = DATA_ON;
          }
          if ((uint8_t)addr == ADDR && (uint8_t)cmd == DATA_OFF) {
            ir_data_received = DATA_OFF;
          }
        }
      }
      // after parsing the data, return spaces to ringbuffer.
      vRingbufferReturnItem(rb, (void *)items);
    }
  }
  ir_parser->del(ir_parser);
  rmt_driver_uninstall(rx_channel);
  vTaskDelete(NULL);
}

/**
 * @brief RMT Transmit Task
 *
 */
static void ir_tx_task(void *arg) {
  uint32_t addr = 0x01;
  uint32_t cmd = 0xDD;
  rmt_item32_t *items = NULL;
  size_t length = 0;
  ir_builder_t *ir_builder = NULL;

  rmt_config_t rmt_tx_config = RMT_DEFAULT_CONFIG_TX(IRC_Pin, tx_channel);
  rmt_tx_config.tx_config.carrier_en = true;
  rmt_config(&rmt_tx_config);
  rmt_driver_install(tx_channel, 0, 0);
  ir_builder_config_t ir_builder_config =
      IR_BUILDER_DEFAULT_CONFIG((ir_dev_t)tx_channel);
  ir_builder_config.flags |=
      IR_TOOLS_FLAGS_PROTO_EXT; // Using extended IR protocols (both NEC and RC5
                                // have extended version)
#if CONFIG_IR_PROTOCOL_NEC
  ir_builder = ir_builder_rmt_new_nec(&ir_builder_config);
#elif CONFIG_IR_PROTOCOL_RC5
  ir_builder = ir_builder_rmt_new_rc5(&ir_builder_config);
#endif
  while (1) {
    ESP_LOGI(TAG, "Send command 0x%x to address 0x%x", cmd, addr);
    // Send new key code
    ESP_ERROR_CHECK(ir_builder->build_frame(ir_builder, addr, cmd));
    ESP_ERROR_CHECK(ir_builder->get_result(ir_builder, &items, &length));
    // To send data according to the waveform items.
    rmt_write_items(tx_channel, items, length, false);
    cmd++;
  }
  ir_builder->del(ir_builder);
  rmt_driver_uninstall(tx_channel);
  vTaskDelete(NULL);
}

void ir_protocol_task_rx_start(void) {
  xTaskCreate(ir_rx_task, "ir_rx_task", 2048, NULL, 10, NULL);
}
void ir_protocol_task_tx_start(void) {
  xTaskCreate(ir_rx_task, "ir_rx_task", 2048, NULL, 10, NULL);
}