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
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_event.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "tusb_console.h"
#include "sdkconfig.h"

#define LED                 5
#define RX_BUFFER_SIZE      128

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

static const char *MAIN_TAG = "main";

void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event);
bool tusb_cdc_data_available(void);
const char* tusb_cdc_read_string(void);
static char rxBuffer[RX_BUFFER_SIZE];
volatile static bool dataReceived = false;




/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum  {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

// static timer
StaticTimer_t blinky_tmdef;
TimerHandle_t blinky_tm;

// static task for usbd
// Increase stack size when debug log is enabled
#if CFG_TUSB_DEBUG
  #define USBD_STACK_SIZE     (3*configMINIMAL_STACK_SIZE)
#else
  #define USBD_STACK_SIZE     (3*configMINIMAL_STACK_SIZE/2)
#endif

StackType_t  usb_device_stack[USBD_STACK_SIZE];
StaticTask_t usb_device_taskdef;

// static task for cdc
#define CDC_STACK_SZIE      configMINIMAL_STACK_SIZE
StackType_t  cdc_stack[CDC_STACK_SZIE];
StaticTask_t cdc_taskdef;


void led_blinky_cb(TimerHandle_t xTimer);
void usb_device_task(void* param);
void cdc_task(void* params);





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




    //board_init();

    // soft timer for blinky
    blinky_tm = xTimerCreateStatic(NULL, pdMS_TO_TICKS(BLINK_NOT_MOUNTED), true, NULL, led_blinky_cb, &blinky_tmdef);
    xTimerStart(blinky_tm, 0);

    // Create a task for tinyusb device stack
    //(void) xTaskCreateStatic( usb_device_task, "usbd", USBD_STACK_SIZE, NULL, configMAX_PRIORITIES-1, usb_device_stack, &usb_device_taskdef);

    // Create CDC task
    //(void) xTaskCreateStatic( cdc_task, "cdc", CDC_STACK_SZIE, NULL, configMAX_PRIORITIES-2, cdc_stack, &cdc_taskdef);

    //vTaskStartScheduler();



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









// USB Device Driver task
// This top level thread process all usb events and invoke callbacks
void usb_device_task(void* param)
{
  (void) param;

  // This should be called after scheduler/kernel is started.
  // Otherwise it could cause kernel issue since USB IRQ handler does use RTOS queue API.
  tusb_init();

  // RTOS forever loop
  while (1)
  {
    // tinyusb device task
    tud_task();
  }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  xTimerChangePeriod(blinky_tm, pdMS_TO_TICKS(BLINK_MOUNTED), 0);
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  xTimerChangePeriod(blinky_tm, pdMS_TO_TICKS(BLINK_NOT_MOUNTED), 0);
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  xTimerChangePeriod(blinky_tm, pdMS_TO_TICKS(BLINK_SUSPENDED), 0);
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  xTimerChangePeriod(blinky_tm, pdMS_TO_TICKS(BLINK_MOUNTED), 0);
}



//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinky_cb(TimerHandle_t xTimer)
{
  (void) xTimer;
  static bool led_state = false;

  gpio_set_level(LED, led_state);
  led_state = 1 - led_state; // toggle
}
