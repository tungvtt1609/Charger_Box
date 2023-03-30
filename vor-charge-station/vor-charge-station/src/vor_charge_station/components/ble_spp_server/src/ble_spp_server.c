#include "ble_spp_server.h"

static uint16_t spp_conn_id = 0xffff;
static esp_gatt_if_t spp_gatts_if = 0xff;
QueueHandle_t spp_uart_queue = NULL;
static xQueueHandle cmd_cmd_queue = NULL;
uint8_t ble_srv_data_received;

static bool enable_data_ntf = false;
static bool is_connected = false;
static esp_bd_addr_t spp_remote_bda = {
    0x0,
};

static uint16_t spp_handle_table[SPP_IDX_NB];

static esp_ble_adv_params_t spp_adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
  esp_gatts_cb_t gatts_cb;
  uint16_t gatts_if;
  uint16_t app_id;
  uint16_t conn_id;
  uint16_t service_handle;
  esp_gatt_srvc_id_t service_id;
  uint16_t char_handle;
  esp_bt_uuid_t char_uuid;
  esp_gatt_perm_t perm;
  esp_gatt_char_prop_t property;
  uint16_t descr_handle;
  esp_bt_uuid_t descr_uuid;
};

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if,
                                        esp_ble_gatts_cb_param_t *param);

/* One gatt-based profile one app_id and one gatts_if, this array will store the
 * gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst spp_profile_tab[SPP_PROFILE_NUM] = {
    [SPP_PROFILE_APP_IDX] =
        {
            .gatts_cb = gatts_profile_event_handler,
            .gatts_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is
                                             ESP_GATT_IF_NONE */
        },
};

/*
 *  SPP PROFILE ATTRIBUTES
 ****************************************************************************************
 */

#define CHAR_DECLARATION_SIZE (sizeof(uint8_t))
static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint8_t char_prop_read_write =
    ESP_GATT_CHAR_PROP_BIT_WRITE_NR | ESP_GATT_CHAR_PROP_BIT_READ;
/// SPP Service - data receive characteristic, read&write without response
static const uint16_t spp_data_receive_uuid = ESP_GATT_UUID_SPP_DATA_RECEIVE;
static const uint8_t spp_data_receive_val[20] = {0x00};

/// SPP Service - status characteristic, notify&read
static const uint16_t character_client_config_uuid =
    ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t char_prop_read_notify =
    ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint16_t spp_status_uuid = ESP_GATT_UUID_SPP_COMMAND_NOTIFY;
static const uint8_t spp_status_val[10] = {0x00};
static const uint8_t spp_status_ccc[2] = {0x00, 0x00};

/// Full HRS Database Description - Used to add attributes into the database
static const esp_gatts_attr_db_t spp_gatt_db[SPP_IDX_NB] = {
    // SPP -  Service Declaration
    [SPP_IDX_SVC] = {{ESP_GATT_AUTO_RSP},
                     {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid,
                      ESP_GATT_PERM_READ, sizeof(spp_service_uuid),
                      sizeof(spp_service_uuid), (uint8_t *)&spp_service_uuid}},

    // SPP -  data receive characteristic Declaration
    [SPP_IDX_SPP_DATA_RECV_CHAR] = {{ESP_GATT_AUTO_RSP},
                                    {ESP_UUID_LEN_16,
                                     (uint8_t *)&character_declaration_uuid,
                                     ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE,
                                     CHAR_DECLARATION_SIZE,
                                     (uint8_t *)&char_prop_read_write}},

    // SPP -  data receive characteristic Value
    [SPP_IDX_SPP_DATA_RECV_VAL] =
        {{ESP_GATT_AUTO_RSP},
         {ESP_UUID_LEN_16, (uint8_t *)&spp_data_receive_uuid,
          ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, SPP_DATA_MAX_LEN,
          sizeof(spp_data_receive_val), (uint8_t *)spp_data_receive_val}},

    // SPP -  status characteristic Declaration
    [SPP_IDX_SPP_STATUS_CHAR] = {{ESP_GATT_AUTO_RSP},
                                 {ESP_UUID_LEN_16,
                                  (uint8_t *)&character_declaration_uuid,
                                  ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE,
                                  CHAR_DECLARATION_SIZE,
                                  (uint8_t *)&char_prop_read_notify}},

    // SPP -  status characteristic Value
    [SPP_IDX_SPP_STATUS_VAL] = {{ESP_GATT_AUTO_RSP},
                                {ESP_UUID_LEN_16, (uint8_t *)&spp_status_uuid,
                                 ESP_GATT_PERM_READ, SPP_STATUS_MAX_LEN,
                                 sizeof(spp_status_val),
                                 (uint8_t *)spp_status_val}},

    // SPP -  status characteristic - Client Characteristic Configuration
    // Descriptor
    [SPP_IDX_SPP_STATUS_CFG] = {{ESP_GATT_AUTO_RSP},
                                {ESP_UUID_LEN_16,
                                 (uint8_t *)&character_client_config_uuid,
                                 ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                 sizeof(uint16_t), sizeof(spp_status_ccc),
                                 (uint8_t *)spp_status_ccc}},
};

