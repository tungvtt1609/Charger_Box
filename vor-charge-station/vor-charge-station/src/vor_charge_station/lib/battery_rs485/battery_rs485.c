#include "battery_rs485.h"

#define LOG_BATTERY_INFO 0

char read_basic_info_cmd[7] = {0xDD, 0xA5, 0x03, 0x00, 0xFF, 0xFD, 0x77};
char read_cell_voltage_cmd[7] = {0xDD, 0xA5, 0x04, 0x00, 0xFF, 0xFC, 0x77};
char read_hardware_ver_cmd[7] = {0xDD, 0xA5, 0x05, 0x00, 0xFF, 0xFB, 0x77};

void acs_uart_config(){
       uart_config_t uart_config = {
        .baud_rate = ACS_RS485_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_driver_install(ACS_RS485_UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(ACS_RS485_UART_PORT, &uart_config);
    uart_set_pin(ACS_RS485_UART_PORT, ACS_RS485_TXD, ACS_RS485_RXD, RTS, CTS);
}

void send_read_basic_info_cmd(void){
    for (uint8_t i = 0; i < 7; i++){
        uart_write_bytes(ACS_RS485_UART_PORT,&read_basic_info_cmd[i],1);
    }
}

void send_read_cell_voltage_cmd(void){
    for (uint8_t i = 0; i < 7; i++){
        uart_write_bytes(ACS_RS485_UART_PORT,&read_cell_voltage_cmd[i],1);
    }
}
void send_read_read_hardware_ver_cmd(void){
    for (uint8_t i = 0; i < 7; i++){
        uart_write_bytes(ACS_RS485_UART_PORT,&read_hardware_ver_cmd[i],1);
    }
}
void send_read_all_cmd(void){
    send_read_basic_info_cmd();
    send_read_cell_voltage_cmd();
    send_read_read_hardware_ver_cmd();
}


int16_t eletric_current_raw, voltage_raw, remaining_capacity, nominal_capacity; 
uint8_t *data_buffer;

void get_basic_info(void){
    //while(1){
        // Send command to BMS to read data feedback from BMS
        send_read_basic_info_cmd();
        int len = uart_read_bytes(2, data_buffer, BUF_SIZE-1, 20 / portTICK_RATE_MS);
        if(len>0 && data_buffer[0]==0xDD && data_buffer[1]==0x3) {
            // Processing received data
            voltage_raw = data_buffer[4] << 8 | data_buffer[5];
            eletric_current_raw = data_buffer[6] << 8 | data_buffer[7];
            remaining_capacity = data_buffer[8] << 8 | data_buffer[9];
            nominal_capacity = data_buffer[10] << 8 | data_buffer[11];;
        } 
        get_battery_voltage();
        get_battery_current();
        // get_remaining_capacity();
        get_percentage();
    //}
}

float get_battery_voltage(void){
    float volt = voltage_raw/100.0f;
    #if LOG_BATTERY_INFO 
        ESP_LOGI(TAG, "Voltage: %f V", volt);
    #endif
    return volt;
}

float get_battery_current(void)
{
    float current = eletric_current_raw/100.0f;
    #if LOG_BATTERY_INFO 
        ESP_LOGI(TAG,"Current: %f A",current);
    #endif
    return current;
}

float get_remaining_capacity(void)
{
    float remaning_cap = remaining_capacity * 10;
    #if LOG_BATTERY_INFO 
        ESP_LOGI(TAG,"Remaining capacity: %f mAh",remaning_cap);
    #endif
    return remaning_cap; // mAh
}

float get_percentage(void){
    float percent = (remaining_capacity * 1.0f / nominal_capacity) * 100.0f;
    #if LOG_BATTERY_INFO 
        ESP_LOGI(TAG,"Percent: %f ",percent);
    #endif
    return percent;
}

