#ifndef VOR_CHARGE_PINOUT_H
#define VOR_CHARGE_PINOUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/gpio.h"
#include "driver/rmt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

// relay control define
#define RELAY_0 GPIO_NUM_4 // 35 can only be input
#define RELAY_1 GPIO_NUM_33
#define RELAY_2 GPIO_NUM_32

// input define
#define X0 GPIO_NUM_35
#define X1 GPIO_NUM_34
#define X2 GPIO_NUM_39
#define WF_CONFIG_BT_Pin GPIO_NUM_13
#define IRR_Pin GPIO_NUM_19 // IR_Receive

// output define
#define Y0 GPIO_NUM_25
#define Y1 GPIO_NUM_26
#define Y2 GPIO_NUM_27
#define Y3 GPIO_NUM_14
#define IRC_Pin GPIO_NUM_18 // IR_Control
#define AUTO_MAN X2

#define STATUS_LED Y0
#define ERROR_LED Y1
#define AUTO 0
#define MAN 1

#define STATE_ON 0
#define STATE_OFF 1
#define ACS_STATUS_ON 1
#define ACS_STATUS_OFF 0
#define TEST_LED_DEV_MODULE GPIO_NUM_2

void gpio_setup();
void relay_on(gpio_num_t relay_number);
void relay_off(gpio_num_t relay_number);
void charging();
void check_charging_mode();
void gpio_toggle_pin(gpio_num_t pin);
void acs_turn_on(void);
void acs_turn_off(void);
void acs_switch_state(void);
uint8_t acs_get_status(void);

#ifdef __cplusplus
}
#endif

#endif
