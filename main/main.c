#include "main.h"

int uart_port0 = UART_NUM_0;
int uart_port2 = UART_NUM_2;

void uart_init() // UART INITIALIZATION TASK
{
    uart_config_t uart_config0 = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

    uart_config_t uart_config2 = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
    uart_param_config(uart_port0, &uart_config0);
    uart_param_config(uart_port2, &uart_config2);

    uart_set_pin(uart_port0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_set_pin(uart_port2, UART_2_TX, UART_2_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_driver_install(uart_port0, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_driver_install(uart_port2, BUF_SIZE * 2, 0, 0, NULL, 0);
}

void gpio_init()
{
    gpio_reset_pin(SIM_POWER_PIN);
    gpio_set_direction(SIM_POWER_PIN, GPIO_MODE_OUTPUT);
}

void app_main(void)
{
    gpio_init();
    uart_init();
    sim7600_init();
    ssl_init();

    // Initialize Task WatchDog Timer
    ESP_ERROR_CHECK(esp_task_wdt_init(WDT_TIMEOUT, false));

    OTA_ACTIVATE = true;
    xTaskCreate(ESP_OTA_task, "ESP_OTA_task", 8192, NULL, 5, NULL);
}
