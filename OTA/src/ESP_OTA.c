#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "sdkconfig.h"
#include "string.h"

#include "esp_log.h"
#include "errno.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"

#include "ESP_OTA.h"

#define BUFFSIZE 1024
#define HASH_LEN 32 /* SHA-256 digest length */
char Version[] = "key:d8b9c9670c9717be1f8c74c9e0eb83ec82020fd791c8d0ac";

static const char *TAG = "ESP_OTA_TEST";
// static char ota_write_data[BUFFSIZE + 1] = {0};

const int uart_port0 = UART_NUM_0;
const int uart_port2 = UART_NUM_2;

char ATcommand[220];
uint8_t rx_buffer[1024] = {0};
uint8_t ATisOK = 0;

char decode_data[1026] = {0};
// unsigned char *out;
static char out[BUFFSIZE + 1] = {0};
size_t out_len;
int b64invs[] = {62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58,
                 59, 60, 61, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2, 3, 4, 5,
                 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28,
                 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
                 43, 44, 45, 46, 47, 48, 49, 50, 51};

void b64_generate_decode_table();
int b64_isvalidchar(char c);
size_t b64_decoded_size(const char *in);

int b64_decode(const char *in /*, unsigned char *out*/, size_t outlen)
{
    size_t len;
    size_t i;
    size_t j;
    int v;

    if (in == NULL || out == NULL)
    // if (in == NULL)
    {
        printf("NULL\n");
        if (out == NULL)
        {
            printf("Out is NULL \n");
        }
        return 0;
    }

    len = strlen(in);
    // printf("len = %d\r\n", len);
    // printf("rx_buff_len = %d\r\n", outlen);
    if (outlen < b64_decoded_size(in) || len % 4 != 0)
    {
        // printf("\n outlen = %d\n", outlen);
        printf("Invalid input data size\n");
        return 0;
    }

    for (i = 0; i < len; i++)
    {
        if (!b64_isvalidchar(in[i]))
        {
            printf("Invalid char\n");
            return 0;
        }
    }

    b64_generate_decode_table();

    for (i = 0, j = 0; i < len; i += 4, j += 3)
    {
        v = b64invs[in[i] - 43];
        v = (v << 6) | b64invs[in[i + 1] - 43];
        v = in[i + 2] == '=' ? v << 6 : (v << 6) | b64invs[in[i + 2] - 43];
        v = in[i + 3] == '=' ? v << 6 : (v << 6) | b64invs[in[i + 3] - 43];

        out[j] = (v >> 16) & 0xFF;
        // printf(out[j]);
        // printf("Converted Value %d: %c \n", j, out[j]);
        if (in[i + 2] != '=')
            out[j + 1] = (v >> 8) & 0xFF;
        // printf(out[j+1]);
        // printf("Converted Value %d: %c \n", j+1, out[j+1]);
        if (in[i + 3] != '=')
            out[j + 2] = v & 0xFF;
        // printf(out[j+2]);
        // printf("Converted Value %d: %c \n", j+2, out[j+2]);
    }

    return 1;
}

int b64_isvalidchar(char c)
{
    if (c >= '0' && c <= '9')
        // printf("c = %c\r\n", c);
        return 1;
    if (c >= 'A' && c <= 'Z')
        // printf("c = %c\r\n", c);
        return 1;
    if (c >= 'a' && c <= 'z')
        // printf("c = %c\r\n", c);
        return 1;
    if (c == '+' || c == '/' || c == '=')
        // printf("c = %c\r\n", c);
        return 1;
    return 0;
}

size_t b64_decoded_size(const char *in)
{
    size_t len;
    size_t ret;
    size_t i;

    if (in == NULL)
        return 0;

    len = strlen(in);
    ret = len / 4 * 3;

    for (i = len; i-- > 0;)
    {
        if (in[i] == '=')
        {
            ret--;
        }
        else
        {
            break;
        }
    }

    return ret;
}

void b64_generate_decode_table()
{
    int inv[80];
    size_t i;

    memset(inv, -1, sizeof(inv));
    for (i = 0; i < sizeof(b64invs) - 1; i++)
    {
        inv[b64invs[i] - 43] = i;
    }
}