static uint8_t find_char_and_desr_index(uint16_t handle) {
  uint8_t error = 0xff;

  for (int i = 0; i < SPP_IDX_NB; i++) {
    if (handle == spp_handle_table[i]) {
      return i;
    }
  }

  return error;
}

void spp_cmd_task(void *arg) {
  uint8_t *cmd_id;

  for (;;) {
    vTaskDelay(50 / portTICK_PERIOD_MS);
    if (xQueueReceive(cmd_cmd_queue, &cmd_id, portMAX_DELAY)) {
      esp_log_buffer_char(GATTS_TABLE_TAG, (char *)(cmd_id),
                          strlen((char *)cmd_id));
      free(cmd_id);
    }
  }
  vTaskDelete(NULL);
}

static void spp_task_init(void) {
  cmd_cmd_queue = xQueueCreate(10, sizeof(uint32_t));
  xTaskCreate(spp_cmd_task, "spp_cmd_task", 2048, NULL, 10, NULL);
}

static void gap_event_handler(esp_gap_ble_cb_event_t event,
                              esp_ble_gap_cb_param_t *param) {
  esp_err_t err;
  ESP_LOGE(GATTS_TABLE_TAG, "GAP_EVT, event %d\n", event);

  switch (event) {
  case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
    esp_ble_gap_start_advertising(&spp_adv_params);
    break;
  case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
    // advertising start complete event to indicate advertising start
    // successfully or failed
    if ((err = param->adv_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
      ESP_LOGE(GATTS_TABLE_TAG, "Advertising start failed: %s\n",
               esp_err_to_name(err));
    }
    break;
  default:
    break;
  }
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if,
                                        esp_ble_gatts_cb_param_t *param) {
  esp_ble_gatts_cb_param_t *p_data = (esp_ble_gatts_cb_param_t *)param;
  uint8_t res = 0xff;

  ESP_LOGI(GATTS_TABLE_TAG, "event = %x\n", event);
  switch (event) {
  case ESP_GATTS_REG_EVT:
    ESP_LOGI(GATTS_TABLE_TAG, "%s %d\n", __func__, __LINE__);
    esp_ble_gap_set_device_name(SAMPLE_DEVICE_NAME);

    ESP_LOGI(GATTS_TABLE_TAG, "%s %d\n", __func__, __LINE__);
    esp_ble_gap_config_adv_data_raw((uint8_t *)spp_adv_data,
                                    sizeof(spp_adv_data));

    ESP_LOGI(GATTS_TABLE_TAG, "%s %d\n", __func__, __LINE__);
    esp_ble_gatts_create_attr_tab(spp_gatt_db, gatts_if, SPP_IDX_NB,
                                  SPP_SVC_INST_ID);
    break;
  case ESP_GATTS_READ_EVT:
    res = find_char_and_desr_index(p_data->read.handle);
    if (res == SPP_IDX_SPP_STATUS_VAL) {
      // TODO:client read the status characteristic
    }
    break;
  case ESP_GATTS_WRITE_EVT: {
    res = find_char_and_desr_index(p_data->write.handle);
    esp_log_buffer_hex(GATTS_TABLE_TAG, p_data->write.value, p_data->write.len);
    ble_srv_data_received = (uint8_t)*p_data->write.value;
    break;
  }
  case ESP_GATTS_CONNECT_EVT:
    spp_conn_id = p_data->connect.conn_id;
    spp_gatts_if = gatts_if;
    is_connected = true;
    memcpy(&spp_remote_bda, &p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
    break;
  case ESP_GATTS_DISCONNECT_EVT:
    is_connected = false;
    enable_data_ntf = false;
    esp_ble_gap_start_advertising(&spp_adv_params);
    break;
  case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
    ESP_LOGI(GATTS_TABLE_TAG, "The number handle =%x\n",
             param->add_attr_tab.num_handle);
    if (param->add_attr_tab.status != ESP_GATT_OK) {
      ESP_LOGE(GATTS_TABLE_TAG,
               "Create attribute table failed, error code=0x%x",
               param->add_attr_tab.status);
    } else if (param->add_attr_tab.num_handle != SPP_IDX_NB) {
      ESP_LOGE(GATTS_TABLE_TAG,
               "Create attribute table abnormally, num_handle (%d) doesn't "
               "equal to HRS_IDX_NB(%d)",
               param->add_attr_tab.num_handle, SPP_IDX_NB);
    } else {
      memcpy(spp_handle_table, param->add_attr_tab.handles,
             sizeof(spp_handle_table));
      esp_ble_gatts_start_service(spp_handle_table[SPP_IDX_SVC]);
    }
    break;
  }
  default:
    break;
  }
}

static void gatts_event_handler(esp_gatts_cb_event_t event,
                                esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param) {
  ESP_LOGI(GATTS_TABLE_TAG, "EVT %d, gatts if %d\n", event, gatts_if);

  /* If event is register event, store the gatts_if for each profile */
  if (event == ESP_GATTS_REG_EVT) {
    if (param->reg.status == ESP_GATT_OK) {
      spp_profile_tab[SPP_PROFILE_APP_IDX].gatts_if = gatts_if;
    } else {
      ESP_LOGI(GATTS_TABLE_TAG, "Reg app failed, app_id %04x, status %d\n",
               param->reg.app_id, param->reg.status);
      return;
    }
  }

  do {
    int idx;
    for (idx = 0; idx < SPP_PROFILE_NUM; idx++) {
      if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a
                                             certain gatt_if, need to call every
                                             profile cb function */
          gatts_if == spp_profile_tab[idx].gatts_if) {
        if (spp_profile_tab[idx].gatts_cb) {
          spp_profile_tab[idx].gatts_cb(event, gatts_if, param);
        }
      }
    }
  } while (0);
}

