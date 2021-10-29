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

#include "esp_rom_gpio.h"
#include "hal/gpio_ll.h"
#include "hal/usb_hal.h"
#include "soc/usb_periph.h"
#include "driver/periph_ctrl.h"
#include "driver/rmt.h"

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

// ***************************************************************
//                        Attention!
//
// UART1 Debug enabled on GPIO17 / GPIO18




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


//#if CFG_TUSB_DEBUG
  #define USBD_STACK_SIZE     (3*configMINIMAL_STACK_SIZE)
//#else
  //#define USBD_STACK_SIZE     (3*configMINIMAL_STACK_SIZE/2)
//#endif

StackType_t  usb_device_stack[USBD_STACK_SIZE];
StaticTask_t usb_device_taskdef;

// static task for cdc
#define CDC_STACK_SZIE      configMINIMAL_STACK_SIZE
StackType_t  cdc_stack[CDC_STACK_SZIE];
StaticTask_t cdc_taskdef;


void led_blinky_cb(TimerHandle_t xTimer);
void usb_device_task(void* param);
void cdc_task(void* params);



static void configure_pins(usb_hal_context_t *usb);



void app_main(void)
{
    // Init USB
    periph_module_reset(PERIPH_USB_MODULE);
    periph_module_enable(PERIPH_USB_MODULE);
    usb_hal_context_t hal = {.use_external_phy = false};    // Use built-in PHY
    usb_hal_init(&hal);
    configure_pins(&hal);
    tusb_init();

    // Setup USB CDC and reroute logger
    tinyusb_config_cdcacm_t amc_cfg = {0};
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&amc_cfg));
    tinyusb_cdcacm_register_callback(0, CDC_EVENT_RX, &tinyusb_cdc_rx_callback);
    esp_tusb_init_console(TINYUSB_CDC_ACM_0);               // Log to USB
    
  
    // Create a task for tinyusb device stack
    xTaskCreateStatic(usb_device_task, "usbd", USBD_STACK_SIZE, NULL, configMAX_PRIORITIES-1, usb_device_stack, &usb_device_taskdef);
    blinky_tm = xTimerCreateStatic(NULL, pdMS_TO_TICKS(BLINK_NOT_MOUNTED), true, NULL, led_blinky_cb, &blinky_tmdef);
    xTimerStart(blinky_tm, 0);    

    // Init GPIO
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << LED);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(LED, 0);

    // Blink LED on startup
    for (int i = 0; i < 5; i++)
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        gpio_set_level(LED, i % 2);
    }

    // Test if logging works
    vTaskDelay(500 / portTICK_PERIOD_MS);
    esp_log_level_set(MAIN_TAG, ESP_LOG_DEBUG);                 // Set log to highetst level
    fprintf(stdout, "\033[2J\033[1;1H");                        // Clear screen
    ESP_LOGD(MAIN_TAG, "ESP_LOGD: This is a debug output");     // White
    ESP_LOGI(MAIN_TAG, "ESP_LOGI: This is a info output");      // Green
    ESP_LOGW(MAIN_TAG, "ESP_LOGW: This is a warning output");   // Yellow
    ESP_LOGE(MAIN_TAG, "ESP_LOGE: This is a error output");     // Red
    fprintf(stdout, "\n");
    
    while (1)
    {
        ESP_LOGD(MAIN_TAG, "Time: %d", (uint32_t) (esp_timer_get_time() / 1000));
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



static void configure_pins(usb_hal_context_t *usb)
{
  /* usb_periph_iopins currently configures USB_OTG as USB Device.
   * Introduce additional parameters in usb_hal_context_t when adding support
   * for USB Host.
   */
  for (const usb_iopin_dsc_t *iopin = usb_periph_iopins; iopin->pin != -1; ++iopin) {
    if ((usb->use_external_phy) || (iopin->ext_phy_only == 0)) {
      esp_rom_gpio_pad_select_gpio(iopin->pin);
      if (iopin->is_output) {
        esp_rom_gpio_connect_out_signal(iopin->pin, iopin->func, false, false);
      } else {
        esp_rom_gpio_connect_in_signal(iopin->pin, iopin->func, false);
        if ((iopin->pin != GPIO_FUNC_IN_LOW) && (iopin->pin != GPIO_FUNC_IN_HIGH)) {
          gpio_ll_input_enable(&GPIO, iopin->pin);
        }
      }
      esp_rom_gpio_pad_unhold(iopin->pin);
    }
  }
  if (!usb->use_external_phy) {
    gpio_set_drive_capability(USBPHY_DM_NUM, GPIO_DRIVE_CAP_3);
    gpio_set_drive_capability(USBPHY_DP_NUM, GPIO_DRIVE_CAP_3);
  }
}


void led_blinky_cb(TimerHandle_t xTimer)
{
  (void) xTimer;
  static bool led_state = false;

  gpio_set_level(LED, led_state);
  led_state = 1 - led_state; // toggle
}



void usb_device_task(void* param)   
{
  while (1)
  {
    tud_task();
  }
}


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