// static void __attribute__((noreturn)) task_fatal_error(void)
// {
//     ESP_LOGE(TAG, "Exiting task due to fatal error...");
//     (void)vTaskDelete(NULL);

//     while (1)
//     {
//         ;
//     }
// }

static void print_sha256(const uint8_t *image_hash, const char *label)
{
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i)
    {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(TAG, "%s: %s", label, hash_print);
}

// static void infinite_loop(void)
// {
//     int i = 0;
//     ESP_LOGI(TAG, "When a new firmware is available on the server, press the reset button to download it");
//     while (1)
//     {
//         ESP_LOGI(TAG, "Waiting for a new firmware ... %d", ++i);
//         vTaskDelay(2000 / portTICK_PERIOD_MS);
//     }
// }

static void ESP_OTA_task(void *pvParameter)
{
    while (check_version())
    {
        printf("\n\tStarting OTA\n");

        esp_err_t err;

        esp_ota_handle_t update_handle = 0;
        const esp_partition_t *update_partition = NULL;

        ESP_LOGI(TAG, "Starting ESP OTA");

        const esp_partition_t *configured = esp_ota_get_boot_partition();
        const esp_partition_t *running = esp_ota_get_running_partition();

        if (configured != running)
        {
            ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                     configured->address, running->address);
            ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
        }
        ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
                 running->type, running->subtype, running->address);

        update_partition = esp_ota_get_next_update_partition(NULL);
        assert(update_partition != NULL);
        ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
                 update_partition->subtype, update_partition->address);

        sprintf(ATcommand, "AT+HTTPINIT\r\n");
        uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
        uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 2000 / portTICK_PERIOD_MS);
        uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
        uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));
        memset(rx_buffer, 0, sizeof(rx_buffer));
        memset(ATcommand, 0, sizeof(ATcommand));

        sprintf(ATcommand, "AT+HTTPPARA=\"URL\",\"https://s3.ap-south-1.amazonaws.com/yatri.static/embedded/ESP_OTA_ota_0.txt\"\r\n");
        uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
        uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 2000 / portTICK_PERIOD_MS);
        uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
        uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));
        memset(rx_buffer, 0, sizeof(rx_buffer));
        memset(ATcommand, 0, sizeof(ATcommand));

        sprintf(ATcommand, "AT+HTTPACTION=0\r\n");
        uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
        uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 7000 / portTICK_PERIOD_MS);
        uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
        uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));

        if (strstr((char *)rx_buffer, "+HTTPACTION"))
        {
            char *data1[30] = {0};
            int i1 = 0;
            char delim1[] = ",";
            char *ptr1 = strtok((char *)rx_buffer, delim1);
            while (ptr1 != NULL)
            {
                data1[i1] = ptr1;
                ptr1 = strtok(NULL, delim1);
                i1++;
            }
            // uart_write_bytes(uart_port0, (uint8_t *)"\n", strlen("\n"));
            // uart_write_bytes(uart_port0, (uint8_t *)data1[2], strlen((char *)data1[2]));
            // uart_write_bytes(uart_port0, (uint8_t *)"\n", strlen("\n"));
            vTaskDelay(50 / portTICK_PERIOD_MS);
            fileSize = atoi(data1[2]);
            File_Size = ((fileSize * 3) / 4);
            printf("file size (base64): %lld\n", fileSize);
            printf("file size (hex): %lld\n", File_Size);
            memset(rx_buffer, 0, sizeof(rx_buffer));
        }
        else
        {
            printf("Network Error\n");
            esp_restart();
        }

        err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
            //
            esp_restart();
        }
        ESP_LOGI(TAG, "esp_ota_begin succeeded");

        __int32_t i = 0;
        int len = 0;
        __int32_t data_bytes = 0;
        int max;
        int choose = 0;

        while (!Flag_File_End)
        {
            if ((fileSize - i) >= 1024)
            {
                data_bytes = 1024;
            }
            else
            {
                data_bytes = fileSize - i;
            }

            if ((i + data_bytes) == fileSize)
            {
                Flag_File_End = 1;
            }

            memset(rx_buffer, 0, sizeof(rx_buffer));
            sprintf(ATcommand, "AT+HTTPREAD=%d,%d\r\n", i, data_bytes);
            // rx_buffer[i+1024] = '\0';
            uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
            uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
            uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
            uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));

            if (strstr((char *)rx_buffer, "+HTTPREAD"))
            {
                char *data2[10] = {0};
                int i2 = 0;
                char delim2[] = "\n";
                char *ptr2 = strtok((char *)rx_buffer, delim2);
                while (ptr2 != NULL)
                {
                    data2[i2] = ptr2;
                    ptr2 = strtok(NULL, delim2);
                    i2++;
                }
                max = 0;

                for (int j = 0; j < i2; j++)
                {
                    if (strlen(data2[j]) > max)
                    {
                        max = strlen(data2[j]);
                        choose = j;
                    }
                }

                for (int h = 0; h < data_bytes; h++)
                {
                    decode_data[h] = data2[choose][h];
                }
            }
            else
            {
                printf("Failed to fetch data\nNetwork Error!!\n");
                esp_restart();
            }
            len = strlen(decode_data);
            i += len;
            out_len = len;
            // out = malloc(len);
            if (!b64_decode(decode_data, out_len))
            {
                printf("Decode Failure\nNetwork Error\n");
                esp_restart();
            }

            printf("Decoded sucessfully\n");
            printf("OUT: %s\n", out);
            printf("LENGTH: %d\n", strlen(out));

            err = esp_ota_write(update_handle, (const void *)out, strlen(out));
            if (err != ESP_OK)
            {
                esp_restart();
            }
            // free(out);
        }
        Flag_File_End = 0;

        sprintf(ATcommand, "AT+HTTPTERM\r\n");
        uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
        uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 5000 / portTICK_PERIOD_MS);
        uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
        uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));
        memset(rx_buffer, 0, sizeof(rx_buffer));
        memset(ATcommand, 0, sizeof(ATcommand));
        vTaskDelay(50 / portTICK_PERIOD_MS);
        /////////////////////////////////////

        err = esp_ota_end(update_handle);
        if (err != ESP_OK)
        {
            printf("OTA end failed\n");
            esp_restart();
        }

        err = esp_ota_set_boot_partition(update_partition);
        if (err != ESP_OK)
        {
            printf("OTA set boot partition\n");
            esp_restart();
        }
        ESP_LOGI(TAG, "Prepare to restart system!");
        esp_restart();
        // return;
    }
    vTaskDelay(1);
}