void ble_task_start(void) {

  esp_err_t ret;
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

  // Initialize NVS
  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

#if ENABLE_LOG_ERR
  ret = esp_bt_controller_init(&bt_cfg);
  if (ret) {
    ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed: %s\n", __func__,
             esp_err_to_name(ret));
    return;
  }
  ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
  if (ret) {
    ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed: %s\n", __func__,
             esp_err_to_name(ret));
    return;
  }
  ESP_LOGI(GATTS_TABLE_TAG, "%s init bluetooth\n", __func__);
  ret = esp_bluedroid_init();
  if (ret) {
    ESP_LOGE(GATTS_TABLE_TAG, "%s init bluetooth failed: %s\n", __func__,
             esp_err_to_name(ret));
    return;
  }
  ret = esp_bluedroid_enable();
  if (ret) {
    ESP_LOGE(GATTS_TABLE_TAG, "%s enable bluetooth failed: %s\n", __func__,
             esp_err_to_name(ret));
    return;
  }
#endif

  esp_bt_controller_init(&bt_cfg);
  esp_bt_controller_enable(ESP_BT_MODE_BLE);
  esp_bluedroid_init();
  esp_bluedroid_enable();
  esp_ble_gatts_register_callback(gatts_event_handler);
  esp_ble_gap_register_callback(gap_event_handler);
  esp_ble_gatts_app_register(ESP_SPP_APP_ID);
  spp_task_init();
}
