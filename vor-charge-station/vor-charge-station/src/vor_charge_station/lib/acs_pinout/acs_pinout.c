#include "acs_pinout.h"

#define TAG "ACS_GPIO"
#define LOG_INFO 0
#define START_MODE AUTO
// #define START_MODE MAN
bool charging_mode = START_MODE, last_charging_mode = AUTO;
bool charge_status = 0, last_charge_status = 0;

void gpio_setup() {
  gpio_reset_pin(GPIO_NUM_14); // reset pin 14 to disable JTAG pin.
  gpio_set_direction(TEST_LED_DEV_MODULE, GPIO_MODE_OUTPUT);
  gpio_set_direction(RELAY_0, GPIO_MODE_INPUT_OUTPUT);
  gpio_set_direction(RELAY_1, GPIO_MODE_INPUT_OUTPUT);
  gpio_set_direction(RELAY_2, GPIO_MODE_INPUT_OUTPUT);
  gpio_set_direction(Y0, GPIO_MODE_INPUT_OUTPUT);
  gpio_set_direction(Y1, GPIO_MODE_INPUT_OUTPUT);
  gpio_set_direction(Y2, GPIO_MODE_INPUT_OUTPUT);
  gpio_set_direction(Y3, GPIO_MODE_INPUT_OUTPUT);
  gpio_set_direction(IRC_Pin, GPIO_MODE_OUTPUT);

  gpio_set_direction(X0, GPIO_MODE_INPUT);
  gpio_set_direction(X1, GPIO_MODE_INPUT);
  gpio_set_direction(X2, GPIO_MODE_INPUT);
  gpio_set_direction(WF_CONFIG_BT_Pin, GPIO_MODE_INPUT);
  gpio_set_direction(IRR_Pin, GPIO_MODE_INPUT);

  gpio_set_pull_mode(X0, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(X1, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(X2, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(WF_CONFIG_BT_Pin, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(IRR_Pin, GPIO_PULLUP_ONLY);

  relay_off(RELAY_0);
  relay_off(RELAY_1);
  relay_off(RELAY_2);

  gpio_set_level(Y0, 0);
  gpio_set_level(Y1, 0);
  gpio_set_level(Y2, 0);
  gpio_set_level(Y3, 0);
  gpio_set_level(IRC_Pin, 0);

  vTaskDelay(1000 / portTICK_PERIOD_MS);
  // check_charging_mode();
  relay_on(RELAY_0);
  relay_on(RELAY_1);
  relay_on(RELAY_2);

  for (int i = 0; i <= 5; i++) {
    gpio_toggle_pin(Y0);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  gpio_set_level(Y0, 1);
  gpio_set_level(Y1, 1);
  gpio_set_level(Y2, 1);
  gpio_set_level(Y3, 1);

  relay_off(RELAY_0);
  relay_off(RELAY_1);
  relay_off(RELAY_2);

  charge_status = 0;
}

// relay on if output is HIGH
void relay_off(gpio_num_t relay_number) { gpio_set_level(relay_number, 1); }

// relay on if output is LOW
void relay_on(gpio_num_t relay_number) { gpio_set_level(relay_number, 0); }

void charging() {
  // ESP_LOGI(TAG,"X2 val: %d",gpio_get_level(AUTO_MAN));
  switch (gpio_get_level(AUTO_MAN)) {
  case MAN:
    /* code */
    // Manual mode
    // vTaskDelay(1000/portTICK_PERIOD_MS);
    charging_mode = MAN;
    if (last_charging_mode != charging_mode) {
#if LOG_INFO
      ESP_LOGI(TAG, "Manual mode ");
#endif
      // Manual mode with contact
      vTaskDelay(3000 / portTICK_PERIOD_MS);
      relay_on(RELAY_1);
      relay_on(RELAY_2);
      last_charging_mode = charging_mode;
    }
    break;

  case AUTO:
    // vTaskDelay(1000/portTICK_PERIOD_MS);
    // Auto mode
    charging_mode = AUTO;
    if (last_charging_mode != charging_mode) {
#if LOG_INFO
      ESP_LOGI(TAG, "Auto mode ");
#endif
      // buzzer_notice();
      relay_off(RELAY_1);
      relay_off(RELAY_2);
      vTaskDelay(3000 / portTICK_PERIOD_MS);
      last_charging_mode = charging_mode;
    }
    break;
  }
}

void check_charging_mode() {
  if (gpio_get_level(AUTO_MAN) == MAN) {
    // Manual mode
    charging_mode = MAN;
    relay_on(RELAY_1);
    relay_on(RELAY_2);
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    last_charging_mode = charging_mode;
  } else {
    // Auto mode
    charging_mode = AUTO;
    last_charging_mode = charging_mode;
  }
}

bool state;
void gpio_toggle_pin(gpio_num_t pin) {
  state = gpio_get_level(pin);
  gpio_set_level(pin, !state);
}

void acs_turn_on(void) {
  relay_on(RELAY_1);
  relay_on(RELAY_2);
  charge_status = ACS_STATUS_ON;
}
void acs_turn_off(void) {
  relay_off(RELAY_1);
  relay_off(RELAY_2);
  charge_status = ACS_STATUS_OFF;
}
uint8_t acs_get_status(void) {
  if (gpio_get_level(RELAY_1) == STATE_ON &&
      gpio_get_level(RELAY_2) == STATE_ON) {
    charge_status = ACS_STATUS_ON;
    ESP_LOGI(TAG, "ACS_IS_ON");
  } else {
    charge_status = ACS_STATUS_OFF;
    ESP_LOGI(TAG, "ACS_IS_OFF");
  }
  return (charge_status);
}

void acs_switch_state(void) {
  if (acs_get_status() != last_charge_status) {
    if (acs_get_status() == ACS_STATUS_ON)
      acs_turn_off();
    else
      acs_turn_on();
  }
  last_charge_status = acs_get_status();
};
