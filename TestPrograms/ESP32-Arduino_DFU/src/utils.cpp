/*
 * Fleet-Monitor Software
 * Copyright (C) 2021 Institute of Networked Solutions OST
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "utils.h"
#include "task_hmi.h"
#include "USB.h"
#include "SPI.h"
#include "SdFat.h"
#include "Adafruit_SPIFlash.h"
#include "Adafruit_TinyUSB.h"
#include "EEPROM.h"
#include "format/ff.h"
#include "format/diskio.h"
#include "freertos/task.h"
#include "secure_boot.h"  // Include this local version befor global efuse module, due to compile issues
#include "esp_efuse.h"
#include "esp_efuse_table.h"
#include "soc/soc.h"
#include "soc/efuse_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/system_reg.h"
#include "soc/usb_periph.h"
#include "soc/usb_reg.h"

#include "esp32s2/rom/usb/chip_usb_dw_wrapper.h"
#include "esp32s2/rom/usb/usb_dc.h"
#include "esp32s2/rom/usb/cdc_acm.h"
#include "esp32s2/rom/usb/usb_persist.h"

#define EEPROM_SIZE          64
#define EEPROM_ADDR_STATUS   0x00
#define EEPROM_ADDR_PASSWORD 0x01
#define EEPROM_STATUS_MAGIC  0xA5

static int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize);
static int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize);
static void msc_flush_cb(void);
static volatile bool updated = false;
static Settings settings;

USBCDC USBSerial;
Adafruit_USBD_MSC usb_msc;
Adafruit_FlashTransport_ESP32 flashTransport;
Adafruit_SPIFlash flash(&flashTransport);
FatFileSystem fatfs;
SystemParser systemParser;

/**
 * @brief Initialization of USB, flash, fatfs and efuses
 *
 * @param labelName is the name to be displayed on the computer
 * @param forceFormat is used to maunally reformat the filesystem
 * @return true on success
 * @return false on error
 */
bool utils_init(const char* labelName, bool forceFormat) {
  USBSerial.begin();
  //USB.enableDFU();
  //USB.productName("Fleet-Monitor");
  
  USB.begin(); 

  //USBSerial.printf(CLEAR_TERMINAL);
  //USBSerial.println("FleetMonitor_V0.1");

  if (!EEPROM.begin(EEPROM_SIZE)) {
    USBSerial.println("[UTILS] Failed to initialise EEPROM");
    return 0;
  }

  if (!flash.begin()) {
    USBSerial.println("[UTILS] Error, failed to initialize flash chip!");
    return 0;
  }

  if (!fatfs.begin(&flash) || forceFormat)  // Check if disk must be formated
  {
    USBSerial.println("[UTILS] No FAT File system found, try to format disk...");
    utils_format(labelName);
  }

  if (!utils_updateEfuse()) {
    USBSerial.println("[UTILS] Could not update Efuses");
    return false;
  }

  yield();
  vTaskDelay(200);
  return true;
}

/**
 * @brief Load system configuration
 *
 * @param fileName is the name of the config file
 * @return true on success
 * @return false on error
 */
bool utils_systemConfig(const char* fileName) {
  if (!systemParser.loadFile(fileName)) {
    USBSerial.println("[UTILS] System config loading failed.");
    return false;
  }
  USBSerial.println("[UTILS] System config loading was successful.");

  settings.ssid = systemParser.getSsid();
  settings.hostIp = systemParser.getHostIp();
  settings.hostPort = systemParser.getHostPort();
  settings.configMode = systemParser.getConfigMode();
  settings.connectionType = systemParser.getConnectionType();
  settings.overwriteFile = systemParser.getOverwriteState();
  const char* password = systemParser.getPassword();
  bool valid = false;
  if (password) {
    if (strlen(password) > 0) {
      valid = true;
    }
  }
  if (valid) {
    settings.password = password;
    USBSerial.printf("[UTILS] New Password has been set, now store in EEPROM: %s\n", settings.password);
    for (int i = 0; i < EEPROM_SIZE - EEPROM_ADDR_PASSWORD; i++) {
      EEPROM.write(EEPROM_ADDR_PASSWORD + i, settings.password[i]);
      if (i >= strlen(settings.password)) break;  // Copy Null-Terminator aswell
    }
    EEPROM.write(EEPROM_ADDR_STATUS, EEPROM_STATUS_MAGIC);
    EEPROM.commit();
  } else {
    if (EEPROM.read(EEPROM_ADDR_STATUS) == EEPROM_STATUS_MAGIC) {
      USBSerial.println("[UTILS] EEPROM is valid");
      static char localPassword[EEPROM_SIZE - EEPROM_ADDR_PASSWORD];
      for (int i = 0; i < EEPROM_SIZE - EEPROM_ADDR_PASSWORD; i++) {
        localPassword[i] = EEPROM.read(EEPROM_ADDR_PASSWORD + i);
        if (localPassword[i] == '\0') break;
      }
      settings.password = (const char*)localPassword;
      USBSerial.printf("[UTILS] Password has been loaded from EEPROM: %s\n", settings.password);
    } else {
      USBSerial.println("[UTILS] EEPROM is invalid");
      settings.password = "";
    }
  }

  if (systemParser.getBootloaderMode()) {
    USBSerial.println("[UTILS] Going to reset ESP32...");
    hmi_setLed(led_t{.type = LED_STATUS, .mode = LED_OFF, .color = NONE});
    hmi_setLed(led_t{.type = LED_CAN, .mode = LED_OFF, .color = NONE});
    utils_enterDfuMode();
  }

  return true;
}

