// ***************************************************************
//                        Attention!
//
// UART1 Debug enabled on GPIO17 / GPIO18

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
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
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"
#include "diskio_impl.h"

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

enum {BLINK_NOT_MOUNTED = 250, BLINK_MOUNTED = 1000, BLINK_SUSPENDED = 2500};

void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event);
bool tusb_cdc_data_available(void);
const char* tusb_cdc_read_string(void);

void led_blinky_cb(TimerHandle_t xTimer);
void usb_device_task(void* param);
static void configure_pins(usb_hal_context_t *usb);

StaticTimer_t blinky_tmdef;
TimerHandle_t blinky_tm;
StackType_t  usb_device_stack[(3*configMINIMAL_STACK_SIZE)];
StaticTask_t usb_device_taskdef;

static char rxBuffer[RX_BUFFER_SIZE];
volatile static bool dataReceived = false;



static volatile DSTATUS Stat = STA_NOINIT;

/* Private function prototypes -----------------------------------------------*/
static DSTATUS SD_CheckStatus(BYTE lun);
DSTATUS SD_initialize (BYTE);
DSTATUS SD_status (BYTE);
DRESULT SD_read (BYTE, BYTE*, DWORD, UINT);
DRESULT SD_write (BYTE, const BYTE*, DWORD, UINT);
DRESULT SD_ioctl (BYTE, BYTE, void*);

const ff_diskio_impl_t  driver =
{
  .init = SD_initialize,
  .status = SD_status,
  .read = SD_read,
  .write = SD_write,
  .ioctl = SD_ioctl
};