int check_version()
{
    printf("Inside Check Version\n");
    sprintf(ATcommand, "AT+HTTPTERM\r\n");
    uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
    uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 2000 / portTICK_PERIOD_MS);
    uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
    uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));
    memset(rx_buffer, 0, sizeof(rx_buffer));
    memset(ATcommand, 0, sizeof(ATcommand));

    sprintf(ATcommand, "AT+HTTPINIT\r\n");
    uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
    uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 2000 / portTICK_PERIOD_MS);
    uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
    uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));
    memset(rx_buffer, 0, sizeof(rx_buffer));
    memset(ATcommand, 0, sizeof(ATcommand));

    sprintf(ATcommand, "AT+HTTPPARA=\"URL\",\"http://api.embedded.yatrimotorcycles.com/api/v1/bikes/bike-software\"\r\n");
    uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
    uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 2000 / portTICK_PERIOD_MS);
    uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
    uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));
    memset(rx_buffer, 0, sizeof(rx_buffer));
    memset(ATcommand, 0, sizeof(ATcommand));

    sprintf(ATcommand, "AT+HTTPPARA=\"USERDATA\",\"%s\"\r\n", Version);
    uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
    uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 2000 / portTICK_PERIOD_MS);
    uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
    uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));
    memset(rx_buffer, 0, sizeof(rx_buffer));
    memset(ATcommand, 0, sizeof(ATcommand));

    sprintf(ATcommand, "AT+HTTPACTION=0\r\n");
    uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
    uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 7000 / portTICK_PERIOD_MS);
    uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
    uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));
    memset(rx_buffer, 0, sizeof(rx_buffer));
    memset(ATcommand, 0, sizeof(ATcommand));

    sprintf(ATcommand, "AT+HTTPREAD=0,1024\r\n");
    uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
    uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
    uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
    uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));
    // memset(rx_buffer, 0, sizeof(rx_buffer));
    // memset(ATcommand, 0, sizeof(ATcommand));

    if (strstr((char *)rx_buffer, "success"))
    {
        char *data3[20] = {0};
        int i3 = 0;
        char delim3[] = "\"";
        char *ptr3 = strtok((char *)rx_buffer, delim3);
        while (ptr3 != NULL)
        {
            data3[i3] = ptr3;
            ptr3 = strtok(NULL, delim3);
            i3++;
        }

        sprintf(new_version, "%s", data3[13]); /*new_version = data3[13];*/
    }
    else
    {
        sprintf(ATcommand, "AT+HTTPTERM\r\n");
        uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
        uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
        uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));
        memset(rx_buffer, 0, sizeof(rx_buffer));
        memset(ATcommand, 0, sizeof(ATcommand));
        vTaskDelay(50 / portTICK_PERIOD_MS);
        printf("Connection Error!\n");
        return 0;
    }

    sprintf(ATcommand, "AT+HTTPTERM\r\n");
    uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
    uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 2000 / portTICK_PERIOD_MS);
    uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
    uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));
    memset(rx_buffer, 0, sizeof(rx_buffer));
    memset(ATcommand, 0, sizeof(ATcommand));

    // current_version = "1.0.2";

    printf("current version = %s\n", current_version);
    printf("new version = %s\n", new_version);

    if (strcmp(new_version, current_version))
    {
        printf("New update available\n");
        // new_verlen = strlen(new_version);
        printf("Check version Completed\n");
        return 1;
    }
    else
    {
        printf("Already installed version\n");
        printf("Check version Completed\n");
        return 0;
    }
}