/**
 * @brief Initializes the mass storage device
 *
 * @return true
 * @return false
 */
bool utils_startMsc(void) {
  usb_msc.setID("Onway AG", "Fleet-Monitor", "1.0");
  usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);  // Set callback
  usb_msc.setCapacity(flash.size() / 512,
                      512);    // Set disk size, block size should be 512 regardless of spi flash page size
  usb_msc.setUnitReady(true);  // MSC is ready for read/write
  return usb_msc.begin();
}

/**
 * @brief Format the filesystem
 *
 * @param labelName is the name for the mass storage device
 * @return true on success
 * @return false on error
 */
bool utils_format(const char* labelName) {
  static FATFS elmchamFatfs;
  static uint8_t workbuf[4096];  // Working buffer for f_fdisk function.

  USBSerial.println("[UTILS] Partitioning flash with 1 primary partition...");
  static DWORD plist[] = {100, 0, 0, 0};      // 1 primary partition with 100% of space.
  static uint8_t buf[512] = {0};              // Working buffer for f_fdisk function.
  static FRESULT r = f_fdisk(0, plist, buf);  // Partition the flash with 1 partition that takes the entire space.
  if (r != FR_OK) {
    USBSerial.print("[UTILS] Error, f_fdisk failed with error code: ");
    USBSerial.println(r, DEC);
    return 0;
  }
  USBSerial.println("[UTILS] Partitioned flash!");
  USBSerial.println("[UTILS] Creating and formatting FAT filesystem (this takes ~60 seconds)...");
  r = f_mkfs("", FM_FAT | FM_SFD, 0, workbuf, sizeof(workbuf));  // Make filesystem.
  if (r != FR_OK) {
    USBSerial.print("[UTILS] Error, f_mkfs failed with error code: ");
    USBSerial.println(r, DEC);
    return 0;
  }

  r = f_mount(&elmchamFatfs, "0:", 1);  // mount to set disk label
  if (r != FR_OK) {
    USBSerial.print("[UTILS] Error, f_mount failed with error code: ");
    USBSerial.println(r, DEC);
    return 0;
  }
  USBSerial.print("[UTILS] Setting disk label to: ");
  USBSerial.println(labelName);
  r = f_setlabel(labelName);  // Setting label
  if (r != FR_OK) {
    USBSerial.print("[UTILS] Error, f_setlabel failed with error code: ");
    USBSerial.println(r, DEC);
    return 0;
  }
  f_unmount("0:");     // unmount
  flash.syncBlocks();  // sync to make sure all data is written to flash
  USBSerial.println("[UTILS] Formatted flash!");
  if (!fatfs.begin(&flash))  // Check new filesystem
  {
    USBSerial.println("[UTILS] Error, failed to mount newly formatted filesystem!");
    return 0;
  }
  USBSerial.println("[UTILS] Flash chip successfully formatted with new empty filesystem!");
  yield();
  return 1;
}

/**
 * @brief Check if something in the filesystem changed
 *
 * @param clearFlag if the flag is to be cleared
 * @return true
 * @return false
 */
bool utils_isUpdated(bool clearFlag) {
  bool status = updated;
  if (clearFlag) updated = false;
  return status;
}

/**
 * @brief Set the efuses on first bootup, they will never be updated after
 *
 * @return true on success
 * @return false on error
 */
