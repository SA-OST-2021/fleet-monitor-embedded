/* USB Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// DESCRIPTION:
// This example contains minimal code to make ESP32-S2 based device
// recognizable by USB-host devices as a USB Serial Device printing output from
// the application.

//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include <stdio.h>
#include <stdlib.h>
#include <sys/reent.h>
#include <driver/gpio.h>
#include "esp_log.h"
#include "esp_vfs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "tusb_console.h"
#include "sdkconfig.h"
#include "esp_efuse.h"
#include "esp_efuse_table.h"

#define LED                 5
#define RX_BUFFER_SIZE      128

static const char *MAIN_TAG = "main";

void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event);
bool tusb_cdc_data_available(void);
const char* tusb_cdc_read_string(void);
static char rxBuffer[RX_BUFFER_SIZE];
volatile static bool dataReceived = false;

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

void app_main(void)
{
    // Init USB
    tinyusb_config_t tusb_cfg = {0};                        // the configuration uses default values
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    tinyusb_config_cdcacm_t amc_cfg = {0};                  // the configuration uses default values
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&amc_cfg));
    tinyusb_cdcacm_register_callback(0, CDC_EVENT_RX, &tinyusb_cdc_rx_callback);
    esp_tusb_init_console(TINYUSB_CDC_ACM_0);               // log to usb
    

    // Init GPIO
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << LED);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_drive_capability(LED, GPIO_DRIVE_CAP_MAX);
    gpio_set_level(LED, 0);

    // Blink LED on startup
    for (int i = 0; i < 5; i++)
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        gpio_set_level(LED, i % 2);
    }

    // Wait for Serial Monitor
    while(!tud_cdc_connected())
    {
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    // We are connected to Serial Monitor!
    vTaskDelay(500 / portTICK_PERIOD_MS);
    esp_log_level_set(MAIN_TAG, ESP_LOG_DEBUG);                 // Set log to highetst level
    fprintf(stdout, "\033[2J\033[1;1H");                        // Clear screen
    ESP_LOGD(MAIN_TAG, "ESP_LOGD: This is a debug output");     // White
    ESP_LOGI(MAIN_TAG, "ESP_LOGI: This is a info output");      // Green
    ESP_LOGW(MAIN_TAG, "ESP_LOGW: This is a warning output");   // Yellow
    ESP_LOGE(MAIN_TAG, "ESP_LOGE: This is a error output");     // Red
    fprintf(stdout, "\n");

    uint8_t efuseUartPrintControl, efuseUartPrintChannel;
    esp_efuse_read_field_blob(ESP_EFUSE_UART_PRINT_CONTROL, &efuseUartPrintControl, 2);
    esp_efuse_read_field_blob(ESP_EFUSE_UART_PRINT_CHANNEL, &efuseUartPrintChannel, 1);

    if (efuseUartPrintControl == 3 && efuseUartPrintChannel == 1)
    {
        ESP_LOGI(MAIN_TAG, "EFUSE have been set correctly!");
    }
    else
    {
        ESP_LOGW(MAIN_TAG, "EFUSE are not set:");
        ESP_LOGD(MAIN_TAG, "ESP_EFUSE_UART_PRINT_CONTROL: %d", efuseUartPrintControl);
        ESP_LOGD(MAIN_TAG, "ESP_EFUSE_UART_PRINT_CHANNEL: %d", efuseUartPrintChannel); 

        ESP_LOGE(MAIN_TAG, "Type \"BURN\" if you want to disable and re-route bootloader messages (UART0 -> UART1)");
        ESP_LOGE(MAIN_TAG, "WARNING: This in an irreversible operation that cannot be undone! Are you sure what you are doing?");

        while (!tusb_cdc_data_available())
        {
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        if(strcmp(tusb_cdc_read_string(), "BURN") == 0)
        {
            efuseUartPrintControl = 0x03;
            efuseUartPrintChannel = 0x01;
            esp_efuse_write_field_blob(ESP_EFUSE_UART_PRINT_CONTROL, &efuseUartPrintControl, 2);
            esp_efuse_write_field_blob(ESP_EFUSE_UART_PRINT_CHANNEL, &efuseUartPrintChannel, 1);
            ESP_LOGI(MAIN_TAG, "EFUSE have been burned!");
        }
        else
        {
            ESP_LOGE(MAIN_TAG, "Aborted!");
        }
    }    

    while (1)
    {
        //ESP_LOGD(MAIN_TAG, "example: print -> stdout");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event)
{
    static uint32_t writePointer = 0;
    size_t rx_size = 0;
    if (tinyusb_cdcacm_read(itf, (uint8_t*) (rxBuffer + writePointer), min(RX_BUFFER_SIZE, RX_BUFFER_SIZE - writePointer), &rx_size) ==  ESP_OK)
    {
        for (int i = 0; i < rx_size; i++)
        {
            if(rxBuffer[writePointer] == '\b')
            {
                tinyusb_cdcacm_write_queue(itf, (uint8_t*) "\010 \010", sizeof("\010 \010"));
                if (writePointer)
                {
                    writePointer--;
                }
            }
            else
            {
                tinyusb_cdcacm_write_queue(itf, (uint8_t*) (rxBuffer + writePointer), 1);
                if (rxBuffer[writePointer] == '\n' || rxBuffer[writePointer] == '\r')
                {
                    rxBuffer[writePointer] = '\0';
                    dataReceived = true;
                    writePointer = 0;
                }
                else if (writePointer < RX_BUFFER_SIZE - 1)
                {
                    writePointer++;
                }
            }
        }
        tinyusb_cdcacm_write_flush(itf, 0);
    }
    else
    {
        ESP_LOGE(MAIN_TAG, "Read error");
    }
}

bool tusb_cdc_data_available(void)
{
    return dataReceived;
}

const char* tusb_cdc_read_string(void)
{
    dataReceived = false;
    return rxBuffer;
}