void gpio_init()
{
    gpio_reset_pin(SIM_POWER_PIN);
    gpio_set_direction(SIM_POWER_PIN, GPIO_MODE_OUTPUT);
}

void sim7600_powerup()
{
    uart_write_bytes(uart_port0, "POWERING UP\n", strlen("POWERING UP\n"));
    gpio_set_level(SIM_POWER_PIN, 1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    gpio_set_level(SIM_POWER_PIN, 0);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    uart_write_bytes(uart_port0, "LEAVING POWERING UP\n", strlen("LEAVING POWERING UP\n"));
}

void sim7600_powerdown()
{
    uart_write_bytes(uart_port0, "POWERING DOWN\n", strlen("POWERING DOWN\n"));
    gpio_set_level(SIM_POWER_PIN, 0);
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    gpio_set_level(SIM_POWER_PIN, 1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    uart_write_bytes(uart_port0, "LEAVING POWERING DOWN\n", strlen("LEAVING POWERING DOWN\n"));
}

void sim7600_init() // SIM7600 INITIALIZATION TEST TASK
{
    uart_write_bytes(uart_port0, "VERIFYING SIM MODULE STATUS\n", strlen("VERIFYING SIM MODULE STATUS\n"));
    while (!ATisOK)
    {
        sprintf(ATcommand, "AT\r\n");
        uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
        uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
        if (strstr((char *)rx_buffer, "OK"))
        {
            ATisOK = 1;
            count = 0;
            uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));
        }
        else
        {
            uart_write_bytes(uart_port0, "COUNTING\n", strlen("COUNTING\n"));
            count++;
            if (count == 5)
            {
                uart_write_bytes(uart_port0, "POWERING UP\n", strlen("POWERING UP\n"));
                sim7600_powerup();
            }
        }
        vTaskDelay(1);
    }
    uart_write_bytes(0, "VERIFIED\n", strlen("VERIFIED\n"));
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    ATisOK = 0;
    sprintf(ATcommand, "AT+CTZU=1\r\n");
    uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
    uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
    uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
    uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));
    vTaskDelay(50 / portTICK_PERIOD_MS);
    memset(rx_buffer, 0, sizeof(rx_buffer));
    memset(ATcommand, 0, sizeof(ATcommand));
    sprintf(ATcommand, "AT+CTZR=1\r\n");
    uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
    uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
    uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
    uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));
    vTaskDelay(50 / portTICK_PERIOD_MS);
    memset(rx_buffer, 0, sizeof(rx_buffer));
    memset(ATcommand, 0, sizeof(ATcommand));
    vTaskDelay(50 / portTICK_PERIOD_MS);

    // delete all sms from sim module storage //
    sprintf(ATcommand, "AT+CMGD=1,4\r\n");
    uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
    uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 3000 / portTICK_PERIOD_MS);
    uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
    uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));
    vTaskDelay(50 / portTICK_PERIOD_MS);
    memset(rx_buffer, 0, sizeof(rx_buffer));
    memset(ATcommand, 0, sizeof(ATcommand));
}

