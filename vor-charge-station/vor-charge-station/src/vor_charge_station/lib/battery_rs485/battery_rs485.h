#ifndef _Battery_RS485_H_
#define _Battery_RS485_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"



#define ACS_RS485_TXD           GPIO_NUM_17
#define ACS_RS485_RXD           GPIO_NUM_16
#define ACS_RS485_UART_PORT     UART_NUM_2
#define RTS                     UART_PIN_NO_CHANGE
#define CTS                     UART_PIN_NO_CHANGE
#define ACS_RS485_BAUDRATE      9600

#define BUF_SIZE                1024

void acs_uart_config();
void send_read_basic_info_cmd(void);
void send_read_cell_voltage_cmd(void);
void send_read_read_hardware_ver_cmd(void);
void send_read_all_cmd(void);
float get_battery_voltage(void);
float get_battery_current(void);
float get_remaining_capacity(void);
float get_percentage(void);
void get_basic_info(void);



#ifdef __cplusplus
}
#endif

#endif