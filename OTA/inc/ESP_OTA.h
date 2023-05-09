#ifndef ESP_OTA_H_
#define ESP_OTA_H_

#define BUF_SIZE (3000)
#define LED_1 2
#define LED_2 4
#define LED_3 15
#define UART_2_RX 16
#define UART_2_TX 17
#define SIM_POWER_PIN 13

#include "main.h"

__int64_t fileSize;
__int64_t File_Size;
int Flag_File_End;

extern bool OTA_ACTIVATE;

void sim7600_powerup();
void sim7600_powerdown();
void sim7600_init();
void ssl_init();
void uart_init();
void gpio_init();
extern char current_version[];
extern uint8_t count;
char new_version[50];

void ESP_OTA_task();
int check_version();
bool ESP32_OTA();

#endif