void ssl_init() // SSL COFIGURATION AND LTE ACTIVATION TASK
{
    // activating LTE service and then initializing ssl configurations
    sprintf(ATcommand, "AT+CNMP=2\r\n");
    uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
    uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
    uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
    uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));
    memset(rx_buffer, 0, sizeof(rx_buffer));
    memset(ATcommand, 0, sizeof(ATcommand));
    vTaskDelay(50 / portTICK_PERIOD_MS);

    sprintf(ATcommand, "AT+CSSLCFG=\"sslversion\",0,4\r\n");
    uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
    uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
    uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
    uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));
    memset(rx_buffer, 0, sizeof(rx_buffer));
    memset(ATcommand, 0, sizeof(ATcommand));
    vTaskDelay(50 / portTICK_PERIOD_MS);

    sprintf(ATcommand, "AT+CSSLCFG=\"authmode\",0,0\r\n");
    uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
    uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
    uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
    uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));
    memset(rx_buffer, 0, sizeof(rx_buffer));
    memset(ATcommand, 0, sizeof(ATcommand));
    vTaskDelay(50 / portTICK_PERIOD_MS);

    sprintf(ATcommand, "AT+CCHSET=1\r\n");
    uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
    uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
    uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
    uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));
    memset(rx_buffer, 0, sizeof(rx_buffer));
    memset(ATcommand, 0, sizeof(ATcommand));
    vTaskDelay(50 / portTICK_PERIOD_MS);

    sprintf(ATcommand, "AT+CCHSTART\r\n");
    uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
    uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
    uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
    uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));
    memset(rx_buffer, 0, sizeof(rx_buffer));
    memset(ATcommand, 0, sizeof(ATcommand));
    vTaskDelay(50 / portTICK_PERIOD_MS);

    sprintf(ATcommand, "AT+CCHOPEN=0,\"www.yatrimotorcycles.com\",443,2\r\n");
    uart_write_bytes(uart_port2, ATcommand, strlen((char *)ATcommand));
    uart_read_bytes(uart_port2, rx_buffer, BUF_SIZE, 3000 / portTICK_PERIOD_MS);
    uart_write_bytes(uart_port0, ATcommand, strlen((char *)ATcommand));
    uart_write_bytes(uart_port0, rx_buffer, strlen((char *)rx_buffer));
    memset(rx_buffer, 0, sizeof(rx_buffer));
    memset(ATcommand, 0, sizeof(ATcommand));
    vTaskDelay(50 / portTICK_PERIOD_MS);
}

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

static bool diagnostic(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_NUM_4);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Diagnostics (5 sec)...");
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    bool diagnostic_is_ok = gpio_get_level(GPIO_NUM_4);

    gpio_reset_pin(GPIO_NUM_4);
    return diagnostic_is_ok;
}

void app_main(void)
{
    gpio_init();
    uart_init();
    sim7600_init();
    ssl_init();

    uint8_t sha_256[HASH_LEN] = {0};
    esp_partition_t partition;

    // get sha256 digest for the partition table
    partition.address = ESP_PARTITION_TABLE_OFFSET;
    partition.size = ESP_PARTITION_TABLE_MAX_LEN;
    partition.type = ESP_PARTITION_TYPE_DATA;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for the partition table: ");

    // get sha256 digest for bootloader
    partition.address = ESP_BOOTLOADER_OFFSET;
    partition.size = ESP_PARTITION_TABLE_OFFSET;
    partition.type = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for bootloader: ");

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    print_sha256(sha_256, "SHA-256 for current firmware: ");

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK)
    {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY)
        {
            // run diagnostic function ...
            bool diagnostic_is_ok = diagnostic();
            if (diagnostic_is_ok)
            {
                ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution ...");
                esp_ota_mark_app_valid_cancel_rollback();
            }
            else
            {
                ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    xTaskCreate(ESP_OTA_task, "ESP_OTA_task", 8192, NULL, 5, NULL);
}