extern uint8_t msc_disk[];
extern volatile bool diskReady;


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
    xTaskCreateStatic(usb_device_task, "usbd", (3*configMINIMAL_STACK_SIZE), NULL, configMAX_PRIORITIES-1, usb_device_stack, &usb_device_taskdef);
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
    /*
    for (int i = 0; i < 5; i++)
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        gpio_set_level(LED, i % 2);
    }
    */

    // Test if logging works
    vTaskDelay(500 / portTICK_PERIOD_MS);
    esp_log_level_set(MAIN_TAG, ESP_LOG_DEBUG);                 // Set log to highetst level
    fprintf(stdout, "\033[2J\033[1;1H");                        // Clear screen
    ESP_LOGD(MAIN_TAG, "ESP_LOGD: This is a debug output");     // White
    ESP_LOGI(MAIN_TAG, "ESP_LOGI: This is a info output");      // Green
    ESP_LOGW(MAIN_TAG, "ESP_LOGW: This is a warning output");   // Yellow
    ESP_LOGE(MAIN_TAG, "ESP_LOGE: This is a error output");     // Red
    fprintf(stdout, "\n");



    static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;
    const char *base_path = "/ram";
    const char *drv = "";
    const size_t workbuf_size = 4096;
    void *workbuf = NULL;
    esp_err_t result = ESP_OK;
    
    
    ESP_LOGI(MAIN_TAG, "Mounting FAT filesystem");
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4,
            .format_if_mount_failed = true,
            .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
    };
    
    //vTaskDelay(2000 / portTICK_PERIOD_MS);
    FATFS *fs;
    esp_err_t err = esp_vfs_fat_register(base_path, drv, 4, &fs);
    if (err != ESP_OK) {
        ESP_LOGE(MAIN_TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
    }
    ESP_LOGI(MAIN_TAG, "Registered!");

    ff_diskio_register(0, &driver);

    FRESULT fresult = f_mount(fs, drv, 1);
    if (fresult != FR_OK) {
        ESP_LOGW(MAIN_TAG, "f_mount failed (%d)", fresult);
        if (!(fresult == FR_NO_FILESYSTEM && mount_config.format_if_mount_failed)) {
            result = ESP_FAIL;
            //goto fail;
        }
        workbuf = malloc(workbuf_size);
        ESP_LOGI(MAIN_TAG, "Formatting FATFS partition");
        fresult = f_mkfs(drv, FM_ANY | FM_SFD, workbuf_size, workbuf, workbuf_size);
        if (fresult != FR_OK) {
            result = ESP_FAIL;
            ESP_LOGE(MAIN_TAG, "f_mkfs failed (%d)", fresult);
            //goto fail;
        }
        free(workbuf);
        workbuf = NULL;
        ESP_LOGI(MAIN_TAG, "Mounting again");
        fresult = f_mount(fs, drv, 0);
        if (fresult != FR_OK) {
            result = ESP_FAIL;
            ESP_LOGE(MAIN_TAG, "f_mount failed after formatting (%d)", fresult);
            //goto fail;
        }
    }
    ESP_LOGI(MAIN_TAG, "Done!");

    diskReady = true;




    /*
    FATFS fs;           // Filesystem object
    FIL fil;            // File object
    FRESULT res;        // API result code
    UINT bw;            // Bytes written
    static BYTE work[4096]; // Work area (larger is better for processing time)

  
    ESP_LOGI(MAIN_TAG, "Start");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    DWORD plist[] = {100, 0, 0, 0};  // 1 primary partition with 100% of space.
    uint8_t buf[512] = {0};          // Working buffer for f_fdisk function.
    FRESULT r = f_fdisk(0, plist, buf);
    if (r != FR_OK)
    {
      ESP_LOGE(MAIN_TAG, "Error, f_fdisk failed with error code");
    }

    ESP_LOGI(MAIN_TAG, "Partitioned flash!");
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    // Format the default drive with default parameters
    
    ESP_LOGI(MAIN_TAG, "Creating and formatting FAT filesystem (this takes ~60 seconds)...");
    r = f_mkfs("", FM_ANY, 0, buf, sizeof(buf));
    if (r != FR_OK)
    {
      ESP_LOGE(MAIN_TAG, "Error, f_mkfs failed with error code:");
    }
    ESP_LOGI(MAIN_TAG, "Formatted flash!");
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    */

    /*


    vTaskDelay(2000 / portTICK_PERIOD_MS);
    ESP_LOGI(MAIN_TAG, "f_mkfs");
    // Give a work area to the default drive
    f_mount(&fs, "", 0);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    ESP_LOGI(MAIN_TAG, "f_mount");
    // Create a file as new
    res = f_open(&fil, "hello.txt", FA_CREATE_NEW | FA_WRITE);
    if (res)
    {
      ESP_LOGE(MAIN_TAG, "1");
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    ESP_LOGI(MAIN_TAG, "f_open");
    // Write a message
    f_write(&fil, "Hello, World!\r\n", 15, &bw);
    if (bw != 15)
    {
      ESP_LOGE(MAIN_TAG, "2");
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    ESP_LOGI(MAIN_TAG, "f_write");
    // Close the file
    f_close(&fil);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    ESP_LOGI(MAIN_TAG, "f_close");
    // Unregister work area
    f_mount(0, "", 0);
    ESP_LOGI(MAIN_TAG, "f_mount");

    */


    
    



    
    







    /*

    ESP_LOGI(MAIN_TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(MAIN_TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(MAIN_TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(MAIN_TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(MAIN_TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(MAIN_TAG, "Partition size: total: %d, used: %d", total, used);
    }

    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(MAIN_TAG, "Opening file");
    FILE* f = fopen("/spiffs/hello.txt", "w");
    if (f == NULL) {
        ESP_LOGE(MAIN_TAG, "Failed to open file for writing");
        return;
    }
    fprintf(f, "Hello World!\n");
    fclose(f);
    ESP_LOGI(MAIN_TAG, "File written");

    // Check if destination file exists before renaming
    struct stat st;
    if (stat("/spiffs/foo.txt", &st) == 0) {
        // Delete it if it exists
        unlink("/spiffs/foo.txt");
    }

    // Rename original file
    ESP_LOGI(MAIN_TAG, "Renaming file");
    if (rename("/spiffs/hello.txt", "/spiffs/foo.txt") != 0) {
        ESP_LOGE(MAIN_TAG, "Rename failed");
        return;
    }

    // Open renamed file for reading
    ESP_LOGI(MAIN_TAG, "Reading file");
    f = fopen("/spiffs/foo.txt", "r");
    if (f == NULL) {
        ESP_LOGE(MAIN_TAG, "Failed to open file for reading");
        return;
    }
    char line[64];
    fgets(line, sizeof(line), f);
    fclose(f);
    // strip newline
    char* pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(MAIN_TAG, "Read from file: '%s'", line);

    // All done, unmount partition and disable SPIFFS
    esp_vfs_spiffs_unregister(conf.partition_label);
    ESP_LOGI(MAIN_TAG, "SPIFFS unmounted");

    */

    
    while (1)
    {
        //ESP_LOGD(MAIN_TAG, "Time: %d", (uint32_t) (esp_timer_get_time() / 1000));
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






/**
  * @brief  Initializes a Drive
  * @param  lun : not used
  * @retval DSTATUS: Operation status
  */
DSTATUS SD_initialize(BYTE lun)
{
  ESP_LOGI(MAIN_TAG, "SD_initialize");
  return STA_NOINIT & ~STA_NOINIT;
}

/**
  * @brief  Gets Disk Status
  * @param  lun : not used
  * @retval DSTATUS: Operation status
  */
DSTATUS SD_status(BYTE lun)
{
  ESP_LOGI(MAIN_TAG, "SD_status");
  return STA_NOINIT & ~STA_NOINIT;
}

/**
  * @brief  Reads Sector(s)
  * @param  lun : not used
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT SD_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
  ESP_LOGI(MAIN_TAG, "SD_read");
  memcpy(buff, msc_disk[sector * 512], 512 * count);
  return RES_OK;
}

/**
  * @brief  Writes Sector(s)
  * @param  lun : not used
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT SD_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
  ESP_LOGI(MAIN_TAG, "SD_write");
  memcpy(msc_disk[sector * 512], buff, 512 * count);
  return RES_OK;
}

/**
  * @brief  I/O control operation
  * @param  lun : not used
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
DRESULT SD_ioctl(BYTE lun, BYTE cmd, void *buff)
{
  ESP_LOGI(MAIN_TAG, "SD_ioctl");
  return RES_OK;
}