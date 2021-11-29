#include "utils.h"
#include "USB.h"
#include "SPI.h"
#include "SdFat.h"
#include "Adafruit_SPIFlash.h"
#include "Adafruit_TinyUSB.h"
#include "format/ff.h"
#include "format/diskio.h"

int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize);
int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize);
void msc_flush_cb(void);
static volatile bool updated = false;

USBCDC USBSerial;
Adafruit_USBD_MSC usb_msc;
Adafruit_FlashTransport_ESP32 flashTransport;
Adafruit_SPIFlash flash(&flashTransport);
FatFileSystem fatfs;

bool utils_init(const char* labelName, bool forceFormat) {
  USB.begin();
  USB.productName("Fleet-Monitor");
  USBSerial.begin(0);
<<<<<<< HEAD
  USBSerial.println("ESP32-Arduino_USB_MSC_Flash");

  if (!flash.begin()) {
    USBSerial.println("Error, failed to initialize flash chip!");
    return 0;
  }

  if (!fatfs.begin(&flash) || forceFormat)  // Check if disk must be formated
  {
    USBSerial.println("No FAT File system found, try to format disk...");
    utils_format(labelName);
  }

  USBSerial.println("Adafruit TinyUSB Mass Storage External Flash example");
  USBSerial.print("JEDEC ID: 0x");
  USBSerial.println(flash.getJEDECID(), HEX);
  USBSerial.print("Flash size: ");
  USBSerial.print(flash.size() / 1024);
  USBSerial.println(" KB");

  usb_msc.setID("Onway AG", "Fleet-Monitor", "1.0");
  usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);  // Set callback
  usb_msc.setCapacity(flash.size() / 512,
                      512);    // Set disk size, block size should be 512 regardless of spi flash page size
  usb_msc.setUnitReady(true);  // MSC is ready for read/write
  usb_msc.begin();

  // while(!USBSerial) yield();
  /*

   // First call begin to mount the filesystem.  Check that it returns true
    // to make sure the filesystem was mounted.
    if (!fatfs.begin(&flash)) {
      USBSerial.println("Error, failed to mount newly formatted filesystem!");
      USBSerial.println("Was the flash chip formatted with the SdFat_format example?");
      while(1) yield();
    }
    USBSerial.println("Mounted filesystem!");

    // Check if a directory called 'test' exists and create it if not there.
    // Note you should _not_ add a trailing slash (like '/test/') to directory names!
    // You can use the same exists function to check for the existance of a file too.
    if (!fatfs.exists("/test")) {
      USBSerial.println("Test directory not found, creating...");

      // Use mkdir to create directory (note you should _not_ have a trailing slash).
      fatfs.mkdir("/test");

      if ( !fatfs.exists("/test") ) {
        USBSerial.println("Error, failed to create directory!");
        while(1) yield();
      }else {
        USBSerial.println("Created directory!");
      }
    }

    // You can also create all the parent subdirectories automatically with mkdir.
    // For example to create the hierarchy /test/foo/bar:
    USBSerial.println("Creating deep folder structure...");
    if ( !fatfs.exists("/test/foo/bar") ) {
      USBSerial.println("Creating /test/foo/bar");
      fatfs.mkdir("/test/foo/bar");

      if ( !fatfs.exists("/test/foo/bar") ) {
        USBSerial.println("Error, failed to create directory!");
        while(1) yield();
      }else {
        USBSerial.println("Created directory!");
      }
    }

    // This will create the hierarchy /test/foo/baz, even when /test/foo already exists:
    if ( !fatfs.exists("/test/foo/baz") ) {
      USBSerial.println("Creating /test/foo/baz");
      fatfs.mkdir("/test/foo/baz");

      if ( !fatfs.exists("/test/foo/baz") ) {
        USBSerial.println("Error, failed to create directory!");
        while(1) yield();
      }else {
        USBSerial.println("Created directory!");
      }
    }

    // Create a file in the test directory and write data to it.
    // Note the FILE_WRITE parameter which tells the library you intend to
    // write to the file.  This will create the file if it doesn't exist,
    // otherwise it will open the file and start appending new data to the
    // end of it.
    File writeFile = fatfs.open("/test/test.txt", FILE_WRITE);
    if (!writeFile) {
      USBSerial.println("Error, failed to open test.txt for writing!");
      while(1) yield();
    }
    USBSerial.println("Opened file /test/test.txt for writing/appending...");

    // Once open for writing you can print to the file as if you're printing
    // to the serial terminal, the same functions are available.
    writeFile.println("Hello world!");
    writeFile.print("Hello number: "); writeFile.println(123, DEC);
    writeFile.print("Hello hex number: 0x"); writeFile.println(123, HEX);

    // Close the file when finished writing.
    writeFile.close();
    USBSerial.println("Wrote to file /test/test.txt!");
=======
>>>>>>> 1f2049ec14c24592067429b9942193a0d0055b75

  // TODO: Set fuse bits

  if (!flash.begin()) {
    USBSerial.println("Error, failed to initialize flash chip!");
    return 0;
  }

  usb_msc.setID("Onway AG", "Fleet-Monitor", "1.0");
  usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);  // Set callback
  usb_msc.setCapacity(flash.size() / 512, 512);    // Set disk size, block size should be 512 regardless of spi flash page size
  usb_msc.setUnitReady(true);  // MSC is ready for read/write
  usb_msc.begin();

  if (!fatfs.begin(&flash) || forceFormat)  // Check if disk must be formated
  {
    USBSerial.println("No FAT File system found, try to format disk...");
    utils_format(labelName);
  }

  return true;
}

bool utils_format(const char* labelName) {
  FATFS elmchamFatfs;
  uint8_t workbuf[4096];  // Working buffer for f_fdisk function.

  USBSerial.println("Partitioning flash with 1 primary partition...");
  DWORD plist[] = {100, 0, 0, 0};      // 1 primary partition with 100% of space.
  uint8_t buf[512] = {0};              // Working buffer for f_fdisk function.
  FRESULT r = f_fdisk(0, plist, buf);  // Partition the flash with 1 partition that takes the entire space.
  if (r != FR_OK) {
    USBSerial.print("Error, f_fdisk failed with error code: ");
    USBSerial.println(r, DEC);
    return 0;
  }
  USBSerial.println("Partitioned flash!");
  USBSerial.println("Creating and formatting FAT filesystem (this takes ~60 seconds)...");
  r = f_mkfs("", FM_FAT | FM_SFD, 0, workbuf, sizeof(workbuf));  // Make filesystem.
  if (r != FR_OK) {
    USBSerial.print("Error, f_mkfs failed with error code: ");
    USBSerial.println(r, DEC);
    return 0;
  }

  r = f_mount(&elmchamFatfs, "0:", 1);  // mount to set disk label
  if (r != FR_OK) {
    USBSerial.print("Error, f_mount failed with error code: ");
    USBSerial.println(r, DEC);
    return 0;
  }
  USBSerial.print("Setting disk label to: ");
  USBSerial.println(labelName);
  r = f_setlabel(labelName);  // Setting label
  if (r != FR_OK) {
    USBSerial.print("Error, f_setlabel failed with error code: ");
    USBSerial.println(r, DEC);
    return 0;
  }
  f_unmount("0:");     // unmount
  flash.syncBlocks();  // sync to make sure all data is written to flash
  USBSerial.println("Formatted flash!");
  if (!fatfs.begin(&flash))  // Check new filesystem
  {
    USBSerial.println("Error, failed to mount newly formatted filesystem!");
    return 0;
  }
  USBSerial.println("Flash chip successfully formatted with new empty filesystem!");
  return 1;
}

bool utils_isUpdated(bool clearFlag) {
  bool status = updated;
  if (clearFlag) updated = false;
  return status;
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and
// return number of copied bytes (must be multiple of block size)
int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize) {
  return flash.readBlocks(lba, (uint8_t*)buffer, bufsize / 512) ? bufsize : -1;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and
// return number of written bytes (must be multiple of block size)
int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize) {
  return flash.writeBlocks(lba, buffer, bufsize / 512) ? bufsize : -1;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host). Used to flush any pending
// cache.
void msc_flush_cb(void) {
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
