#ifndef MAIN_H_
#define MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "string.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "sdkconfig.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

/*Private Files Includes*/
#include "ESP_OTA.h"

#define WDT_TIMEOUT 6000000

extern int uart_port0;
extern int uart_port2;

#endif