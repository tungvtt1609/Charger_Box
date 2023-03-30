#ifndef _ACS_IR_PROTOCOL_H_
#define _ACS_IR_PROTOCOL_H_

#define IRR_Pin GPIO_NUM_19 // IR_Receive
#define IRC_Pin GPIO_NUM_18 // IR_Control
#define ADDR 0x01
#define DATA_ON 0xDD
#define DATA_OFF 0xAA
static void ir_rx_task(void *arg);
static void ir_tx_task(void *arg);
void ir_protocol_task_tx_start(void);
void ir_protocol_task_rx_start(void);
#endif