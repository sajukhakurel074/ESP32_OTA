#ifndef ESP_OTA_H_
#define ESP_OTA_H_

#define BUF_SIZE (3000)
#define LED_1 2
#define LED_2 4
#define LED_3 15
#define UART_2_RX 16
#define UART_2_TX 17
#define SIM_POWER_PIN 13

__int64_t fileSize;
__int64_t File_Size;
uint8_t count = 0;
int Flag_File_End;

void sim7600_powerup();
void sim7600_powerdown();
void sim7600_init();
void ssl_init();
void uart_init();
char current_version[] = "1.0.1";
char new_version[50];

int check_version();
// static void ESP_OTA(void *pvParameter);

#endif