bool utils_updateEfuse(void) {
  esp_err_t err = ESP_OK;
  uint8_t efuseUartPrintControl, efuseUartPrintChannel;
  err |= esp_efuse_read_field_blob(ESP_EFUSE_UART_PRINT_CONTROL, &efuseUartPrintControl, 2);
  err |= esp_efuse_read_field_blob(ESP_EFUSE_UART_PRINT_CHANNEL, &efuseUartPrintChannel, 1);

  if (efuseUartPrintControl == 3 && efuseUartPrintChannel == 1) {
    USBSerial.println("[UTILS] EFuse have been set correctly!");
  } else {
    efuseUartPrintControl = 0x03;
    efuseUartPrintChannel = 0x01;
    err |= esp_efuse_write_field_blob(ESP_EFUSE_UART_PRINT_CONTROL, &efuseUartPrintControl, 2);
    err |= esp_efuse_write_field_blob(ESP_EFUSE_UART_PRINT_CHANNEL, &efuseUartPrintChannel, 1);
    USBSerial.println("[UTILS] EFuse have been burned!");
  }
  return (err == ESP_OK);
}

void utils_enterDfuMode(void)
{
  usb_persist_restart(RESTART_BOOTLOADER);
  //usb_persist_restart(RESTART_BOOTLOADER_DFU);
  //tud_dfu_runtime_reboot_to_dfu_cb();

  //USB0.grstctl |= USB_CSFTRST;
  //while ((USB0.grstctl & USB_CSFTRST) == USB_CSFTRST){}
  //chip_usb_set_persist_flags(USBDC_BOOT_DFU);
  //REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);

/*
  TinyUSBDevice.detach();
  USB0.grstctl |= USB_CSFTRST;
  while ((USB0.grstctl & USB_CSFTRST) == USB_CSFTRST);
  vTaskDelay(3000);  // Give some time to save file on flash

  taskENTER_CRITICAL(0);
  chip_usb_set_persist_flags(USBDC_BOOT_DFU);
  REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
  vTaskDelay(1000);
  esp_restart();
  */
}

/**
 * @brief Get the utils setting
 *
 * @return Settings& with the settings
 */
Settings& utils_getSettings(void) { return settings; }

/**
 * @brief Get the server address
 *
 * @return String& with the server url, including the port
 */
String& utils_getServerAddress(void) {
  static String address = "http://" + String(settings.hostIp);
  if (settings.hostPort != 80) {
    address += ":" + String(settings.hostPort);
  }
  return address;
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and
// return number of copied bytes (must be multiple of block size)
static int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize) {
  return flash.readBlocks(lba, (uint8_t*)buffer, bufsize / 512) ? bufsize : -1;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and
// return number of written bytes (must be multiple of block size)
static int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize) {
  return flash.writeBlocks(lba, buffer, bufsize / 512) ? bufsize : -1;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host). Used to flush any pending
// cache.
static void msc_flush_cb(void) {
  flash.syncBlocks();  // sync with flash
  fatfs.cacheClear();  // clear file system's cache to force refresh
  updated = true;
}

//--------------------------------------------------------------------+
// fatfs diskio
//--------------------------------------------------------------------+
extern "C" {
DSTATUS disk_status(BYTE pdrv) {
  (void)pdrv;
  return 0;
}

DSTATUS disk_initialize(BYTE pdrv) {
  (void)pdrv;
  return 0;
}

DRESULT disk_read(BYTE pdrv,     // Physical drive nmuber to identify the drive
                  BYTE* buff,    // Data buffer to store read data
                  DWORD sector,  // Start sector in LBA
                  UINT count     // Number of sectors to read
) {
  (void)pdrv;
  return flash.readBlocks(sector, buff, count) ? RES_OK : RES_ERROR;
}

DRESULT disk_write(BYTE pdrv,         // Physical drive nmuber to identify the drive
                   const BYTE* buff,  // Data to be written
                   DWORD sector,      // Start sector in LBA
                   UINT count         // Number of sectors to write
) {
  (void)pdrv;
  return flash.writeBlocks(sector, buff, count) ? RES_OK : RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv,  // Physical drive nmuber (0..)
                   BYTE cmd,   // Control code
                   void* buff  // Buffer to send/receive control data
) {
  (void)pdrv;

  switch (cmd) {
    case CTRL_SYNC:
      flash.syncBlocks();
      return RES_OK;

    case GET_SECTOR_COUNT:
      *((DWORD*)buff) = flash.size() / 512;
      return RES_OK;

    case GET_SECTOR_SIZE:
      *((WORD*)buff) = 512;
      return RES_OK;

    case GET_BLOCK_SIZE:
      *((DWORD*)buff) = 8;  // erase block size in units of sector size
      return RES_OK;

    default:
      return RES_PARERR;
  }
}
}
